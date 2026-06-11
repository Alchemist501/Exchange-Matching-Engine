#pragma once

#include <cstdint>
#include <string>
#include <list>

enum class Side {
    BUY,
    SELL
};

enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELLED,
    REJECTED
};

inline std::string toString(Side side) {
    return side == Side::BUY ? "BUY" : "SELL";
}

inline std::string toString(OrderStatus status) {
    switch (status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELLED: return "CANCELLED";
        case OrderStatus::REJECTED: return "REJECTED";
    }
    return "UNKNOWN";
}

struct Order {
    uint64_t orderId;
    std::string traderId;
    Side side;
    double price; // double representing price (or we can use int64_t cents / ticks for fixed-point, but double is fine as requested by prompt)
    uint32_t quantity;
    uint32_t remainingQuantity;
    uint64_t timestamp;
    OrderStatus status = OrderStatus::NEW;
};

struct Trade {
    uint64_t tradeId;
    uint64_t buyOrderId;
    uint64_t sellOrderId;
    std::string buyerId;
    std::string sellerId;
    double executionPrice;
    uint32_t quantity;
    uint64_t timestamp;
};

struct OrderLocation {
    double price;
    std::list<Order>::iterator it;
};
