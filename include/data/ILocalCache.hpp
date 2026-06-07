#pragma once

#include <span>
#include <string_view>
#include <vector>
#include "PriceCandle.hpp"

namespace trading {

class ILocalCache {
public:
    virtual ~ILocalCache() = default;

    ILocalCache()                              = default;
    ILocalCache(const ILocalCache&)            = delete;
    ILocalCache& operator=(const ILocalCache&) = delete;
    ILocalCache(ILocalCache&&)                 = default;
    ILocalCache& operator=(ILocalCache&&)      = default;

    [[nodiscard]] virtual bool trySave(
        std::string_view symbol,
        std::string_view interval,
        std::span<const PriceCandle> candles
    ) = 0;

    [[nodiscard]] virtual bool tryLoad(
        std::string_view symbol,
        std::string_view interval,
        std::vector<PriceCandle>& outCandles
    ) = 0;
};

} // namespace trading
