#pragma once

#include "OrderSide.hpp"
#include <string>

namespace trading {

struct OrderTicket {
    std::string symbol;
    OrderSide side{};
    double quantity{};
    double price{};
    std::string timestamp;
};

} // namespace trading
