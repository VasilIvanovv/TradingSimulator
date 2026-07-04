#include <gtest/gtest.h>
#include "MarketWatcher.hpp"

using namespace trading;

static PriceSource makeSource(double close, std::string timestamp = "2025-01-15") {
    return [close, timestamp](std::string_view) -> std::vector<PriceCandle> {
        PriceCandle c;
        c.close     = close;
        c.timestamp = timestamp;
        return { c };
    };
}

// Always places a buy — simulates an algorithmic engine for a fixed symbol.
class AlwaysBuyEngine : public IDecisionEngine {
public:
    explicit AlwaysBuyEngine(std::string symbol) : m_symbol(std::move(symbol)) {}

    std::vector<OrderTicket> evaluate(const PriceSource& priceSource) override {
        auto history = priceSource(m_symbol);
        if (history.empty()) return {};
        return { OrderTicket{ m_symbol, OrderSide::Buy, 1.0,
                              history.back().close, history.back().timestamp } };
    }

private:
    std::string m_symbol;
};

class MarketWatcherTest : public ::testing::Test {
protected:
    std::vector<OrderTicket> captured;

    // Callback that always reports a successful fill.
    MarketWatcher makeWatcher(double close) {
        return MarketWatcher(
            makeSource(close),
            [this](const OrderTicket& t) { captured.push_back(t); return true; }
        );
    }
};

TEST_F(MarketWatcherTest, Tick_FiresCallbackWhenRuleTriggered) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].symbol, "AAPL");
    EXPECT_EQ(captured[0].side,   OrderSide::Buy);
}

TEST_F(MarketWatcherTest, Tick_NoCallbackWhenRuleNotTriggered) {
    auto watcher = makeWatcher(155.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    EXPECT_TRUE(captured.empty());
}

TEST_F(MarketWatcherTest, Tick_RuleConsumedAfterFill) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    watcher.tick();
    EXPECT_EQ(captured.size(), 1u); // fired only once
}

TEST_F(MarketWatcherTest, Tick_RuleRemovedAfterRejection) {
    std::vector<OrderTicket> captured2;
    MarketWatcher watcher(
        makeSource(148.0),
        [&](const OrderTicket& t) { captured2.push_back(t); return false; } // always reject
    );
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    watcher.tick();
    EXPECT_EQ(captured2.size(), 1u); // fired once, then removed
}

TEST_F(MarketWatcherTest, Tick_RejectionCallbackFired) {
    std::vector<OrderTicket> rejections;
    MarketWatcher watcher(
        makeSource(148.0),
        [](const OrderTicket&) { return false; },
        [&](const OrderTicket& t) { rejections.push_back(t); }
    );
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    ASSERT_EQ(rejections.size(), 1u);
    EXPECT_EQ(rejections[0].symbol, "AAPL");
}

TEST_F(MarketWatcherTest, Tick_MultipleRulesSameSymbol_AllEvaluated) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.addRule("AAPL", 155.0, OrderSide::Buy,  5.0);
    watcher.tick();
    EXPECT_EQ(captured.size(), 2u);
}

TEST_F(MarketWatcherTest, Tick_MultipleSymbols_AllEvaluated) {
    MarketWatcher watcher(
        [](std::string_view symbol) -> std::vector<PriceCandle> {
            PriceCandle c;
            c.close     = (symbol == "AAPL") ? 148.0 : 305.0;
            c.timestamp = "2025-01-15";
            return { c };
        },
        [this](const OrderTicket& t) { captured.push_back(t); return true; }
    );
    watcher.addRule("AAPL", 150.0, OrderSide::Buy,  10.0);
    watcher.addRule("MSFT", 300.0, OrderSide::Sell,  5.0);
    watcher.tick();
    ASSERT_EQ(captured.size(), 2u);
}

TEST_F(MarketWatcherTest, AddEngine_FiresCallbackFromAlgorithmicEngine) {
    auto watcher = makeWatcher(148.0);
    watcher.addEngine(std::make_unique<AlwaysBuyEngine>("AAPL"));
    watcher.tick();
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].symbol, "AAPL");
}

TEST_F(MarketWatcherTest, AddRule_AndEngine_BothEvaluated) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.addEngine(std::make_unique<AlwaysBuyEngine>("AAPL"));
    watcher.tick();
    EXPECT_EQ(captured.size(), 2u);
}

TEST_F(MarketWatcherTest, RemoveRules_StopsEvaluatingSymbol) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.removeRules("AAPL");
    watcher.tick();
    EXPECT_TRUE(captured.empty());
}

TEST_F(MarketWatcherTest, AddRule_AfterRemove_Works) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.removeRules("AAPL");
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    EXPECT_EQ(captured.size(), 1u);
}
