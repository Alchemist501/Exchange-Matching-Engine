#pragma once

#include "Common.hpp"
#include <mutex>

class Statistics {
public:
    Statistics() = default;

    // Callback handlers
    void onTrade(const Trade& trade);

    // Getters
    double getVWAP() const;
    uint64_t getTotalVolume() const;

    // Reset stats
    void reset();

private:
    mutable std::mutex mtx;
    uint64_t totalVolume = 0;
    double totalValue = 0.0;
};
