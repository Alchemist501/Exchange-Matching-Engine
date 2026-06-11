#include "RiskEngine.hpp"
#include <cmath>
#include <iostream>

RiskEngine::RiskEngine() : maxOrderQty(100000) {}

void RiskEngine::setMaxOrderQuantity(uint32_t qty) {
    std::lock_guard<std::mutex> lock(mtx);
    maxOrderQty = qty;
}

void RiskEngine::setMaxPosition(const std::string& traderId, int32_t limit) {
    std::lock_guard<std::mutex> lock(mtx);
    maxPositions[traderId] = limit;
}

bool RiskEngine::checkOrder(const Order& order, int32_t currentPosition, double currentBalance) const {
    std::lock_guard<std::mutex> lock(mtx);

    // 1. Max Order Quantity Check
    if (order.quantity > maxOrderQty) {
        return false;
    }

    // 2. Buying Power Check
    if (order.side == Side::BUY && order.price > 0.0) {
        double cost = order.price * order.quantity;
        if (cost > currentBalance) {
            return false;
        }
    }

    // 3. Position Limit Check
    auto it = maxPositions.find(order.traderId);
    if (it != maxPositions.end()) {
        int32_t limit = it->second;
        int32_t orderQtySigned = (order.side == Side::BUY) ? static_cast<int32_t>(order.quantity) : -static_cast<int32_t>(order.quantity);
        int32_t prospectivePos = currentPosition + orderQtySigned;
        if (std::abs(prospectivePos) > limit) {
            return false;
        }
    }

    return true;
}
