#pragma once

#include "Common.hpp"
#include <unordered_map>
#include <string>
#include <mutex>

struct TraderPortfolio {
    std::string traderId;
    double balance = 100000.0; // Cash balance
    int32_t position = 0;      // Positive = long, negative = short
    double avgEntryPrice = 0.0;
    double realizedPnL = 0.0;
};

class Portfolio {
public:
    Portfolio() = default;

    // Trade execution callback to update portfolios
    void onTrade(const Trade& trade);

    // Initializer/Setters
    void initializeTrader(const std::string& traderId, double initialBalance);

    // Lookups
    TraderPortfolio getTraderPortfolio(const std::string& traderId) const;
    int32_t getPosition(const std::string& traderId) const;
    double getBalance(const std::string& traderId) const;
    double getUnrealizedPnL(const std::string& traderId, double currentMidPrice) const;
    double getRealizedPnL(const std::string& traderId) const;

    std::unordered_map<std::string, TraderPortfolio> getAllPortfolios() const;

private:
    mutable std::mutex mtx;
    std::unordered_map<std::string, TraderPortfolio> portfolios;

    void updateBuyer(TraderPortfolio& p, double price, uint32_t qty);
    void updateSeller(TraderPortfolio& p, double price, uint32_t qty);
};
