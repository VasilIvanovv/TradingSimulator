#pragma once

#include "ExecutionReceipt.hpp"
#include "OrderTicket.hpp"

namespace trading {

class IBrokerExecution {
public:
    virtual ~IBrokerExecution() = default;
    virtual ExecutionReceipt executeOrder(const OrderTicket& ticket) = 0;
    virtual double getAvailableCash() const = 0;
};

} // namespace trading
