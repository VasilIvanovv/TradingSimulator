#pragma once

#include "ExecutionReceipt.hpp"
#include "OrderTicket.hpp"
#include "TradeRecord.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

class IBrokerAccount {
public:
    virtual ~IBrokerAccount() = default;
    virtual ExecutionReceipt executeOrder(const OrderTicket& ticket) = 0;
    virtual double getAvailableCash() const = 0;
    virtual double getPosition(const std::string& symbol) const = 0;
    virtual std::unordered_map<std::string, double> getAllPositions() const = 0;
    virtual const std::vector<TradeRecord>& getTradeHistory() const = 0;
};

} // namespace trading
