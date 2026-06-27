#pragma once

#include "IDecisionEngine.hpp"
#include "OrderTicket.hpp"
#include "PriceCandle.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace trading {

/**
 * Polls price data on a fixed interval and evaluates registered rules and
 * algorithmic engines. Fires an order callback whenever a rule or engine
 * triggers. The polling loop starts automatically when the first rule/engine
 * is added and stops when the last one is removed.
 */
class MarketWatcher {
public:
    /** Callback invoked to fetch the price history for a given symbol. */
    using PriceSource   = std::function<std::vector<PriceCandle>(std::string_view)>;

    /** Callback invoked with a filled-in OrderTicket whenever a rule or engine triggers. */
    using OrderCallback = std::function<void(OrderTicket)>;

    /**
     * @param priceSource      Called each tick to retrieve candle history per symbol.
     * @param onOrderTriggered Called with the resulting OrderTicket when a rule fires.
     */
    MarketWatcher(PriceSource priceSource, OrderCallback onOrderTriggered);
    ~MarketWatcher();

    /**
     * Register a user-defined limit rule for @p symbol.
     * Triggers a one-shot order when the latest close crosses @p triggerPrice.
     * Buy rules trigger when close <= triggerPrice; sell rules when close >= triggerPrice.
     *
     * @param triggerPrice Price level at which to trigger the order.
     * @param side         Whether to buy or sell when triggered.
     * @param quantity     Number of shares to order.
     */
    void addRule(const std::string& symbol, double triggerPrice, OrderSide side, double quantity);

    /**
     * Register an algorithmic decision engine for @p symbol.
     * The engine is evaluated each tick and may produce orders according to its own logic.
     */
    void addEngine(const std::string& symbol, std::unique_ptr<IDecisionEngine> engine);

    /**
     * Remove all rules and engines registered for @p symbol.
     * Stops the polling loop if no rules or engines remain.
     */
    void removeRules(const std::string& symbol);

    /** Set the interval between polling ticks. Takes effect on the next sleep. */
    void setInterval(std::chrono::seconds interval);

    /**
     * Run one evaluation pass immediately — fetches prices for all registered
     * symbols and evaluates every rule and engine. Used for testing and manual control.
     */
    void tick();

private:
    void insertEngine(const std::string& symbol, std::unique_ptr<IDecisionEngine> engine);
    void startLoop();
    void stopLoop();
    void runLoop();

    PriceSource   m_priceSource;
    OrderCallback m_orderCallback;
    std::chrono::seconds m_interval{ 60 };
    std::unordered_map<std::string, std::vector<std::unique_ptr<IDecisionEngine>>> m_engines;
    mutable std::mutex      m_mutex;
    std::condition_variable m_cv;
    std::thread             m_thread;
    std::atomic<bool>       m_running{ false };
};

} // namespace trading
