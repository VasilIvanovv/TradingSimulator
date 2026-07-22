#pragma once

#include "ApiTypes.hpp"
#include "ExecutionReceipt.hpp"
#include "PriceCandle.hpp"
#include "TradingController.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

/// Factory that creates a TradingController for one user. Called at most once
/// per user, on the first authenticated request that user makes.
using ControllerFactory =
    std::function<std::unique_ptr<TradingController>(int userId)>;

/**
 * @brief Transport-agnostic request handler — multi-user edition.
 *
 * Holds a map of per-user @c TradingController instances created on demand
 * via the injected factory. All public methods take an authenticated @p userId
 * extracted from the JWT by the transport layer.
 *
 * Thread safety: controller creation is serialized with a mutex. Individual
 * method calls on an already-created controller are not serialized — callers
 * must not issue concurrent requests for the same user.
 *
 * To add a new transport (WebSocket, gRPC, …), create a new transport class
 * that holds an @c ApiHandler& and calls these methods directly.
 */
class ApiHandler {
public:
    /**
     * @param factory Callable invoked once per new user to create their
     *                TradingController. Must be thread-safe.
     */
    explicit ApiHandler(ControllerFactory factory);

    /** @brief Return a snapshot of @p userId's account state. */
    AccountSnapshot getAccount(int userId);

    /** @brief Place a manual order for @p userId. */
    ExecutionReceipt placeOrder(int userId, const PlaceOrderRequest& req);

    /** @brief Register a limit rule for @p userId. */
    void addRule(int userId, const AddRuleRequest& req);

    /** @brief Remove all limit rules for @p userId on @p symbol. */
    void removeRules(int userId, const std::string& symbol);

    /** @brief Fetch candle history (not user-specific; shared market data). */
    std::optional<std::vector<PriceCandle>> getHistory(int userId,
                                                        const GetHistoryRequest& req);

private:
    TradingController& getOrCreate(int userId);

    ControllerFactory m_factory;
    std::mutex        m_mutex;
    std::unordered_map<int, std::unique_ptr<TradingController>> m_controllers;
};

} // namespace trading
