#pragma once

#include "OrderSide.hpp"
#include <string>

namespace trading {

struct OrderTicket {
    std::string symbol;
    OrderSide side{};
    double quantity{};
    double price{};
};

} // namespace trading
