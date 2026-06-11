#include <gtest/gtest.h>
#include "MatchingEngine.hpp"
#include <vector>

TEST(CancelModifyTests, CancelOrderCorrectly) {
    MatchingEngine engine;
    std::vector<Trade> trades;

    Order buy{1, "Buyer1", Side::BUY, 100.0, 100, 100, 0, OrderStatus::NEW};
    engine.processOrder(buy, trades);

    // Verify it is resting in book
    const Order* resting = engine.getOrderBook().getOrder(1);
    ASSERT_NE(resting, nullptr);
    EXPECT_EQ(resting->remainingQuantity, 100);

    // Cancel order
    Order cancelledOrder;
    bool success = engine.cancelOrder(1, cancelledOrder);
    ASSERT_TRUE(success);
    EXPECT_EQ(cancelledOrder.orderId, 1);
    EXPECT_EQ(cancelledOrder.status, OrderStatus::CANCELLED);

    // Verify it is removed from lookup
    const Order* lookup = engine.getOrderBook().getOrder(1);
    EXPECT_EQ(lookup, nullptr);

    // Verify it is removed from bids depth
    auto depth = engine.getOrderBook().getBidDepth();
    EXPECT_TRUE(depth.empty());
}

TEST(CancelModifyTests, ModifyOrderKeepAndLosePriority) {
    MatchingEngine engine;
    std::vector<Trade> trades;

    // Place two BUY orders at same price (100.0)
    Order buy1{1, "Buyer1", Side::BUY, 100.0, 100, 100, 0, OrderStatus::NEW};
    engine.processOrder(buy1, trades);

    Order buy2{2, "Buyer2", Side::BUY, 100.0, 50, 50, 0, OrderStatus::NEW};
    engine.processOrder(buy2, trades);

    // 1. Modify Buy 1 down in quantity (100 -> 50) -> SHOULD keep priority!
    Order oldOrder, newOrder;
    bool priorityLost = false;
    bool success = engine.modifyOrder(1, 100.0, 50, oldOrder, newOrder, priorityLost);
    ASSERT_TRUE(success);
    EXPECT_FALSE(priorityLost);
    EXPECT_EQ(newOrder.quantity, 50);

    // Let's verify priority by placing a crossing sell order for 75 shares
    // Since Buy 1 kept priority, it should match 50 shares, and Buy 2 should match 25 shares.
    Order sell{3, "Seller", Side::SELL, 100.0, 75, 75, 0, OrderStatus::NEW};
    engine.processOrder(sell, trades);

    ASSERT_EQ(trades.size(), 2);
    // First match against Buy 1 (kept priority)
    EXPECT_EQ(trades[0].buyOrderId, 1);
    EXPECT_EQ(trades[0].quantity, 50);
    // Second match against Buy 2
    EXPECT_EQ(trades[1].buyOrderId, 2);
    EXPECT_EQ(trades[1].quantity, 25);
    
    // Clear trade history
    trades.clear();

    // 2. Modify Buy 2 up in quantity or change price -> SHOULD lose priority!
    // Buy 2 now has 25 shares remaining. Let's add Buy 4 to sit behind it.
    Order buy4{4, "Buyer4", Side::BUY, 100.0, 30, 30, 0, OrderStatus::NEW};
    engine.processOrder(buy4, trades); // queue: Buy 2 (25 shares), Buy 4 (30 shares)

    // Modify Buy 2's price to 99.0 and then back to 100.0, or modify quantity up.
    // Let's modify Buy 2's quantity up from 25 to 60. This must lose priority.
    success = engine.modifyOrder(2, 100.0, 60, oldOrder, newOrder, priorityLost);
    ASSERT_TRUE(success);
    EXPECT_TRUE(priorityLost); // Lost priority! Now queue is: Buy 4 (30 shares), Buy 2 (60 shares)

    // Place crossing sell order for 40 shares.
    // It should match 30 shares against Buy 4, and 10 shares against Buy 2.
    Order sell2{5, "Seller2", Side::SELL, 100.0, 40, 40, 0, OrderStatus::NEW};
    engine.processOrder(sell2, trades);

    ASSERT_EQ(trades.size(), 2);
    // First match is Buy 4 (took priority since Buy 2 lost it)
    EXPECT_EQ(trades[0].buyOrderId, 4);
    EXPECT_EQ(trades[0].quantity, 30);
    // Second match is Buy 2
    EXPECT_EQ(trades[1].buyOrderId, 2);
    EXPECT_EQ(trades[1].quantity, 10);
}
