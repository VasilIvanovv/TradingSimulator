#pragma once

#include "IDecisionEngine.hpp"
#include "OrderTicket.hpp"
#include "UserLimitTracker.hpp"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace trading {

/**
 * Polls price data on a fixed interval and evaluates registered limit rules
 * and algorithmic engines. Fires an order callback whenever any of them trigger.
 * The polling loop starts automatically when the first rule or engine is added
 * and stops when all are removed.
 */
class MarketWatcher {
public:
    /**
     * Callback invoked with an OrderTicket whenever a rule or engine triggers.
     * Must return true if the order was filled, false if it was rejected.
     * On rejection the rule is removed and onOrderRejected (if set) is called.
     */
    using OrderCallback = std::function<bool(const OrderTicket&)>;

    /**
     * Optional callback invoked when a limit rule is triggered but the execution
     * is rejected (e.g. insufficient funds or no position). The rule is removed
     * after this call regardless.
     */
    using RejectionCallback = std::function<void(const OrderTicket&)>;

    /**
     * @param priceSource      Called by engines and rules to retrieve candle history per symbol.
     * @param onOrderTriggered Called with the resulting OrderTicket when a rule fires.
     * @param onOrderRejected  Optional — called when an order is rejected so the caller
     *                         can notify the user. Rule is removed either way.
     */
    MarketWatcher(PriceSource priceSource, OrderCallback onOrderTriggered,
                  RejectionCallback onOrderRejected = nullptr);
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
     * Register an algorithmic decision engine.
     * The engine manages its own symbol subscriptions and fetches prices via PriceSource.
     */
    void addEngine(std::unique_ptr<IDecisionEngine> engine);

    /**
     * Remove all limit rules registered for @p symbol.
     * Stops the polling loop if no rules or engines remain.
     */
    void removeRules(const std::string& symbol);

    /** Return a snapshot of all currently pending limit rules. */
    std::vector<UserLimitTracker::RuleEntry> getAllRules() const;

    /** Set the interval between polling ticks. Takes effect on the next sleep. */
    void setInterval(std::chrono::seconds interval);

    /**
     * Run one evaluation pass immediately — evaluates all rules and engines.
     * Used for testing and manual control.
     */
    void tick();

private:
    bool isEmpty() const; // must be called with m_mutex held
    void startLoop();
    void stopLoop();
    void runLoop();

    PriceSource       m_priceSource;
    OrderCallback     m_orderCallback;
    RejectionCallback m_rejectionCallback;
    std::chrono::seconds m_interval{ 60 };
    UserLimitTracker m_limitTracker;
    std::vector<std::unique_ptr<IDecisionEngine>> m_engines;
    mutable std::mutex      m_mutex;
    std::condition_variable m_cv;
    std::thread             m_thread;
    std::atomic<bool>       m_running{ false };
};

} // namespace trading
