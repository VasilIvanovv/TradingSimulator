#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "IDataProvider.hpp"
#include "ILocalCache.hpp"

namespace trading {

/**
 * @brief Orchestrates data retrieval across a cache and multiple providers.
 *
 * DataBroker owns an ordered list of IDataProvider instances and an optional
 * ILocalCache. On each request it first tries the cache; on a miss it runs a
 * cooperative fetch — every provider is asked in order, each appending the
 * candles it has. The merged, deduplicated result is then written back to the
 * cache and returned to the caller.
 */
class DataBroker {
public:
    /**
     * @param providers  Ordered list of data sources. All are tried on a cache
     *                   miss; results are merged by timestamp.
     * @param cache      Optional local cache. Pass @c nullptr to disable
     *                   caching entirely.
     */
    DataBroker(
        std::vector<std::unique_ptr<IDataProvider>> providers,
        std::unique_ptr<ILocalCache> cache
    );

    ~DataBroker()                            = default;
    DataBroker(const DataBroker&)            = delete;
    DataBroker& operator=(const DataBroker&) = delete;
    DataBroker(DataBroker&&)                 = default;
    DataBroker& operator=(DataBroker&&)      = default;

    /**
     * @brief Return candles for @p symbol at @p interval from @p startDate to now.
     *
     * Checks the cache first. On a miss, queries every registered provider
     * cooperatively, merges and deduplicates the results by timestamp
     * (ascending), writes the assembled dataset to the cache, and returns it.
     *
     * @param symbol    Ticker symbol, e.g. @c "AAPL".
     * @param interval  Candle width, e.g. @c "1day", @c "1h".
     * @param startDate Earliest candle to return, ISO 8601 date string,
     *                  e.g. @c "2020-01-01".
     * @return The assembled candle vector, or @c std::nullopt if no provider
     *         returned any data.
     */
    [[nodiscard]] std::optional<std::vector<PriceCandle>> getHistory(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate
    );

private:
    std::vector<std::unique_ptr<IDataProvider>> m_providers;
    std::unique_ptr<ILocalCache>                m_cache;

    void persistToCache(
        std::string_view symbol,
        std::string_view interval,
        const std::vector<PriceCandle>& candles
    );
};

} // namespace trading
