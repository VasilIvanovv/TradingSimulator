#pragma once

#include "OrderTicket.hpp"
#include "PriceCandle.hpp"
#include <optional>
#include <string_view>
#include <vector>

namespace trading {

class IDecisionEngine {
public:
    virtual ~IDecisionEngine() = default;
    virtual std::optional<OrderTicket> evaluate(std::string_view symbol,
                                                const std::vector<PriceCandle>& history) = 0;
};

} // namespace trading
