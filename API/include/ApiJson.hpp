#pragma once

#include "ApiTypes.hpp"
#include "ExecutionReceipt.hpp"
#include "PriceCandle.hpp"
#include <string>
#include <vector>

namespace trading {

/**
 * @defgroup ApiJson JSON serialization / deserialization
 * @brief Converts between raw JSON strings and typed API structs.
 *
 * All parsing functions throw @c std::runtime_error on malformed input so the
 * transport layer can catch and return an appropriate error response (e.g. 400).
 * @{
 */

/**
 * @brief Parse a @c POST /orders request body.
 * @param body UTF-8 JSON string. Required fields: symbol, side, quantity, price.
 *             Optional field: timestamp (defaults to empty string).
 * @throws std::runtime_error if any required field is missing or has an invalid type.
 */
PlaceOrderRequest parsePlaceOrderRequest(const std::string& body);

/**
 * @brief Parse a @c POST /rules request body.
 * @param body UTF-8 JSON string. Required fields: symbol, triggerPrice, side, quantity.
 * @throws std::runtime_error if any required field is missing or has an invalid type.
 */
AddRuleRequest parseAddRuleRequest(const std::string& body);

/**
 * @brief Serialize an account snapshot to JSON.
 * @return JSON object with keys: cash, positions, tradeHistory.
 */
std::string toJson(const AccountSnapshot& snapshot);

/**
 * @brief Serialize an execution receipt to JSON.
 * @return JSON object with keys: status ("filled" | "rejected"), executionPrice.
 */
std::string toJson(const ExecutionReceipt& receipt);

/**
 * @brief Serialize a candle list to JSON.
 * @return JSON object with key: candles (array of OHLCV objects).
 */
std::string toJson(const std::vector<PriceCandle>& candles);

/**
 * @brief Parse a @c POST /auth/register request body.
 * @param body UTF-8 JSON string. Required fields: username, password.
 */
RegisterRequest parseRegisterRequest(const std::string& body);

/**
 * @brief Parse a @c POST /auth/login request body.
 * @param body UTF-8 JSON string. Required fields: username, password.
 */
LoginRequest parseLoginRequest(const std::string& body);

/**
 * @brief Serialize an auth response (register or login) to JSON.
 * @return JSON object with key: token.
 */
std::string toJson(const AuthResponse& response);

/**
 * @brief Serialize an error message to JSON.
 * @return JSON object with key: error.
 */
std::string toJsonError(const std::string& message);

/** @} */

} // namespace trading
