#pragma once

#include "OrderBook.hpp"
#include <functional>
#include <vector>
#include <memory>
#include <atomic>

class MatchingEngine {
public:
    using TradeCallback = std::function<void(const Trade&)>;
    using OrderCallback = std::function<void(const Order&)>;

    MatchingEngine() : nextTradeId(1) {}

    // Core order processing
    bool processOrder(Order& order, std::vector<Trade>& outTrades);
    bool cancelOrder(uint64_t orderId, Order& outCancelledOrder);
    bool modifyOrder(uint64_t orderId, double newPrice, uint32_t newQuantity, Order& outOldOrder, Order& outNewOrder, bool& priorityLost);

    // Callbacks registrations
    void registerTradeCallback(TradeCallback callback) { tradeCallbacks.push_back(callback); }
    void registerOrderCallback(OrderCallback callback) { orderCallbacks.push_back(callback); }

    // State lookups
    OrderBook& getOrderBook() { return orderBook; }
    const OrderBook& getOrderBook() const { return orderBook; }
    const std::vector<Trade>& getTradeHistory() const { return tradeHistory; }

private:
    OrderBook orderBook;
    std::vector<Trade> tradeHistory;
    std::atomic<uint64_t> nextTradeId;

    std::vector<TradeCallback> tradeCallbacks;
    std::vector<OrderCallback> orderCallbacks;

    void triggerTradeCallbacks(const Trade& trade);
    void triggerOrderCallbacks(const Order& order);

    bool matchLimitOrder(Order& incoming, std::vector<Trade>& outTrades);
    bool matchMarketOrder(Order& incoming, std::vector<Trade>& outTrades);
    void generateTrade(Order& buyOrder, Order& sellOrder, double executionPrice, uint32_t quantity, std::vector<Trade>& outTrades);
};
