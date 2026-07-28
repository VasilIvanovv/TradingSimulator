#include "ApiHandler.hpp"

namespace trading {

ApiHandler::ApiHandler(ControllerFactory factory, SqliteAccountManager& accountManager)
    : m_factory(std::move(factory)), m_accountManager(accountManager) {}

TradingController& ApiHandler::getOrCreate(int userId, int accountId) {
    if (!m_accountManager.ownsAccount(userId, accountId))
        throw std::runtime_error("account not found");

    std::lock_guard lock(m_mutex);
    auto it = m_controllers.find(accountId);
    if (it == m_controllers.end())
        it = m_controllers.emplace(accountId, m_factory(accountId)).first;
    return *it->second;
}

// --- Account management ---

std::vector<AccountInfo> ApiHandler::listAccounts(int userId) {
    m_accountManager.ensureDefault(userId); // auto-create on first login
    auto raw = m_accountManager.listAccounts(userId);
    std::vector<AccountInfo> out;
    out.reserve(raw.size());
    for (const auto& a : raw)
        out.push_back({a.id, a.name});
    return out;
}

std::optional<AccountInfo> ApiHandler::createAccount(int userId, const std::string& name) {
    const auto id = m_accountManager.createAccount(userId, name);
    if (!id) return std::nullopt;
    return AccountInfo{*id, name};
}

bool ApiHandler::renameAccount(int userId, int accountId, const std::string& newName) {
    return m_accountManager.renameAccount(userId, accountId, newName);
}

bool ApiHandler::deleteAccount(int userId, int accountId) {
    {
        std::lock_guard lock(m_mutex);
        m_controllers.erase(accountId);
    }
    return m_accountManager.deleteAccount(userId, accountId);
}

// --- Deposit ---

void ApiHandler::deposit(int userId, int accountId, double amount) {
    getOrCreate(userId, accountId).deposit(amount);
}

DepositAllResult ApiHandler::depositAll(int userId, double amount) {
    DepositAllResult result;
    const auto accounts = m_accountManager.listAccounts(userId);
    for (const auto& a : accounts) {
        try {
            getOrCreate(userId, a.id).deposit(amount);
            result.succeeded.push_back(a.id);
        } catch (...) {
            result.failed.push_back(a.id);
        }
    }
    return result;
}

// --- Trading ---

AccountSnapshot ApiHandler::getAccount(int userId, int accountId) {
    auto& c = getOrCreate(userId, accountId);
    return {c.getAvailableCash(), c.getAllPositions(), c.getTradeHistory()};
}

ExecutionReceipt ApiHandler::placeOrder(int userId, int accountId,
                                         const PlaceOrderRequest& req) {
    return getOrCreate(userId, accountId).placeOrder(
        {req.symbol, req.side, req.quantity, req.price, req.timestamp});
}

void ApiHandler::addRule(int userId, int accountId, const AddRuleRequest& req) {
    getOrCreate(userId, accountId).addLimitRule(
        req.symbol, req.triggerPrice, req.side, req.quantity);
}

void ApiHandler::removeRules(int userId, int accountId, const std::string& symbol) {
    getOrCreate(userId, accountId).removeRules(symbol);
}

std::vector<ActiveRule> ApiHandler::getRules(int userId, int accountId) {
    auto raw = getOrCreate(userId, accountId).getAllLimitRules();
    std::vector<ActiveRule> out;
    out.reserve(raw.size());
    for (const auto& r : raw)
        out.push_back({r.symbol, r.triggerPrice, r.side, r.quantity});
    return out;
}

std::optional<std::vector<PriceCandle>> ApiHandler::getHistory(
    int userId, int accountId, const GetHistoryRequest& req) {
    return getOrCreate(userId, accountId).getHistory(
        req.symbol, req.interval, req.startDate);
}

} // namespace trading
