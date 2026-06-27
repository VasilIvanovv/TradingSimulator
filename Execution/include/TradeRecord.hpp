#pragma once

#include "ExecutionReceipt.hpp"
#include "OrderTicket.hpp"

namespace trading {

struct TradeRecord {
    OrderTicket ticket;
    ExecutionReceipt receipt;
};

} // namespace trading
