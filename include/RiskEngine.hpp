#pragma once

#include "Common.hpp"
#include <unordered_map>
#include <string>
#include <mutex>

class RiskEngine {
public:
    RiskEngine();

    // Configuration
    void setMaxOrderQuantity(uint32_t qty);
    void setMaxPosition(const std::string& traderId, int32_t limit);

    // Checks if order violates any limits
    bool checkOrder(const Order& order, int32_t currentPosition, double currentBalance) const;

private:
    mutable std::mutex mtx;
    uint32_t maxOrderQty;
    std::unordered_map<std::string, int32_t> maxPositions; // Trader ID -> max position size
};
