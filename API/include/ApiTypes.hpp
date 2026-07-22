#pragma once

#include "OrderSide.hpp"
#include "PriceCandle.hpp"
#include "TradeRecord.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

// ---------------------------------------------------------------------------
// Requests (client → server)
// ---------------------------------------------------------------------------

/**
 * @brief Request to place a manual order immediately.
 * Sent as the body of @c POST /orders.
 */
struct PlaceOrderRequest {
    std::string symbol;    ///< Ticker symbol, e.g. "AAPL".
    OrderSide   side{};    ///< Buy or sell.
    double      quantity{};///< Number of shares.
    double      price{};   ///< Limit price per share.
    std::string timestamp; ///< Optional ISO 8601 timestamp; defaults to empty.
};

/**
 * @brief Request to register a one-shot limit rule on the MarketWatcher.
 * Sent as the body of @c POST /rules.
 * The rule fires — and is then removed — the first time the latest close
 * crosses @p triggerPrice in the direction implied by @p side.
 */
struct AddRuleRequest {
    std::string symbol;        ///< Ticker to watch.
    double      triggerPrice{};///< Price level that triggers the order.
    OrderSide   side{};        ///< Buy triggers when close <= triggerPrice; sell when close >= triggerPrice.
    double      quantity{};    ///< Shares to order when triggered.
};

/**
 * @brief Request to fetch candle history for one symbol.
 * Parameters are passed as query strings on @c GET /history.
 */
struct GetHistoryRequest {
    std::string symbol;   ///< Ticker symbol, e.g. "AAPL".
    std::string interval; ///< Candle width, e.g. "1day", "1h".
    std::string startDate;///< Earliest candle to return, ISO 8601, e.g. "2024-01-01".
};

// ---------------------------------------------------------------------------
// Auth requests / responses
// ---------------------------------------------------------------------------

/** @brief Body for @c POST /auth/register and @c POST /auth/login. */
struct RegisterRequest {
    std::string username;
    std::string password;
};

/** @brief Body for @c POST /auth/login. */
struct LoginRequest {
    std::string username;
    std::string password;
};

/** @brief Response for both auth endpoints. */
struct AuthResponse {
    std::string token; ///< Signed JWT. Send as: Authorization: Bearer <token>
};

// ---------------------------------------------------------------------------
// Responses (server → client)
// ---------------------------------------------------------------------------

/**
 * @brief A point-in-time snapshot of the paper account state.
 * Returned by @c GET /account.
 */
struct AccountSnapshot {
    double cash{};                                  ///< Available cash balance.
    std::unordered_map<std::string, double> positions; ///< Open positions: symbol → quantity.
    std::vector<TradeRecord> tradeHistory;          ///< All filled trades, oldest first.
};

} // namespace trading
