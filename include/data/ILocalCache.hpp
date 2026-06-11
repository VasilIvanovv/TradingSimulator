#pragma once

#include <span>
#include <string_view>
#include <vector>
#include "PriceCandle.hpp"

namespace trading {

/**
 * @brief Persistent local storage for previously fetched candle data.
 *
 * A cache sits between DataBroker and the remote providers. On every request
 * the broker checks the cache first (tryLoad); if the data is not there it
 * falls back to the provider chain, then writes the assembled result back
 * (trySave). Implementations decide the storage format (CSV, SQLite, …).
 */
class ILocalCache {
public:
    virtual ~ILocalCache() = default;

    ILocalCache()                              = default;
    ILocalCache(const ILocalCache&)            = delete;
    ILocalCache& operator=(const ILocalCache&) = delete;
    ILocalCache(ILocalCache&&)                 = default;
    ILocalCache& operator=(ILocalCache&&)      = default;

    /**
     * @brief Persist @p candles for the given @p symbol / @p interval pair.
     *
     * Implementations should overwrite any previously cached data for the
     * same key so that the stored dataset always reflects the latest
     * fully-assembled result from the broker.
     *
     * @param symbol   Ticker symbol, e.g. @c "AAPL".
     * @param interval Candle width, e.g. @c "1day".
     * @param candles  Candles to store; sorted ascending by timestamp.
     * @return @c true if all candles were written successfully.
     */
    [[nodiscard]] virtual bool trySave(
        std::string_view symbol,
        std::string_view interval,
        std::span<const PriceCandle> candles
    ) = 0;

    /**
     * @brief Load cached candles from @p startDate onwards into @p outCandles.
     *
     * The implementation is responsible for filtering by @p startDate so that
     * only candles with @c timestamp >= @p startDate are returned. This keeps
     * the filtering at the storage layer where it can be done efficiently
     * (e.g. binary search in a sorted file, SQL WHERE clause).
     *
     * Returns @c false — and leaves @p outCandles unchanged — if no cached
     * file exists, the file cannot be read, or the stored data contains no
     * candles from @p startDate onwards.
     *
     * @param symbol     Ticker symbol, e.g. @c "AAPL".
     * @param interval   Candle width, e.g. @c "1day".
     * @param startDate  Earliest candle to return, ISO 8601 date string,
     *                   e.g. @c "2020-01-01".
     * @param outCandles Output vector; matching candles are appended here.
     * @return @c true if at least one candle was loaded.
     */
    [[nodiscard]] virtual bool tryLoad(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) = 0;
};

} // namespace trading
