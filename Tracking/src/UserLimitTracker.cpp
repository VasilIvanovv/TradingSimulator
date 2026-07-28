#include "UserLimitTracker.hpp"

namespace trading {

void UserLimitTracker::addRule(const std::string &symbol, double triggerPrice,
                               OrderSide side, double quantity) {
    m_rules[symbol].push_back({triggerPrice, side, quantity});
}

void UserLimitTracker::removeRules(const std::string &symbol) {
    m_rules.erase(symbol);
}

bool UserLimitTracker::hasRules() const { return !m_rules.empty(); }

std::vector<UserLimitTracker::RuleEntry> UserLimitTracker::getAllRules() const {
    std::vector<RuleEntry> out;
    for (const auto& [symbol, rules] : m_rules)
        for (const auto& r : rules)
            out.push_back({symbol, r.triggerPrice, r.side, r.quantity});
    return out;
}

std::vector<OrderTicket>
UserLimitTracker::evaluate(const PriceSource& priceSource) {
    std::vector<OrderTicket> tickets;
    for (auto& [symbol, rules] : m_rules) {
        const auto history = priceSource(symbol);
        if (history.empty()) continue;
        const auto& latest = history.back();
        std::erase_if(rules, [&](const LimitRule& rule) {
            const bool hit = (rule.side == OrderSide::Buy) ? latest.close <= rule.triggerPrice
                                                           : latest.close >= rule.triggerPrice;
            if (hit)
                tickets.push_back({ symbol, rule.side, rule.quantity,
                                    latest.close, latest.timestamp });
            return hit;
        });
    }
    std::erase_if(m_rules, [](const auto& kv) { return kv.second.empty(); });
    return tickets;
}

} // namespace trading
