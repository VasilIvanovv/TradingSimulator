#include <gtest/gtest.h>
#include "MarketWatcher.hpp"

using namespace trading;

static std::vector<PriceCandle> makeHistory(double close, std::string timestamp = "2025-01-15") {
    PriceCandle c;
    c.close     = close;
    c.timestamp = std::move(timestamp);
    return { c };
}

// Minimal engine for testing addEngine — always triggers a buy.
class AlwaysBuyEngine : public IDecisionEngine {
public:
    std::optional<OrderTicket> evaluate(std::string_view symbol,
                                        const std::vector<PriceCandle>& history) override {
        if (history.empty()) return std::nullopt;
        return OrderTicket{ std::string(symbol), OrderSide::Buy, 1.0,
                            history.back().close, history.back().timestamp };
    }
};

class MarketWatcherTest : public ::testing::Test {
protected:
    std::vector<OrderTicket> captured;

    MarketWatcher makeWatcher(double close) {
        return MarketWatcher(
            [close](std::string_view) { return makeHistory(close); },
            [this](OrderTicket t) { captured.push_back(std::move(t)); }
        );
    }
};

TEST_F(MarketWatcherTest, Tick_FiresCallbackWhenRuleTriggered) {
    auto watcher = makeWatcher(148.0); // below buy limit of 150
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].symbol, "AAPL");
    EXPECT_EQ(captured[0].side, OrderSide::Buy);
}

TEST_F(MarketWatcherTest, Tick_NoCallbackWhenRuleNotTriggered) {
    auto watcher = makeWatcher(155.0); // above buy limit of 150
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.tick();
    EXPECT_TRUE(captured.empty());
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
        [](std::string_view symbol) {
            return makeHistory(symbol == "AAPL" ? 148.0 : 300.0);
        },
        [this](OrderTicket t) { captured.push_back(std::move(t)); }
    );
    watcher.addRule("AAPL", 150.0, OrderSide::Buy,  10.0);
    watcher.addRule("MSFT", 290.0, OrderSide::Sell,  5.0);
    watcher.tick();
    ASSERT_EQ(captured.size(), 2u);
}

TEST_F(MarketWatcherTest, AddEngine_FiresCallbackFromAlgorithmicEngine) {
    auto watcher = makeWatcher(148.0);
    watcher.addEngine("AAPL", std::make_unique<AlwaysBuyEngine>());
    watcher.tick();
    ASSERT_EQ(captured.size(), 1u);
    EXPECT_EQ(captured[0].symbol, "AAPL");
}

TEST_F(MarketWatcherTest, AddRule_AndEngine_BothEvaluated) {
    auto watcher = makeWatcher(148.0);
    watcher.addRule("AAPL", 150.0, OrderSide::Buy, 10.0);
    watcher.addEngine("AAPL", std::make_unique<AlwaysBuyEngine>());
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
