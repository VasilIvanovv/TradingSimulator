#pragma once

#include "ApiTypes.hpp"
#include "ExecutionReceipt.hpp"
#include "PriceCandle.hpp"
#include "TradingController.hpp"
#include <optional>
#include <string>
#include <vector>

namespace trading {

/**
 * @brief Transport-agnostic request handler.
 *
 * Translates typed request structs into @c TradingController calls and returns
 * typed results. Has no knowledge of HTTP, WebSocket, or any serialization
 * format — those concerns belong to the transport and JSON layers respectively.
 *
 * To add a new transport (WebSocket, gRPC, …), create a new transport class
 * that holds an @c ApiHandler& and calls these methods directly.
 */
class ApiHandler {
public:
    /**
     * @param controller The wired-up controller that owns all three layers.
     *                   Must outlive this handler.
     */
    explicit ApiHandler(TradingController& controller);

    /**
     * @brief Return a snapshot of the current account state.
     * @return Cash balance, open positions, and full trade history.
     */
    AccountSnapshot getAccount() const;

    /**
     * @brief Place a manual order immediately, bypassing the MarketWatcher.
     * @param req Order parameters.
     * @return Execution receipt indicating whether the order was filled or rejected.
     */
    ExecutionReceipt placeOrder(const PlaceOrderRequest& req);

    /**
     * @brief Register a one-shot limit rule on the MarketWatcher.
     * The rule is removed automatically after it fires (filled or rejected).
     * @param req Rule parameters.
     */
    void addRule(const AddRuleRequest& req);

    /**
     * @brief Remove all limit rules registered for @p symbol.
     * @param symbol Ticker whose rules should be cleared.
     */
    void removeRules(const std::string& symbol);

    /**
     * @brief Fetch candle history for one symbol through the DataBroker.
     * @param req Symbol, interval, and start date.
     * @return Candles sorted ascending by timestamp, or @c std::nullopt if
     *         no provider returned data.
     */
    std::optional<std::vector<PriceCandle>> getHistory(const GetHistoryRequest& req);

private:
    TradingController& m_controller;
};

} // namespace trading
