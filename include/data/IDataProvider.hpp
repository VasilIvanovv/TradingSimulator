#pragma once

#include "PriceCandle.hpp"
#include <string_view>
#include <vector>

namespace trading {

/**
 * @brief Source of historical OHLCV candle data.
 *
 * Implementations cover remote REST APIs (TwelveData, Alpaca) as well as
 * local sources (CSV files, databases). DataBroker holds a prioritised list
 * of providers and calls each one cooperatively: every provider appends what
 * it has to the shared result and the broker merges the contributions.
 */
class IDataProvider {
public:
    virtual ~IDataProvider() = default;

    IDataProvider()                                = default;
    IDataProvider(const IDataProvider&)            = delete;
    IDataProvider& operator=(const IDataProvider&) = delete;
    IDataProvider(IDataProvider&&)                 = default;
    IDataProvider& operator=(IDataProvider&&)      = default;

    /**
     * @brief Fetch candles for @p symbol at @p interval starting from @p startDate.
     *
     * Implementations should append their results to @p outCandles and return
     * @c true. On any failure (network error, unknown symbol, empty result)
     * they must leave @p outCandles unchanged and return @c false — partial
     * writes on failure are a contract violation.
     *
     * @param symbol     Ticker symbol, e.g. @c "AAPL" or @c "EUR/USD".
     * @param interval   Candle width accepted by the underlying API,
     *                   e.g. @c "1day", @c "1h", @c "15min".
     * @param startDate  Earliest candle to fetch, ISO 8601 date string,
     *                   e.g. @c "2020-01-01". Data is returned up to now.
     * @param outCandles Output vector; successful results are appended here.
     * @return @c true if at least one candle was fetched and appended.
     */
    [[nodiscard]] virtual bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) = 0;

    /**
     * @brief Whether this provider fetches data over a network.
     *
     * DataBroker uses this to decide whether to persist the assembled result
     * to the local cache after a successful cooperative fetch. Local sources
     * return @c false so their data is never needlessly re-written.
     */
    [[nodiscard]] virtual bool isRemoteSource() const noexcept { return false; }
};

} // namespace trading
