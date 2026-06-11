#include "WebServer.hpp"
#include "crow.h"
#include "Logger.hpp"
#include "nlohmann/json.hpp"
#include "DashboardHTML.hpp"
#include <set>
#include <atomic>

using json = nlohmann::json;

struct WebServer::Impl {
    crow::SimpleApp app;
    std::mutex connMutex;
    std::set<crow::websocket::connection*> activeConnections;
    std::atomic<uint64_t> nextOrderId{1000};
};

WebServer::WebServer(MatchingEngine& engine, Statistics& stats, RiskEngine& risk, Portfolio& portfolio)
    : engine(engine), stats(stats), risk(risk), portfolio(portfolio), running(false), impl(std::make_unique<Impl>()) {
    
    // Register Portfolio callbacks so that every trade execution automatically updates trader balances/positions
    engine.registerTradeCallback([this](const Trade& trade) {
        this->portfolio.onTrade(trade);
        this->stats.onTrade(trade);
    });
}

WebServer::~WebServer() {
    stop();
}

void WebServer::start(uint16_t port) {
    if (running) return;
    running = true;
    serverPort = port;

    setupEngineCallbacks();
    setupRoutes();

    serverThread = std::thread([this]() {
        Logger::info("Starting WebServer on port " + std::to_string(serverPort));
        impl->app.port(serverPort).multithreaded().run();
    });
}

void WebServer::stop() {
    if (!running) return;
    running = false;
    impl->app.stop();
    if (serverThread.joinable()) {
        serverThread.join();
    }
    Logger::info("WebServer stopped successfully");
}

void WebServer::setupEngineCallbacks() {
    // Whenever a trade occurs, broadcast to WebSockets
    engine.registerTradeCallback([this](const Trade& trade) {
        json msg;
        msg["event"] = "trade";
        msg["tradeId"] = trade.tradeId;
        msg["buyOrderId"] = trade.buyOrderId;
        msg["sellOrderId"] = trade.sellOrderId;
        msg["buyerId"] = trade.buyerId;
        msg["sellerId"] = trade.sellerId;
        msg["price"] = trade.executionPrice;
        msg["quantity"] = trade.quantity;
        msg["timestamp"] = trade.timestamp;

        std::string payload = msg.dump();
        
        std::lock_guard<std::mutex> lock(impl->connMutex);
        for (auto* conn : impl->activeConnections) {
            conn->send_text(payload);
        }
    });

    // Whenever an order status changes, broadcast depth and top-of-book
    engine.registerOrderCallback([this](const Order& order) {
        // 1. Broadcast Depth Update
        json depthMsg;
        depthMsg["event"] = "depth";
        
        json bidsArray = json::array();
        for (const auto& level : engine.getOrderBook().getBidDepth(10)) {
            bidsArray.push_back({level.first, level.second});
        }
        depthMsg["bids"] = bidsArray;

        json asksArray = json::array();
        for (const auto& level : engine.getOrderBook().getAskDepth(10)) {
            asksArray.push_back({level.first, level.second});
        }
        depthMsg["asks"] = asksArray;

        std::string depthPayload = depthMsg.dump();

        // 2. Broadcast Top of Book Update
        json topMsg;
        topMsg["event"] = "top";
        topMsg["bid"] = engine.getOrderBook().getBestBid();
        topMsg["ask"] = engine.getOrderBook().getBestAsk();
        topMsg["spread"] = engine.getOrderBook().getSpread();
        topMsg["mid"] = engine.getOrderBook().getMidPrice();

        std::string topPayload = topMsg.dump();

        std::lock_guard<std::mutex> lock(impl->connMutex);
        for (auto* conn : impl->activeConnections) {
            conn->send_text(depthPayload);
            conn->send_text(topPayload);
        }
    });
}

void WebServer::setupRoutes() {
    // 0. Static Landing Page (Exchange Dashboard UI)
    CROW_ROUTE(impl->app, "/")([]() {
        crow::response res(DASHBOARD_HTML);
        res.set_header("Content-Type", "text/html");
        return res;
    });

    // 1. WebSocket endpoint for market feed
    CROW_ROUTE(impl->app, "/market")
        .websocket(&impl->app)
        .onopen([this](crow::websocket::connection& conn) {
            Logger::info("New WebSocket connection established");
            std::lock_guard<std::mutex> lock(impl->connMutex);
            impl->activeConnections.insert(&conn);

            // Send initial depth & top-of-book on connection
            json depthMsg;
            depthMsg["event"] = "depth";
            json bidsArray = json::array();
            for (const auto& level : engine.getOrderBook().getBidDepth(10)) {
                bidsArray.push_back({level.first, level.second});
            }
            depthMsg["bids"] = bidsArray;
            json asksArray = json::array();
            for (const auto& level : engine.getOrderBook().getAskDepth(10)) {
                asksArray.push_back({level.first, level.second});
            }
            depthMsg["asks"] = asksArray;
            conn.send_text(depthMsg.dump());
        })
        .onclose([this](crow::websocket::connection& conn, const std::string& reason) {
            Logger::info("WebSocket connection closed: " + reason);
            std::lock_guard<std::mutex> lock(impl->connMutex);
            impl->activeConnections.erase(&conn);
        })
        .onmessage([this](crow::websocket::connection& conn, const std::string& data, bool is_binary) {
            // Echo or handle client message if needed
        });

    // 2. REST: Place Order
    CROW_ROUTE(impl->app, "/orders").methods("POST"_method)([this](const crow::request& req) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid JSON body");
        }

        if (!body.contains("traderId") || !body.contains("side") || !body.contains("quantity")) {
            return crow::response(400, "Missing required fields: traderId, side, quantity");
        }

        std::string traderId = body["traderId"];
        std::string sideStr = body["side"];
        uint32_t quantity = body["quantity"];
        double price = body.value("price", -1.0); // Defaults to -1.0 if market order

        Side side = (sideStr == "BUY" || sideStr == "buy") ? Side::BUY : Side::SELL;

        // Ensure trader portfolio is initialized
        portfolio.initializeTrader(traderId, portfolio.getBalance(traderId));

        uint64_t orderId = impl->nextOrderId.fetch_add(1);

        Order order{
            orderId,
            traderId,
            side,
            price,
            quantity,
            quantity,
            0, // timestamp set by engine
            OrderStatus::NEW
        };

        // Pre-trade risk checking
        int32_t currentPos = portfolio.getPosition(traderId);
        double currentBalance = portfolio.getBalance(traderId);

        if (!risk.checkOrder(order, currentPos, currentBalance)) {
            Logger::warn("Order rejected by Risk Engine: Order ID " + std::to_string(orderId) + " for " + traderId);
            json rejectRes;
            rejectRes["orderId"] = orderId;
            rejectRes["status"] = toString(OrderStatus::REJECTED);
            rejectRes["reason"] = "Risk limits exceeded (Max Order size, balance check, or position limits)";
            return crow::response(400, rejectRes.dump());
        }

        std::vector<Trade> trades;
        engine.processOrder(order, trades);

        json res;
        res["orderId"] = order.orderId;
        res["traderId"] = order.traderId;
        res["side"] = toString(order.side);
        res["price"] = order.price;
        res["quantity"] = order.quantity;
        res["remainingQuantity"] = order.remainingQuantity;
        res["status"] = toString(order.status);

        json tradesArray = json::array();
        for (const auto& t : trades) {
            json tJson;
            tJson["tradeId"] = t.tradeId;
            tJson["price"] = t.executionPrice;
            tJson["quantity"] = t.quantity;
            tradesArray.push_back(tJson);
        }
        res["trades"] = tradesArray;

        return crow::response(200, res.dump());
    });

    // 3. REST: Cancel Order
    CROW_ROUTE(impl->app, "/orders/<uint>").methods("DELETE"_method)([this](uint64_t orderId) {
        Order cancelledOrder;
        if (engine.cancelOrder(orderId, cancelledOrder)) {
            json res;
            res["orderId"] = cancelledOrder.orderId;
            res["status"] = toString(OrderStatus::CANCELLED);
            return crow::response(200, res.dump());
        }
        return crow::response(404, "Order not found or already filled");
    });

    // 4. REST: Modify Order
    CROW_ROUTE(impl->app, "/orders/<uint>").methods("PUT"_method)([this](const crow::request& req, uint64_t orderId) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (const std::exception& e) {
            return crow::response(400, "Invalid JSON body");
        }

        if (!body.contains("price") || !body.contains("quantity")) {
            return crow::response(400, "Missing required fields: price, quantity");
        }

        double price = body["price"];
        uint32_t quantity = body["quantity"];

        Order oldOrder, newOrder;
        bool priorityLost = false;

        if (engine.modifyOrder(orderId, price, quantity, oldOrder, newOrder, priorityLost)) {
            json res;
            res["orderId"] = orderId;
            res["oldPrice"] = oldOrder.price;
            res["oldQuantity"] = oldOrder.quantity;
            res["newPrice"] = newOrder.price;
            res["newQuantity"] = newOrder.quantity;
            res["priorityLost"] = priorityLost;
            res["status"] = toString(newOrder.status);
            return crow::response(200, res.dump());
        }
        return crow::response(404, "Order not found");
    });

    // 5. REST: Market Depth
    CROW_ROUTE(impl->app, "/market/depth").methods("GET"_method)([this]() {
        json res;
        
        json bidsArray = json::array();
        for (const auto& level : engine.getOrderBook().getBidDepth(20)) {
            bidsArray.push_back({level.first, level.second});
        }
        res["bids"] = bidsArray;

        json asksArray = json::array();
        for (const auto& level : engine.getOrderBook().getAskDepth(20)) {
            asksArray.push_back({level.first, level.second});
        }
        res["asks"] = asksArray;

        return crow::response(200, res.dump());
    });

    // 6. REST: Market Stats
    CROW_ROUTE(impl->app, "/market/stats").methods("GET"_method)([this]() {
        json res;
        res["bestBid"] = engine.getOrderBook().getBestBid();
        res["bestAsk"] = engine.getOrderBook().getBestAsk();
        res["spread"] = engine.getOrderBook().getSpread();
        res["midPrice"] = engine.getOrderBook().getMidPrice();
        res["volume"] = stats.getTotalVolume();
        res["vwap"] = stats.getVWAP();
        return crow::response(200, res.dump());
    });

    // 7. REST: Trader Portfolio
    CROW_ROUTE(impl->app, "/portfolio/<string>").methods("GET"_method)([this](std::string traderId) {
        double midPrice = engine.getOrderBook().getMidPrice();
        json res;
        res["traderId"] = traderId;
        res["balance"] = portfolio.getBalance(traderId);
        res["position"] = portfolio.getPosition(traderId);
        res["realizedPnL"] = portfolio.getRealizedPnL(traderId);
        res["unrealizedPnL"] = portfolio.getUnrealizedPnL(traderId, midPrice);
        res["totalPnL"] = res["realizedPnL"].get<double>() + res["unrealizedPnL"].get<double>();
        return crow::response(200, res.dump());
    });
}
