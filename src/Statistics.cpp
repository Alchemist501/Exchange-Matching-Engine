#include "Statistics.hpp"

void Statistics::onTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(mtx);
    totalVolume += trade.quantity;
    totalValue += (trade.executionPrice * trade.quantity);
}

double Statistics::getVWAP() const {
    std::lock_guard<std::mutex> lock(mtx);
    if (totalVolume == 0) {
        return 0.0;
    }
    return totalValue / totalVolume;
}

uint64_t Statistics::getTotalVolume() const {
    std::lock_guard<std::mutex> lock(mtx);
    return totalVolume;
}

void Statistics::reset() {
    std::lock_guard<std::mutex> lock(mtx);
    totalVolume = 0;
    totalValue = 0.0;
}
