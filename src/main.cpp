#include "MatchingEngine.hpp"
#include "Statistics.hpp"
#include "RiskEngine.hpp"
#include "Portfolio.hpp"
#include "WebServer.hpp"
#include "Logger.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <string>

void printBanner() {
    std::cout << "\033[36m"
              << "====================================================================\n"
              << "  ██████╗ ██████╗ ██████╗ ███████╗██████╗  ██████╗  ██████╗ ██╗  ██╗  \n"
              << "  ██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗██╔═══██╗██╔═══██╗██║ ██╔╝  \n"
              << "  ██║  ██║██████╔╝██║  ██║█████╗  ██████╔╝██║   ██║██║   ██║█████╔╝   \n"
              << "  ██║  ██║██╔══██╗██║  ██║██╔══╝  ██╔══██╗██║   ██║██║   ██║██╔═██╗   \n"
              << "  ██████╔╝██║  ██║██████╔╝███████╗██║  ██║╚██████╔╝╚██████╔╝██║  ██╗  \n"
              << "  ╚═════╝ ╚═╝  ╚═╝╚═════╝ ╚══════╝╚═╝  ╚═╝ ╚═════╝  ╚═════╝ ╚═╝  ╚═╝  \n"
              << "  \n"
              << "         Exchange Infrastructure & Matching Engine Platform v1.0    \n"
              << "====================================================================\n"
              << "\033[0m";
}

// Global flag to stop the simulation
std::atomic<bool> runSimulation{false};

void marketSimulationThread(MatchingEngine& engine, Portfolio& portfolio) {
    Logger::info("Starting automated market flow simulation...");
    
    std::default_random_engine generator;
    std::normal_distribution<double> priceDistribution(100.0, 2.0); // mean price of 100
    std::uniform_int_distribution<int> qtyDistribution(10, 100);
    std::uniform_int_distribution<int> sideDistribution(0, 1);
    std::uniform_int_distribution<int> orderTypeDistribution(0, 9); // 90% limit, 10% market
    std::uniform_int_distribution<int> traderDistribution(0, 4);

    std::vector<std::string> traders = {"MarketMakerA", "MarketMakerB", "AlgoFundX", "RetailTrder", "Arbitrageur"};
    
    // Initialize portfolios
    for (const auto& trader : traders) {
        portfolio.initializeTrader(trader, 250000.0);
    }

    uint64_t simOrderId = 500000;

    while (runSimulation) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        double rawPrice = priceDistribution(generator);
        double price = std::round(rawPrice * 100.0) / 100.0; // round to 2 decimals
        uint32_t qty = qtyDistribution(generator) * 10;
        Side side = (sideDistribution(generator) == 0) ? Side::BUY : Side::SELL;
        std::string traderId = traders[traderDistribution(generator)];

        Order order;
        order.orderId = ++simOrderId;
        order.traderId = traderId;
        order.side = side;
        order.quantity = qty;
        order.remainingQuantity = qty;

        int orderType = orderTypeDistribution(generator);
        if (orderType == 0) { // Market order
            order.price = -1.0;
        } else {
            // Standard limit order. MM buy orders slightly lower, sell orders slightly higher
            double bestBid = engine.getOrderBook().getBestBid();
            double bestAsk = engine.getOrderBook().getBestAsk();
            
            if (side == Side::BUY) {
                if (bestAsk > 0.0) {
                    order.price = std::min(price, bestAsk - 0.05); // don't cross to stay resting
                } else {
                    order.price = price - 0.1;
                }
            } else {
                if (bestBid > 0.0) {
                    order.price = std::max(price, bestBid + 0.05);
                } else {
                    order.price = price + 0.1;
                }
            }
        }

        std::vector<Trade> trades;
        engine.processOrder(order, trades);

        if (!trades.empty()) {
            Logger::info("Simulated execution: " + std::to_string(trades.size()) + " trade(s) processed.");
        }
    }
    Logger::info("Market simulation stopped.");
}

int main(int argc, char* argv[]) {
    printBanner();

    uint16_t port = 8080;
    bool simulate = false;

    // Parse CLI arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        } else if (arg == "--simulate") {
            simulate = true;
        }
    }

    MatchingEngine engine;
    Statistics stats;
    RiskEngine risk;
    Portfolio portfolio;

    // Configure risk limits
    risk.setMaxOrderQuantity(200000); // 200,000 max size
    risk.setMaxPosition("MarketMakerA", 150000);
    risk.setMaxPosition("MarketMakerB", 150000);
    risk.setMaxPosition("AlgoFundX", 100000);
    risk.setMaxPosition("RetailTrder", 10000);

    WebServer server(engine, stats, risk, portfolio);
    
    server.start(port);

    std::thread simThread;
    if (simulate) {
        runSimulation = true;
        simThread = std::thread(marketSimulationThread, std::ref(engine), std::ref(portfolio));
    }

    Logger::info("Exchange Gateway is up and running. Press Enter to shutdown.");
    std::cin.get();

    if (simulate) {
        runSimulation = false;
        if (simThread.joinable()) {
            simThread.join();
        }
    }

    server.stop();
    Logger::info("Exchange terminated cleanly. Goodbye!");
    return 0;
}
