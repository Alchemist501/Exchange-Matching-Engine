#include "MatchingEngine.hpp"
#include <chrono>
#include <iostream>

bool MatchingEngine::processOrder(Order& order, std::vector<Trade>& outTrades) {
    order.remainingQuantity = order.quantity;
    order.status = OrderStatus::NEW;
    order.timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

    triggerOrderCallbacks(order);

    if (order.quantity == 0) {
        order.status = OrderStatus::REJECTED;
        triggerOrderCallbacks(order);
        return false;
    }

    bool matched = false;
    if (order.price <= 0.0 && order.price != -1.0) {
        // Market orders are represented with price = -1.0 or similar invalid price.
        // If price is 0.0 or negative but not -1.0, we treat as market order or reject.
        order.price = -1.0;
    }

    if (order.price == -1.0) {
        matched = matchMarketOrder(order, outTrades);
    } else {
        matched = matchLimitOrder(order, outTrades);
    }

    return matched;
}

bool MatchingEngine::cancelOrder(uint64_t orderId, Order& outCancelledOrder) {
    if (orderBook.cancelOrder(orderId, outCancelledOrder)) {
        outCancelledOrder.status = OrderStatus::CANCELLED;
        triggerOrderCallbacks(outCancelledOrder);
        return true;
    }
    return false;
}

bool MatchingEngine::modifyOrder(uint64_t orderId, double newPrice, uint32_t newQuantity, Order& outOldOrder, Order& outNewOrder, bool& priorityLost) {
    if (orderBook.modifyOrder(orderId, newPrice, newQuantity, outOldOrder, outNewOrder, priorityLost)) {
        // Trigger callback for the modification
        triggerOrderCallbacks(outNewOrder);
        return true;
    }
    return false;
}

bool MatchingEngine::matchLimitOrder(Order& incoming, std::vector<Trade>& outTrades) {
    if (incoming.side == Side::BUY) {
        auto& asks = orderBook.getAsks();
        while (incoming.remainingQuantity > 0 && !asks.empty()) {
            auto askIt = asks.begin();
            double bestAskPrice = askIt->first;
            
            if (bestAskPrice > incoming.price) {
                break; // No cross
            }

            auto& list = askIt->second;
            while (incoming.remainingQuantity > 0 && !list.empty()) {
                auto& restingOrder = list.front();
                uint32_t matchQty = std::min(incoming.remainingQuantity, restingOrder.remainingQuantity);

                generateTrade(incoming, restingOrder, bestAskPrice, matchQty, outTrades);

                incoming.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    restingOrder.status = OrderStatus::FILLED;
                    triggerOrderCallbacks(restingOrder);
                    
                    // Remove from book
                    Order dummy;
                    orderBook.cancelOrder(restingOrder.orderId, dummy);
                } else {
                    restingOrder.status = OrderStatus::PARTIALLY_FILLED;
                    triggerOrderCallbacks(restingOrder);
                }
            }
        }

        if (incoming.remainingQuantity > 0) {
            incoming.status = incoming.remainingQuantity == incoming.quantity ? OrderStatus::NEW : OrderStatus::PARTIALLY_FILLED;
            orderBook.addOrder(incoming);
        } else {
            incoming.status = OrderStatus::FILLED;
        }
    } else { // SELL side
        auto& bids = orderBook.getBids();
        while (incoming.remainingQuantity > 0 && !bids.empty()) {
            auto bidIt = bids.begin();
            double bestBidPrice = bidIt->first;

            if (bestBidPrice < incoming.price) {
                break; // No cross
            }

            auto& list = bidIt->second;
            while (incoming.remainingQuantity > 0 && !list.empty()) {
                auto& restingOrder = list.front();
                uint32_t matchQty = std::min(incoming.remainingQuantity, restingOrder.remainingQuantity);

                // Buy order is resting, Sell order is incoming. Execution price is resting price (bestBidPrice)
                generateTrade(restingOrder, incoming, bestBidPrice, matchQty, outTrades);

                incoming.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    restingOrder.status = OrderStatus::FILLED;
                    triggerOrderCallbacks(restingOrder);

                    Order dummy;
                    orderBook.cancelOrder(restingOrder.orderId, dummy);
                } else {
                    restingOrder.status = OrderStatus::PARTIALLY_FILLED;
                    triggerOrderCallbacks(restingOrder);
                }
            }
        }

        if (incoming.remainingQuantity > 0) {
            incoming.status = incoming.remainingQuantity == incoming.quantity ? OrderStatus::NEW : OrderStatus::PARTIALLY_FILLED;
            orderBook.addOrder(incoming);
        } else {
            incoming.status = OrderStatus::FILLED;
        }
    }

    triggerOrderCallbacks(incoming);
    return true;
}

bool MatchingEngine::matchMarketOrder(Order& incoming, std::vector<Trade>& outTrades) {
    if (incoming.side == Side::BUY) {
        auto& asks = orderBook.getAsks();
        while (incoming.remainingQuantity > 0 && !asks.empty()) {
            auto askIt = asks.begin();
            double bestAskPrice = askIt->first;
            auto& list = askIt->second;

            while (incoming.remainingQuantity > 0 && !list.empty()) {
                auto& restingOrder = list.front();
                uint32_t matchQty = std::min(incoming.remainingQuantity, restingOrder.remainingQuantity);

                generateTrade(incoming, restingOrder, bestAskPrice, matchQty, outTrades);

                incoming.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    restingOrder.status = OrderStatus::FILLED;
                    triggerOrderCallbacks(restingOrder);

                    Order dummy;
                    orderBook.cancelOrder(restingOrder.orderId, dummy);
                } else {
                    restingOrder.status = OrderStatus::PARTIALLY_FILLED;
                    triggerOrderCallbacks(restingOrder);
                }
            }
        }
    } else { // SELL side
        auto& bids = orderBook.getBids();
        while (incoming.remainingQuantity > 0 && !bids.empty()) {
            auto bidIt = bids.begin();
            double bestBidPrice = bidIt->first;
            auto& list = bidIt->second;

            while (incoming.remainingQuantity > 0 && !list.empty()) {
                auto& restingOrder = list.front();
                uint32_t matchQty = std::min(incoming.remainingQuantity, restingOrder.remainingQuantity);

                generateTrade(restingOrder, incoming, bestBidPrice, matchQty, outTrades);

                incoming.remainingQuantity -= matchQty;
                restingOrder.remainingQuantity -= matchQty;

                if (restingOrder.remainingQuantity == 0) {
                    restingOrder.status = OrderStatus::FILLED;
                    triggerOrderCallbacks(restingOrder);

                    Order dummy;
                    orderBook.cancelOrder(restingOrder.orderId, dummy);
                } else {
                    restingOrder.status = OrderStatus::PARTIALLY_FILLED;
                    triggerOrderCallbacks(restingOrder);
                }
            }
        }
    }

    if (incoming.remainingQuantity > 0) {
        // Any remaining quantity of market order is cancelled immediately
        incoming.status = OrderStatus::CANCELLED;
    } else {
        incoming.status = OrderStatus::FILLED;
    }

    triggerOrderCallbacks(incoming);
    return true;
}

void MatchingEngine::generateTrade(Order& buyOrder, Order& sellOrder, double executionPrice, uint32_t quantity, std::vector<Trade>& outTrades) {
    uint64_t tradeId = nextTradeId.fetch_add(1, std::memory_order_relaxed);
    uint64_t now = std::chrono::steady_clock::now().time_since_epoch().count();

    Trade trade {
        tradeId,
        buyOrder.orderId,
        sellOrder.orderId,
        buyOrder.traderId,
        sellOrder.traderId,
        executionPrice,
        quantity,
        now
    };

    tradeHistory.push_back(trade);
    outTrades.push_back(trade);

    triggerTradeCallbacks(trade);
}

void MatchingEngine::triggerTradeCallbacks(const Trade& trade) {
    for (const auto& cb : tradeCallbacks) {
        cb(trade);
    }
}

void MatchingEngine::triggerOrderCallbacks(const Order& order) {
    for (const auto& cb : orderCallbacks) {
        cb(order);
    }
}
