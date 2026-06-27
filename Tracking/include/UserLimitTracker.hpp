#pragma once

#include "IDecisionEngine.hpp"
#include "OrderSide.hpp"

namespace trading {

// Fires once when the latest close price crosses the trigger price.
// Buy:  triggers when close <= triggerPrice
// Sell: triggers when close >= triggerPrice
class UserLimitTracker : public IDecisionEngine {
public:
    UserLimitTracker(double triggerPrice, OrderSide side, double quantity);

    std::optional<OrderTicket> evaluate(std::string_view symbol,
                                        const std::vector<PriceCandle>& history) override;

private:
    double    m_triggerPrice;
    OrderSide m_side;
    double    m_quantity;
    bool      m_fired{ false };
};

} // namespace trading
