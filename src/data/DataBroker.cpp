#include "data/DataBroker.hpp"
#include "Logging.hpp"

#include <algorithm>

namespace trading {

DataBroker::DataBroker(std::vector<std::unique_ptr<IDataProvider>> providers,
                       std::unique_ptr<ILocalCache> cache)
    : m_providers(std::move(providers)), m_cache(std::move(cache)) {}

std::optional<std::vector<PriceCandle>>
DataBroker::getHistory(std::string_view symbol, std::string_view interval,
                       std::string_view startDate) {
    // 1. Cache-first: the cache is responsible for returning only candles
    //    from startDate onwards; no further filtering needed here.
    if (m_cache) {
        std::vector<PriceCandle> cached;
        if (m_cache->tryLoad(symbol, interval, startDate, cached)) {
            LOG(Info) << "Cache hit for " << symbol << "/" << interval
                      << " from " << startDate << " (" << cached.size()
                      << " candles)";
            return cached;
        }
        LOG(Debug) << "Cache miss for " << symbol << "/" << interval;
    }

    // 2. Cooperative fetch: every provider contributes what it has.
    //    All providers are tried; results are merged, sorted by timestamp,
    //    and deduplicated before being persisted and returned.
    std::vector<PriceCandle> all;

    for (auto& provider : m_providers) {
        std::vector<PriceCandle> candles;
        LOG(Debug) << "Trying provider for " << symbol << "/" << interval
                   << " from " << startDate;

        if (provider->tryGetHistory(symbol, interval, startDate, candles)) {
            LOG(Info) << "Provider contributed " << candles.size()
                      << " candles for " << symbol << "/" << interval;
            all.insert(all.end(),
                       std::make_move_iterator(candles.begin()),
                       std::make_move_iterator(candles.end()));
        } else {
            LOG(Warning) << "Provider failed for " << symbol << "/" << interval;
        }
    }

    if (all.empty()) {
        LOG(Error) << "All providers exhausted for " << symbol << "/"
                   << interval;
        return std::nullopt;
    }

    std::ranges::sort(all, {}, &PriceCandle::timestamp);
    auto tail = std::ranges::unique(all, {}, &PriceCandle::timestamp);
    all.erase(tail.begin(), tail.end());

    LOG(Info) << "Assembled " << all.size() << " candles for " << symbol
              << "/" << interval;
    persistToCache(symbol, interval, all);
    return all;
}

void DataBroker::persistToCache(std::string_view symbol,
                                std::string_view interval,
                                const std::vector<PriceCandle>& candles) {
    if (!m_cache)
        return;
    if (!m_cache->trySave(symbol, interval, candles)) {
        LOG(Warning) << "Cache write failed for " << symbol << "/" << interval;
    }
}

} // namespace trading
