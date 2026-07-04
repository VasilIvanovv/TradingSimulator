#pragma once

#include "OrderTicket.hpp"
#include "PriceCandle.hpp"
#include <functional>
#include <string_view>
#include <vector>

namespace trading {

/** Callback that fetches candle history for a given symbol. */
using PriceSource = std::function<std::vector<PriceCandle>(std::string_view)>;

/**
 * Interface for algorithmic decision engines.
 * Each engine is responsible for fetching whatever price data it needs
 * via the supplied PriceSource and returning any orders it wants to place.
 */
class IDecisionEngine {
public:
    virtual ~IDecisionEngine() = default;
    virtual std::vector<OrderTicket> evaluate(const PriceSource& priceSource) = 0;
};

} // namespace trading
