#include "Portfolio.hpp"
#include <algorithm>
#include <cmath>

void Portfolio::onTrade(const Trade& trade) {
    std::lock_guard<std::mutex> lock(mtx);

    // Initialize if they don't exist
    if (portfolios.find(trade.buyerId) == portfolios.end()) {
        portfolios[trade.buyerId] = TraderPortfolio{ trade.buyerId };
    }
    if (portfolios.find(trade.sellerId) == portfolios.end()) {
        portfolios[trade.sellerId] = TraderPortfolio{ trade.sellerId };
    }

    updateBuyer(portfolios[trade.buyerId], trade.executionPrice, trade.quantity);
    updateSeller(portfolios[trade.sellerId], trade.executionPrice, trade.quantity);
}

void Portfolio::initializeTrader(const std::string& traderId, double initialBalance) {
    std::lock_guard<std::mutex> lock(mtx);
    portfolios[traderId] = TraderPortfolio{ traderId, initialBalance, 0, 0.0, 0.0 };
}

TraderPortfolio Portfolio::getTraderPortfolio(const std::string& traderId) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = portfolios.find(traderId);
    if (it != portfolios.end()) {
        return it->second;
    }
    return TraderPortfolio{ traderId };
}

int32_t Portfolio::getPosition(const std::string& traderId) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = portfolios.find(traderId);
    if (it != portfolios.end()) {
        return it->second.position;
    }
    return 0;
}

double Portfolio::getBalance(const std::string& traderId) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = portfolios.find(traderId);
    if (it != portfolios.end()) {
        return it->second.balance;
    }
    return 100000.0; // default initial balance
}

double Portfolio::getUnrealizedPnL(const std::string& traderId, double currentMidPrice) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = portfolios.find(traderId);
    if (it == portfolios.end() || it->second.position == 0 || currentMidPrice <= 0.0) {
        return 0.0;
    }

    const auto& p = it->second;
    if (p.position > 0) {
        return p.position * (currentMidPrice - p.avgEntryPrice);
    } else {
        return std::abs(p.position) * (p.avgEntryPrice - currentMidPrice);
    }
}

double Portfolio::getRealizedPnL(const std::string& traderId) const {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = portfolios.find(traderId);
    if (it != portfolios.end()) {
        return it->second.realizedPnL;
    }
    return 0.0;
}

std::unordered_map<std::string, TraderPortfolio> Portfolio::getAllPortfolios() const {
    std::lock_guard<std::mutex> lock(mtx);
    return portfolios;
}

void Portfolio::updateBuyer(TraderPortfolio& p, double price, uint32_t qty) {
    p.balance -= (price * qty);

    if (p.position < 0) { // Currently short
        int32_t shortQty = std::abs(p.position);
        uint32_t coverQty = std::min(qty, static_cast<uint32_t>(shortQty));
        
        // Profit/loss on covering short
        p.realizedPnL += coverQty * (p.avgEntryPrice - price);
        p.position += coverQty;

        uint32_t remQty = qty - coverQty;
        if (remQty > 0) {
            // Net long
            p.position = remQty;
            p.avgEntryPrice = price;
        } else if (p.position == 0) {
            p.avgEntryPrice = 0.0;
        }
    } else { // Currently long or flat
        double totalCost = (p.position * p.avgEntryPrice) + (qty * price);
        p.position += qty;
        p.avgEntryPrice = totalCost / p.position;
    }
}

void Portfolio::updateSeller(TraderPortfolio& p, double price, uint32_t qty) {
    p.balance += (price * qty);

    if (p.position > 0) { // Currently long
        uint32_t closeQty = std::min(qty, static_cast<uint32_t>(p.position));
        
        // Profit/loss on closing long
        p.realizedPnL += closeQty * (price - p.avgEntryPrice);
        p.position -= closeQty;

        uint32_t remQty = qty - closeQty;
        if (remQty > 0) {
            // Net short
            p.position = -static_cast<int32_t>(remQty);
            p.avgEntryPrice = price;
        } else if (p.position == 0) {
            p.avgEntryPrice = 0.0;
        }
    } else { // Currently short or flat
        int32_t absPos = std::abs(p.position);
        double totalCost = (absPos * p.avgEntryPrice) + (qty * price);
        p.position -= qty;
        p.avgEntryPrice = totalCost / std::abs(p.position);
    }
}
