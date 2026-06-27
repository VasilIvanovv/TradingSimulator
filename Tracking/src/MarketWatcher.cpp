#include "MarketWatcher.hpp"
#include "UserLimitTracker.hpp"

namespace trading {

MarketWatcher::MarketWatcher(PriceSource priceSource, OrderCallback onOrderTriggered)
    : m_priceSource(std::move(priceSource))
    , m_orderCallback(std::move(onOrderTriggered)) {}

MarketWatcher::~MarketWatcher() {
    stopLoop();
}

void MarketWatcher::addRule(const std::string& symbol, double triggerPrice,
                            OrderSide side, double quantity) {
    insertEngine(symbol, std::make_unique<UserLimitTracker>(triggerPrice, side, quantity));
}

void MarketWatcher::addEngine(const std::string& symbol, std::unique_ptr<IDecisionEngine> engine) {
    insertEngine(symbol, std::move(engine));
}

void MarketWatcher::insertEngine(const std::string& symbol, std::unique_ptr<IDecisionEngine> engine) {
    bool shouldStart = false;
    {
        std::lock_guard lock(m_mutex);
        shouldStart = m_engines.empty();
        m_engines[symbol].push_back(std::move(engine));
    }
    if (shouldStart)
        startLoop();
}

void MarketWatcher::removeRules(const std::string& symbol) {
    bool shouldStop = false;
    {
        std::lock_guard lock(m_mutex);
        m_engines.erase(symbol);
        shouldStop = m_engines.empty();
    }
    if (shouldStop)
        stopLoop();
}

void MarketWatcher::setInterval(std::chrono::seconds interval) {
    std::lock_guard lock(m_mutex);
    m_interval = interval;
}

void MarketWatcher::tick() {
    std::vector<std::string> symbols;
    {
        std::lock_guard lock(m_mutex);
        for (const auto& [symbol, _] : m_engines)
            symbols.push_back(symbol);
    }

    for (const auto& symbol : symbols) {
        const auto history = m_priceSource(symbol);

        std::vector<OrderTicket> triggered;
        {
            std::lock_guard lock(m_mutex);
            auto it = m_engines.find(symbol);
            if (it == m_engines.end())
                continue;
            for (auto& engine : it->second)
                if (auto ticket = engine->evaluate(symbol, history))
                    triggered.push_back(*ticket);
        }

        for (const auto& ticket : triggered)
            m_orderCallback(ticket);
    }
}

void MarketWatcher::startLoop() {
    m_running = true;
    m_thread  = std::thread(&MarketWatcher::runLoop, this);
}

void MarketWatcher::stopLoop() {
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

// Sleep first so tests can call tick() directly without racing the loop thread.
void MarketWatcher::runLoop() {
    while (m_running) {
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait_for(lock, m_interval, [this] { return !m_running.load(); });
        }
        if (m_running)
            tick();
    }
}

} // namespace trading
