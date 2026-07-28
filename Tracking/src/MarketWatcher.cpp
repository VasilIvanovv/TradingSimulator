#include "MarketWatcher.hpp"

namespace trading {

MarketWatcher::MarketWatcher(PriceSource priceSource, OrderCallback onOrderTriggered,
                             RejectionCallback onOrderRejected)
    : m_priceSource(std::move(priceSource))
    , m_orderCallback(std::move(onOrderTriggered))
    , m_rejectionCallback(std::move(onOrderRejected)) {}

MarketWatcher::~MarketWatcher() { stopLoop(); }

void MarketWatcher::addRule(const std::string &symbol, double triggerPrice,
                            OrderSide side, double quantity) {
    bool shouldStart = false;
    {
        std::lock_guard lock(m_mutex);
        shouldStart = isEmpty();
        m_limitTracker.addRule(symbol, triggerPrice, side, quantity);
    }
    if (shouldStart)
        startLoop();
}

void MarketWatcher::addEngine(std::unique_ptr<IDecisionEngine> engine) {
    bool shouldStart = false;
    {
        std::lock_guard lock(m_mutex);
        shouldStart = isEmpty();
        m_engines.push_back(std::move(engine));
    }
    if (shouldStart)
        startLoop();
}

void MarketWatcher::removeRules(const std::string &symbol) {
    bool shouldStop = false;
    {
        std::lock_guard lock(m_mutex);
        m_limitTracker.removeRules(symbol);
        shouldStop = isEmpty();
    }
    if (shouldStop)
        stopLoop();
}

std::vector<UserLimitTracker::RuleEntry> MarketWatcher::getAllRules() const {
    std::lock_guard lock(m_mutex);
    return m_limitTracker.getAllRules();
}

void MarketWatcher::setInterval(std::chrono::seconds interval) {
    std::lock_guard lock(m_mutex);
    m_interval = interval;
}

void MarketWatcher::tick() {
    // Snapshot engine pointers under lock so we don't hold it during price
    // fetches.
    std::vector<IDecisionEngine *> engineSnapshot;
    {
        std::lock_guard lock(m_mutex);
        for (auto &e : m_engines)
            engineSnapshot.push_back(e.get());
    }

    // Limit rules are removed as soon as they trigger, fill or reject.
    for (const auto& ticket : m_limitTracker.evaluate(m_priceSource)) {
        if (!m_orderCallback(ticket) && m_rejectionCallback)
            m_rejectionCallback(ticket);
    }

    // Algorithmic engines: fire and let the engine manage its own state.
    for (auto* engine : engineSnapshot)
        for (const auto& ticket : engine->evaluate(m_priceSource))
            m_orderCallback(ticket);

    // Auto-stop if all limit rules were consumed and no engines remain.
    {
        std::lock_guard lock(m_mutex);
        if (isEmpty())
            m_running.store(false, std::memory_order_release);
    }
}

bool MarketWatcher::isEmpty() const {
    return !m_limitTracker.hasRules() && m_engines.empty();
}

void MarketWatcher::startLoop() {
    // Join a previously auto-stopped thread before spawning a new one.
    if (m_thread.joinable())
        m_thread.join();
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&MarketWatcher::runLoop, this);
}

void MarketWatcher::stopLoop() {
    m_running.store(false, std::memory_order_release);
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

// Sleep first so tests can call tick() directly without racing the loop thread.
void MarketWatcher::runLoop() {
    while (m_running.load(std::memory_order_acquire)) {
        {
            std::unique_lock lock(m_mutex);
            m_cv.wait_for(lock, m_interval,
                          [this] { return !m_running.load(std::memory_order_acquire); });
        }
        if (m_running.load(std::memory_order_acquire))
            tick();
    }
}

} // namespace trading
