#pragma once

#include "IDecisionEngine.hpp"
#include "OrderSide.hpp"
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

/**
 * Manages all user-defined limit rules across any number of symbols.
 * Each rule fires once when the latest close price crosses its trigger price
 * and is removed immediately, regardless of whether the order fills or rejects.
 */
class UserLimitTracker {
public:
    struct RuleEntry {
        std::string symbol;
        double      triggerPrice;
        OrderSide   side;
        double      quantity;
    };

    void addRule(const std::string& symbol, double triggerPrice, OrderSide side, double quantity);
    void removeRules(const std::string& symbol);
    bool hasRules() const;
    std::vector<RuleEntry> getAllRules() const;

    /** Evaluate all rules, remove those that trigger, and return their tickets. */
    std::vector<OrderTicket> evaluate(const PriceSource& priceSource);

private:
    struct LimitRule {
        double    triggerPrice;
        OrderSide side;
        double    quantity;
    };

    std::unordered_map<std::string, std::vector<LimitRule>> m_rules;
};

} // namespace trading
