#include <gtest/gtest.h>
#include "MatchingEngine.hpp"
#include <vector>

TEST(MatchingEngineTests, LimitOrderPriceTimePriority) {
    MatchingEngine engine;
    std::vector<Trade> trades;

    // 1. Add some sell limit orders to the book
    // Seller 1: Sell 100 @ 101.0
    Order sell1{1, "Seller1", Side::SELL, 101.0, 100, 100, 0, OrderStatus::NEW};
    engine.processOrder(sell1, trades);
    ASSERT_EQ(trades.size(), 0);

    // Seller 2: Sell 50 @ 101.0 (should sit behind Seller 1 in time priority)
    Order sell2{2, "Seller2", Side::SELL, 101.0, 50, 50, 0, OrderStatus::NEW};
    engine.processOrder(sell2, trades);
    ASSERT_EQ(trades.size(), 0);

    // Seller 3: Sell 100 @ 100.5 (should sit in front of Seller 1 due to better price priority)
    Order sell3{3, "Seller3", Side::SELL, 100.5, 100, 100, 0, OrderStatus::NEW};
    engine.processOrder(sell3, trades);
    ASSERT_EQ(trades.size(), 0);

    // 2. Incoming BUY order crossing the book
    // Buyer: BUY 150 @ 102.0
    // Matching expectation:
    // - Matches 100 @ 100.5 (Seller 3) - price improvement for buyer
    // - Matches 50 @ 101.0 (Seller 1) - time priority over Seller 2
    // - Buyer remaining qty: 0
    Order buy{4, "Buyer1", Side::BUY, 102.0, 150, 150, 0, OrderStatus::NEW};
    engine.processOrder(buy, trades);

    ASSERT_EQ(trades.size(), 2);

    // Trade 1: Seller 3 (100.5)
    EXPECT_EQ(trades[0].buyOrderId, 4);
    EXPECT_EQ(trades[0].sellOrderId, 3);
    EXPECT_DOUBLE_EQ(trades[0].executionPrice, 100.5);
    EXPECT_EQ(trades[0].quantity, 100);

    // Trade 2: Seller 1 (101.0)
    EXPECT_EQ(trades[1].buyOrderId, 4);
    EXPECT_EQ(trades[1].sellOrderId, 1);
    EXPECT_DOUBLE_EQ(trades[1].executionPrice, 101.0);
    EXPECT_EQ(trades[1].quantity, 50);

    // Check remaining book quantities
    const Order* restingSell1 = engine.getOrderBook().getOrder(1);
    ASSERT_NE(restingSell1, nullptr);
    EXPECT_EQ(restingSell1->remainingQuantity, 50);
    EXPECT_EQ(restingSell1->status, OrderStatus::PARTIALLY_FILLED);

    const Order* restingSell2 = engine.getOrderBook().getOrder(2);
    ASSERT_NE(restingSell2, nullptr);
    EXPECT_EQ(restingSell2->remainingQuantity, 50);

    const Order* restingSell3 = engine.getOrderBook().getOrder(3);
    EXPECT_EQ(restingSell3, nullptr); // Fully filled and removed
}

TEST(MatchingEngineTests, MarketOrderMatching) {
    MatchingEngine engine;
    std::vector<Trade> trades;

    // Sell limit orders on book
    Order sell1{1, "Seller1", Side::SELL, 10.0, 50, 50, 0, OrderStatus::NEW};
    engine.processOrder(sell1, trades);
    Order sell2{2, "Seller2", Side::SELL, 11.0, 100, 100, 0, OrderStatus::NEW};
    engine.processOrder(sell2, trades);

    trades.clear();

    // Market Buy Order for 75 shares
    // Matches 50 @ 10.0 and 25 @ 11.0
    Order marketBuy{3, "Buyer1", Side::BUY, -1.0, 75, 75, 0, OrderStatus::NEW};
    engine.processOrder(marketBuy, trades);

    ASSERT_EQ(trades.size(), 2);
    EXPECT_DOUBLE_EQ(trades[0].executionPrice, 10.0);
    EXPECT_EQ(trades[0].quantity, 50);
    EXPECT_DOUBLE_EQ(trades[1].executionPrice, 11.0);
    EXPECT_EQ(trades[1].quantity, 25);

    EXPECT_EQ(marketBuy.status, OrderStatus::FILLED);
    EXPECT_EQ(marketBuy.remainingQuantity, 0);

    // Check remaining depth
    const Order* restingSell2 = engine.getOrderBook().getOrder(2);
    ASSERT_NE(restingSell2, nullptr);
    EXPECT_EQ(restingSell2->remainingQuantity, 75);
}
