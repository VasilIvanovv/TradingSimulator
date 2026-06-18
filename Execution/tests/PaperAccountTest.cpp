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
