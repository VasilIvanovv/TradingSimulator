#pragma once

#include "ApiTypes.hpp"
#include "ExecutionReceipt.hpp"
#include "PriceCandle.hpp"
#include "SqliteAccountManager.hpp"
#include "TradingController.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

/// Factory that creates a TradingController for one account.
using ControllerFactory =
    std::function<std::unique_ptr<TradingController>(int accountId)>;

class ApiHandler {
public:
    ApiHandler(ControllerFactory factory, SqliteAccountManager& accountManager);

    // --- Account management ---
    std::vector<AccountInfo> listAccounts(int userId);
    std::optional<AccountInfo> createAccount(int userId, const std::string& name);
    bool renameAccount(int userId, int accountId, const std::string& newName);
    bool deleteAccount(int userId, int accountId);

    // --- Deposit ---
    void             deposit(int userId, int accountId, double amount);
    DepositAllResult depositAll(int userId, double amount);

    // --- Trading (all require userId to own accountId) ---
    AccountSnapshot  getAccount(int userId, int accountId);
    ExecutionReceipt placeOrder(int userId, int accountId, const PlaceOrderRequest& req);
    void             addRule(int userId, int accountId, const AddRuleRequest& req);
    void             removeRules(int userId, int accountId, const std::string& symbol);
    std::vector<ActiveRule> getRules(int userId, int accountId);
    std::optional<std::vector<PriceCandle>> getHistory(int userId, int accountId,
                                                        const GetHistoryRequest& req);

private:
    TradingController& getOrCreate(int userId, int accountId);

    ControllerFactory     m_factory;
    SqliteAccountManager& m_accountManager;
    std::mutex            m_mutex;
    std::unordered_map<int, std::unique_ptr<TradingController>> m_controllers; // keyed by accountId
};

} // namespace trading
