#include "OrderBook.hpp"
#include <algorithm>
#include <chrono>

bool OrderBook::addOrder(const Order& order) {
    if (orderLookup.find(order.orderId) != orderLookup.end()) {
        return false; // Order ID must be unique
    }

    if (order.side == Side::BUY) {
        auto& list = bids[order.price];
        list.push_back(order);
        orderLookup[order.orderId] = OrderLocation{ order.price, std::prev(list.end()) };
    } else {
        auto& list = asks[order.price];
        list.push_back(order);
        orderLookup[order.orderId] = OrderLocation{ order.price, std::prev(list.end()) };
    }
    return true;
}

bool OrderBook::cancelOrder(uint64_t orderId, Order& outCancelledOrder) {
    auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end()) {
        return false;
    }

    double price = lookupIt->second.price;
    auto listIt = lookupIt->second.it;
    outCancelledOrder = *listIt;

    if (outCancelledOrder.side == Side::BUY) {
        bids[price].erase(listIt);
        removePriceLevelIfEmpty(price, Side::BUY);
    } else {
        asks[price].erase(listIt);
        removePriceLevelIfEmpty(price, Side::SELL);
    }

    orderLookup.erase(lookupIt);
    return true;
}

bool OrderBook::modifyOrder(uint64_t orderId, double newPrice, uint32_t newQuantity, Order& outOldOrder, Order& outNewOrder, bool& priorityLost) {
    auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end()) {
        return false;
    }

    double oldPrice = lookupIt->second.price;
    auto listIt = lookupIt->second.it;
    outOldOrder = *listIt;

    if (oldPrice == newPrice) {
        if (newQuantity <= outOldOrder.quantity) {
            // Keep priority, just decrease quantity
            listIt->quantity = newQuantity;
            listIt->remainingQuantity = newQuantity;
            outNewOrder = *listIt;
            priorityLost = false;
        } else {
            // Price is same, but quantity increases -> loses priority
            Order tempOrder = outOldOrder;
            tempOrder.quantity = newQuantity;
            tempOrder.remainingQuantity = newQuantity;
            tempOrder.timestamp = std::chrono::steady_clock::now().time_since_epoch().count(); // refresh timestamp
            
            // Cancel and re-add
            Order dummy;
            cancelOrder(orderId, dummy);
            addOrder(tempOrder);
            outNewOrder = tempOrder;
            priorityLost = true;
        }
    } else {
        // Price changed -> loses priority
        Order tempOrder = outOldOrder;
        tempOrder.price = newPrice;
        tempOrder.quantity = newQuantity;
        tempOrder.remainingQuantity = newQuantity;
        tempOrder.timestamp = std::chrono::steady_clock::now().time_since_epoch().count(); // refresh timestamp
        
        // Cancel and re-add
        Order dummy;
        cancelOrder(orderId, dummy);
        addOrder(tempOrder);
        outNewOrder = tempOrder;
        priorityLost = true;
    }
    return true;
}

const Order* OrderBook::getOrder(uint64_t orderId) const {
    auto lookupIt = orderLookup.find(orderId);
    if (lookupIt == orderLookup.end()) {
        return nullptr;
    }
    return &(*lookupIt->second.it);
}

std::vector<std::pair<double, uint32_t>> OrderBook::getBidDepth(size_t limit) const {
    std::vector<std::pair<double, uint32_t>> depth;
    size_t count = 0;
    for (const auto& [price, list] : bids) {
        if (count >= limit) break;
        uint32_t levelQty = 0;
        for (const auto& order : list) {
            levelQty += order.remainingQuantity;
        }
        if (levelQty > 0) {
            depth.push_back({ price, levelQty });
            count++;
        }
    }
    return depth;
}

std::vector<std::pair<double, uint32_t>> OrderBook::getAskDepth(size_t limit) const {
    std::vector<std::pair<double, uint32_t>> depth;
    size_t count = 0;
    for (const auto& [price, list] : asks) {
        if (count >= limit) break;
        uint32_t levelQty = 0;
        for (const auto& order : list) {
            levelQty += order.remainingQuantity;
        }
        if (levelQty > 0) {
            depth.push_back({ price, levelQty });
            count++;
        }
    }
    return depth;
}

double OrderBook::getBestBid() const {
    for (const auto& [price, list] : bids) {
        for (const auto& order : list) {
            if (order.remainingQuantity > 0) {
                return price;
            }
        }
    }
    return 0.0;
}

double OrderBook::getBestAsk() const {
    for (const auto& [price, list] : asks) {
        for (const auto& order : list) {
            if (order.remainingQuantity > 0) {
                return price;
            }
        }
    }
    return 0.0;
}

double OrderBook::getSpread() const {
    double bid = getBestBid();
    double ask = getBestAsk();
    if (bid > 0.0 && ask > 0.0) {
        return ask - bid;
    }
    return 0.0;
}

double OrderBook::getMidPrice() const {
    double bid = getBestBid();
    double ask = getBestAsk();
    if (bid > 0.0 && ask > 0.0) {
        return (bid + ask) / 2.0;
    } else if (bid > 0.0) {
        return bid;
    } else if (ask > 0.0) {
        return ask;
    }
    return 0.0;
}

void OrderBook::removePriceLevelIfEmpty(double price, Side side) {
    if (side == Side::BUY) {
        auto it = bids.find(price);
        if (it != bids.end() && it->second.empty()) {
            bids.erase(it);
        }
    } else {
        auto it = asks.find(price);
        if (it != asks.end() && it->second.empty()) {
            asks.erase(it);
        }
    }
}
