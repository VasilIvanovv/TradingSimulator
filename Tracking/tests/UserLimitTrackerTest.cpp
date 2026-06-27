#include <gtest/gtest.h>
#include "UserLimitTracker.hpp"

using namespace trading;

static PriceCandle makeCandle(double close, std::string timestamp = "2025-01-15") {
    PriceCandle c;
    c.close     = close;
    c.timestamp = std::move(timestamp);
    return c;
}

TEST(UserLimitTrackerTest, Buy_TriggersWhenCloseAtOrBelowLimit) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    auto result = tracker.evaluate("AAPL", { makeCandle(150.0) });
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->side, OrderSide::Buy);
    EXPECT_DOUBLE_EQ(result->price, 150.0);
    EXPECT_DOUBLE_EQ(result->quantity, 10.0);
    EXPECT_EQ(result->symbol, "AAPL");
    EXPECT_EQ(result->timestamp, "2025-01-15");
}

TEST(UserLimitTrackerTest, Buy_TriggersWhenCloseBelowLimit) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    EXPECT_TRUE(tracker.evaluate("AAPL", { makeCandle(148.0) }).has_value());
}

TEST(UserLimitTrackerTest, Buy_DoesNotTriggerWhenCloseAboveLimit) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    EXPECT_FALSE(tracker.evaluate("AAPL", { makeCandle(151.0) }).has_value());
}

TEST(UserLimitTrackerTest, Sell_TriggersWhenCloseAtOrAboveLimit) {
    UserLimitTracker tracker(200.0, OrderSide::Sell, 5.0);
    auto result = tracker.evaluate("AAPL", { makeCandle(200.0) });
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->side, OrderSide::Sell);
}

TEST(UserLimitTrackerTest, Sell_DoesNotTriggerWhenCloseBelowLimit) {
    UserLimitTracker tracker(200.0, OrderSide::Sell, 5.0);
    EXPECT_FALSE(tracker.evaluate("AAPL", { makeCandle(199.0) }).has_value());
}

TEST(UserLimitTrackerTest, FiresOnlyOnce) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    EXPECT_TRUE(tracker.evaluate("AAPL",  { makeCandle(148.0) }).has_value());
    EXPECT_FALSE(tracker.evaluate("AAPL", { makeCandle(148.0) }).has_value());
}

TEST(UserLimitTrackerTest, EmptyHistory_ReturnsNullopt) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    EXPECT_FALSE(tracker.evaluate("AAPL", {}).has_value());
}

TEST(UserLimitTrackerTest, UsesLatestCandleOnly) {
    UserLimitTracker tracker(150.0, OrderSide::Buy, 10.0);
    // Earlier candles are above limit, only latest is below
    auto result = tracker.evaluate("AAPL", { makeCandle(200.0), makeCandle(148.0) });
    EXPECT_TRUE(result.has_value());
}
