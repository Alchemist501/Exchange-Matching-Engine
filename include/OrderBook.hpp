#pragma once

#include "Common.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <utility>

class OrderBook {
public:
    OrderBook() = default;

    // Core modifications
    bool addOrder(const Order& order);
    bool cancelOrder(uint64_t orderId, Order& outCancelledOrder);
    bool modifyOrder(uint64_t orderId, double newPrice, uint32_t newQuantity, Order& outOldOrder, Order& outNewOrder, bool& priorityLost);

    // Lookups
    const Order* getOrder(uint64_t orderId) const;
    
    // Accessors for matching
    std::map<double, std::list<Order>, std::greater<double>>& getBids() { return bids; }
    std::map<double, std::list<Order>>& getAsks() { return asks; }

    const std::map<double, std::list<Order>, std::greater<double>>& getBids() const { return bids; }
    const std::map<double, std::list<Order>>& getAsks() const { return asks; }

    // Market data depth queries
    std::vector<std::pair<double, uint32_t>> getBidDepth(size_t limit = 10) const;
    std::vector<std::pair<double, uint32_t>> getAskDepth(size_t limit = 10) const;

    double getBestBid() const;
    double getBestAsk() const;
    double getSpread() const;
    double getMidPrice() const;

private:
    std::map<double, std::list<Order>, std::greater<double>> bids; // highest price first
    std::map<double, std::list<Order>> asks;                     // lowest price first
    std::unordered_map<uint64_t, OrderLocation> orderLookup;     // O(1) order location mapping

    void removePriceLevelIfEmpty(double price, Side side);
};
