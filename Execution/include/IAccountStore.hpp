#pragma once

#include "TradeRecord.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace trading {

class IAccountStore {
public:
    virtual ~IAccountStore() = default;

    // Populates outCash/outPositions/outTrades from persisted state.
    // Returns false if no state has been saved yet (first run).
    virtual bool load(double& outCash,
                      std::unordered_map<std::string, double>& outPositions,
                      std::vector<TradeRecord>& outTrades) = 0;

    // Atomically persists the new cash balance, the updated quantity for one
    // symbol (0.0 means the position was fully closed), and the filled trade.
    virtual void persist(double cash,
                         const std::string& symbol,
                         double newPosition,
                         const TradeRecord& trade) = 0;
};

} // namespace trading
