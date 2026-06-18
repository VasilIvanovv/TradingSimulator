#pragma once

#include "IBrokerExecution.hpp"
#include <string>
#include <unordered_map>

namespace trading {

class PaperAccount : public IBrokerExecution {
public:
    explicit PaperAccount(double initialCash);

    ExecutionReceipt executeOrder(const OrderTicket& ticket) override;
    double getAvailableCash() const override;

    double getPosition(const std::string& symbol) const;

private:
    double m_cash{};
    std::unordered_map<std::string, double> m_positions;
};

} // namespace trading
