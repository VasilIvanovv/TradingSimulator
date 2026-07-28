#pragma once

#include "IBrokerAccount.hpp"
#include "IAccountStore.hpp"
#include <memory>

namespace trading {

class PaperAccount : public IBrokerAccount {
public:
    // store is optional; without it the account is in-memory only.
    explicit PaperAccount(double initialCash,
                          std::unique_ptr<IAccountStore> store = nullptr);

    ExecutionReceipt executeOrder(const OrderTicket& ticket) override;
    void deposit(double amount) override;
    double getAvailableCash() const override;
    double getPosition(const std::string& symbol) const override;
    std::unordered_map<std::string, double> getAllPositions() const override;
    const std::vector<TradeRecord>& getTradeHistory() const override;

private:
    std::unique_ptr<IAccountStore> m_store;
    double m_cash{};
    std::unordered_map<std::string, double> m_positions;
    std::vector<TradeRecord> m_tradeHistory;
};

} // namespace trading
