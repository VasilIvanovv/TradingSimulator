#pragma once

#include <string_view>
#include <vector>
#include "PriceCandle.hpp"

namespace trading {

class IDataProvider {
public:
    virtual ~IDataProvider() = default;

    IDataProvider()                                = default;
    IDataProvider(const IDataProvider&)            = delete;
    IDataProvider& operator=(const IDataProvider&) = delete;
    IDataProvider(IDataProvider&&)                 = default;
    IDataProvider& operator=(IDataProvider&&)      = default;

    [[nodiscard]] virtual bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::vector<PriceCandle>& outCandles
    ) = 0;

    // Remote sources trigger a cache write after a successful fetch.
    [[nodiscard]] virtual bool isRemoteSource() const noexcept { return false; }
};

} // namespace trading
