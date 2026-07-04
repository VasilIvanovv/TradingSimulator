#include <gtest/gtest.h>
#include "UserLimitTracker.hpp"

using namespace trading;

static PriceSource makeSource(double close, std::string timestamp = "2025-01-15") {
    return [close, timestamp](std::string_view) -> std::vector<PriceCandle> {
        PriceCandle c;
        c.close     = close;
        c.timestamp = timestamp;
        return { c };
    };
}

TEST(UserLimitTrackerTest, Buy_TriggersWhenCloseAtOrBelowLimit) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    auto tickets = tracker.evaluate(makeSource(150.0));
    ASSERT_EQ(tickets.size(), 1u);
    EXPECT_EQ(tickets[0].symbol,        "AAPL");
    EXPECT_EQ(tickets[0].side,          OrderSide::Buy);
    EXPECT_DOUBLE_EQ(tickets[0].price,    150.0);
    EXPECT_DOUBLE_EQ(tickets[0].quantity, 10.0);
    EXPECT_EQ(tickets[0].timestamp,     "2025-01-15");
}

TEST(UserLimitTrackerTest, Buy_TriggersWhenCloseBelowLimit) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    EXPECT_EQ(tracker.evaluate(makeSource(148.0)).size(), 1u);
}

TEST(UserLimitTrackerTest, Buy_DoesNotTriggerWhenCloseAboveLimit) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    EXPECT_TRUE(tracker.evaluate(makeSource(151.0)).empty());
}

TEST(UserLimitTrackerTest, Sell_TriggersWhenCloseAtOrAboveLimit) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 200.0, OrderSide::Sell, 5.0);
    auto tickets = tracker.evaluate(makeSource(200.0));
    ASSERT_EQ(tickets.size(), 1u);
    EXPECT_EQ(tickets[0].side, OrderSide::Sell);
}

TEST(UserLimitTrackerTest, Sell_DoesNotTriggerWhenCloseBelowLimit) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 200.0, OrderSide::Sell, 5.0);
    EXPECT_TRUE(tracker.evaluate(makeSource(199.0)).empty());
}

TEST(UserLimitTrackerTest, RuleRemoved_AfterTrigger) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    ASSERT_EQ(tracker.evaluate(makeSource(148.0)).size(), 1u);
    EXPECT_TRUE(tracker.evaluate(makeSource(148.0)).empty());
}

TEST(UserLimitTrackerTest, EmptyHistory_DoesNotTrigger) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    PriceSource emptySource = [](std::string_view) { return std::vector<PriceCandle>{}; };
    EXPECT_TRUE(tracker.evaluate(emptySource).empty());
}

TEST(UserLimitTrackerTest, MultipleRulesSameSymbol_AllEvaluated) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    tracker.addRule("AAPL", 155.0, OrderSide::Buy,  5.0);
    EXPECT_EQ(tracker.evaluate(makeSource(148.0)).size(), 2u);
}

TEST(UserLimitTrackerTest, MultipleSymbols_EachFetchedIndependently) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy,  10.0);
    tracker.addRule("MSFT", 300.0, OrderSide::Sell,  5.0);

    PriceSource source = [](std::string_view symbol) -> std::vector<PriceCandle> {
        PriceCandle c;
        c.close     = (symbol == "AAPL") ? 148.0 : 305.0;
        c.timestamp = "2025-01-15";
        return { c };
    };

    ASSERT_EQ(tracker.evaluate(source).size(), 2u);
}

TEST(UserLimitTrackerTest, RemoveRules_StopsTracking) {
    UserLimitTracker tracker;
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    tracker.removeRules("AAPL");
    EXPECT_TRUE(tracker.evaluate(makeSource(148.0)).empty());
}

TEST(UserLimitTrackerTest, HasRules_ReflectsState) {
    UserLimitTracker tracker;
    EXPECT_FALSE(tracker.hasRules());
    tracker.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    EXPECT_TRUE(tracker.hasRules());
    tracker.removeRules("AAPL");
    EXPECT_FALSE(tracker.hasRules());
}
