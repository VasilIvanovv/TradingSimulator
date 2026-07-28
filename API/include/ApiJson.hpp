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
 * @brief Serialize the symbol catalogue to JSON.
 * @return JSON array of { symbol, name, sector } objects.
 */
std::string toJson(const std::vector<SymbolInfo>& symbols);

/**
 * @brief Serialize active limit rules to JSON.
 * @return JSON array of { symbol, triggerPrice, side, quantity } objects.
 */
std::string toJson(const std::vector<ActiveRule>& rules);

/** @brief Serialize the account list to JSON. */
std::string toJson(const std::vector<AccountInfo>& accounts);

/** @brief Parse a deposit request body. Required field: amount. */
DepositRequest parseDepositRequest(const std::string& body);

/** @brief Parse a create/rename account request body. Required field: name. */
AccountNameRequest parseAccountNameRequest(const std::string& body);

/** @brief Serialize a deposit-all result to JSON. */
std::string toJson(const DepositAllResult& result);

/** @brief Serialize a single account info to JSON. */
std::string toJson(const AccountInfo& account);

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
