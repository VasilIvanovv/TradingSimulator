#include <gtest/gtest.h>
#include "PaperAccount.hpp"

using namespace trading;

class PaperAccountTest : public ::testing::Test {
protected:
    PaperAccount account{ 10000.0 };
};

TEST_F(PaperAccountTest, GetAvailableCash_ReturnsInitialCash) {
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 10000.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Buy_ReducesCashAndUpdatesPosition) {
    const auto receipt = account.executeOrder({ "AAPL", OrderSide::Buy, 10.0, 150.0 });
    EXPECT_EQ(receipt.status, ExecutionStatus::Filled);
    EXPECT_DOUBLE_EQ(receipt.executionPrice, 150.0);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 8500.0);
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 10.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Buy_RejectsWhenInsufficientCash) {
    const auto receipt = account.executeOrder({ "AAPL", OrderSide::Buy, 10.0, 1500.0 });
    EXPECT_EQ(receipt.status, ExecutionStatus::Rejected);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 10000.0);
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 0.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Sell_IncreasesCashAndReducesPosition) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 10.0, 150.0 });
    const auto receipt = account.executeOrder({ "AAPL", OrderSide::Sell, 5.0, 200.0 });
    EXPECT_EQ(receipt.status, ExecutionStatus::Filled);
    EXPECT_DOUBLE_EQ(receipt.executionPrice, 200.0);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 9500.0);
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 5.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Sell_RejectsWhenNoPosition) {
    const auto receipt = account.executeOrder({ "AAPL", OrderSide::Sell, 5.0, 200.0 });
    EXPECT_EQ(receipt.status, ExecutionStatus::Rejected);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 10000.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Sell_RejectsWhenInsufficientShares) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 3.0, 150.0 });
    const auto receipt = account.executeOrder({ "AAPL", OrderSide::Sell, 5.0, 200.0 });
    EXPECT_EQ(receipt.status, ExecutionStatus::Rejected);
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 3.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_Sell_ClearsPositionWhenAllSharesSold) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 150.0 });
    account.executeOrder({ "AAPL", OrderSide::Sell, 5.0, 200.0 });
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 0.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_MultipleBuys_AccumulatesPosition) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 100.0 });
    account.executeOrder({ "AAPL", OrderSide::Buy, 3.0, 100.0 });
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 8.0);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 9200.0);
}

TEST_F(PaperAccountTest, ExecuteOrder_MultipleSymbols_TracksIndependently) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 100.0 });
    account.executeOrder({ "MSFT", OrderSide::Buy, 3.0, 200.0 });
    EXPECT_DOUBLE_EQ(account.getPosition("AAPL"), 5.0);
    EXPECT_DOUBLE_EQ(account.getPosition("MSFT"), 3.0);
    EXPECT_DOUBLE_EQ(account.getAvailableCash(), 8900.0);
}

TEST_F(PaperAccountTest, GetAllPositions_ReturnsAllOpenPositions) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 100.0 });
    account.executeOrder({ "MSFT", OrderSide::Buy, 3.0, 200.0 });
    const auto positions = account.getAllPositions();
    ASSERT_EQ(positions.size(), 2u);
    EXPECT_DOUBLE_EQ(positions.at("AAPL"), 5.0);
    EXPECT_DOUBLE_EQ(positions.at("MSFT"), 3.0);
}

TEST_F(PaperAccountTest, GetAllPositions_ExcludesFullySoldPositions) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 100.0 });
    account.executeOrder({ "AAPL", OrderSide::Sell, 5.0, 100.0 });
    EXPECT_TRUE(account.getAllPositions().empty());
}

TEST_F(PaperAccountTest, GetTradeHistory_EmptyInitially) {
    EXPECT_TRUE(account.getTradeHistory().empty());
}

TEST_F(PaperAccountTest, GetTradeHistory_RecordsFilledOrders) {
    account.executeOrder({ "AAPL", OrderSide::Buy,  5.0, 100.0 });
    account.executeOrder({ "AAPL", OrderSide::Sell, 3.0, 120.0 });
    const auto& history = account.getTradeHistory();
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].ticket.symbol, "AAPL");
    EXPECT_EQ(history[0].ticket.side,   OrderSide::Buy);
    EXPECT_DOUBLE_EQ(history[0].receipt.executionPrice, 100.0);
    EXPECT_EQ(history[1].ticket.side, OrderSide::Sell);
    EXPECT_DOUBLE_EQ(history[1].receipt.executionPrice, 120.0);
}

TEST_F(PaperAccountTest, GetTradeHistory_DoesNotRecordRejectedOrders) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 10.0, 9999.0 }); // rejected — not enough cash
    EXPECT_TRUE(account.getTradeHistory().empty());
}

TEST_F(PaperAccountTest, GetTradeHistory_PreservesTimestamp) {
    account.executeOrder({ "AAPL", OrderSide::Buy, 5.0, 100.0, "2025-01-15" });
    EXPECT_EQ(account.getTradeHistory()[0].ticket.timestamp, "2025-01-15");
}
