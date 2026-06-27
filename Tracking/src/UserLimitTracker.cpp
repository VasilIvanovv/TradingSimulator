#include "UserLimitTracker.hpp"

namespace trading {

UserLimitTracker::UserLimitTracker(double triggerPrice, OrderSide side, double quantity)
    : m_triggerPrice(triggerPrice), m_side(side), m_quantity(quantity) {}

std::optional<OrderTicket> UserLimitTracker::evaluate(std::string_view symbol,
                                                       const std::vector<PriceCandle>& history) {
    if (m_fired || history.empty())
        return std::nullopt;

    const auto& latest    = history.back();
    const bool  triggered = (m_side == OrderSide::Buy) ? latest.close <= m_triggerPrice
                                                        : latest.close >= m_triggerPrice;
    if (!triggered)
        return std::nullopt;

    m_fired = true;
    return OrderTicket{ std::string(symbol), m_side, m_quantity, latest.close, latest.timestamp };
}

} // namespace trading
