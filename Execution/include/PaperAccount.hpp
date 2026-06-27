#pragma once

#include "IBrokerAccount.hpp"

namespace trading {

class PaperAccount : public IBrokerAccount {
public:
    explicit PaperAccount(double initialCash);

    ExecutionReceipt executeOrder(const OrderTicket& ticket) override;
    double getAvailableCash() const override;
    double getPosition(const std::string& symbol) const override;
    std::unordered_map<std::string, double> getAllPositions() const override;
    const std::vector<TradeRecord>& getTradeHistory() const override;

private:
    double m_cash{};
    std::unordered_map<std::string, double> m_positions;
    std::vector<TradeRecord> m_tradeHistory;
};

} // namespace trading
