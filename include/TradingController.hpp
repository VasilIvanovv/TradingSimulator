#pragma once

#include "DataBroker.hpp"
#include "ExecutionReceipt.hpp"
#include "IBrokerAccount.hpp"
#include "MarketWatcher.hpp"
#include "PriceCandle.hpp"
#include "TradeRecord.hpp"
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

/**
 * Wires the MarketData, Tracking, and Execution layers into a single facade.
 * The MarketWatcher polls on a fixed interval; triggered orders are routed
 * through the account automatically. Direct orders can also be placed via
 * placeOrder().
 */
class TradingController {
  public:
    /**
     * @param account       Broker account used to execute triggered and manual
     * orders.
     * @param broker        Data broker that supplies candle history.
     * @param dataInterval  Candle width passed to the broker (e.g. "1day").
     * @param startDate     Earliest history date, ISO 8601 (e.g. "2020-01-01").
     * @param pollInterval  How often the watcher evaluates rules and engines.
     */
    TradingController(std::unique_ptr<IBrokerAccount> account,
                      std::unique_ptr<DataBroker> broker,
                      std::string dataInterval, std::string startDate,
                      std::chrono::seconds pollInterval = std::chrono::seconds{
                          60});

    // --- Tracking ---
    void addLimitRule(const std::string &symbol, double triggerPrice,
                      OrderSide side, double quantity);
    void addEngine(std::unique_ptr<IDecisionEngine> engine);
    void removeRules(const std::string &symbol);

    // --- Account state ---
    double getAvailableCash() const;
    std::unordered_map<std::string, double> getAllPositions() const;
    const std::vector<TradeRecord> &getTradeHistory() const;

    // --- Manual execution ---
    ExecutionReceipt placeOrder(const OrderTicket &ticket);

    // --- Market data ---
    std::optional<std::vector<PriceCandle>> getHistory(const std::string& symbol,
                                                        const std::string& interval,
                                                        const std::string& startDate);

  private:
    std::unique_ptr<IBrokerAccount> m_account;
    std::unique_ptr<DataBroker> m_broker;
    std::string m_dataInterval;
    std::string m_startDate;
    MarketWatcher m_watcher;
};

} // namespace trading
