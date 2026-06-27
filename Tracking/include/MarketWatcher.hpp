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

class MarketWatcher {
public:
    using PriceSource   = std::function<std::vector<PriceCandle>(std::string_view)>;
    using OrderCallback = std::function<void(OrderTicket)>;

    MarketWatcher(PriceSource priceSource, OrderCallback onOrderTriggered);
    ~MarketWatcher();

    // Add a user-defined limit rule for a symbol.
    void addRule(const std::string& symbol, double triggerPrice, OrderSide side, double quantity);

    // Add an algorithmic decision engine for a symbol.
    void addEngine(const std::string& symbol, std::unique_ptr<IDecisionEngine> engine);

    void removeRules(const std::string& symbol);
    void setInterval(std::chrono::seconds interval);

    // One evaluation pass — useful for testing and manual control.
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
