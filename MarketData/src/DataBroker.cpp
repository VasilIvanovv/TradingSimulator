#include "DataBroker.hpp"
#include "Logging.hpp"

#include <algorithm>
#include <ctime>

namespace trading {

namespace {

// Converts "YYYY-MM-DD" or "YYYY-MM-DD HH:MM:SS" to a rough hour count.
// Uses 365 days/year and 30 days/month — imprecise by design; sufficient for
// staleness detection without requiring <chrono> calendar arithmetic.
long roughHourIndex(std::string_view ts) {
    if (ts.size() < 10) return 0;
    long y = (ts[0]-'0')*1000 + (ts[1]-'0')*100 + (ts[2]-'0')*10 + (ts[3]-'0');
    long m = (ts[5]-'0')*10 + (ts[6]-'0');
    long d = (ts[8]-'0')*10 + (ts[9]-'0');
    long h = ts.size() >= 13 ? (ts[11]-'0')*10 + (ts[12]-'0') : 0;
    return (y * 365 + m * 30 + d) * 24 + h;
}

long roughHoursNow() {
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
    gmtime_s(&tm, &t);
    long y = tm.tm_year + 1900;
    long m = tm.tm_mon  + 1;
    return (y * 365 + m * 30 + tm.tm_mday) * 24 + tm.tm_hour;
}

// Maximum hours between the last cached candle and now before we consider
// the cache stale.  Sized to survive a long holiday weekend:
//   1h  — Fri close → Mon open is ~66 h; allow 80.
//   1day— Fri close → Mon open is  3 days; allow 5 days (120 h).
// Returns -1 for unrecognised intervals (skip the check).
long maxNormalGapHours(std::string_view interval) {
    if (interval == "1h"   || interval == "60min") return 80;
    if (interval == "1day" || interval == "1d"   ) return 120;
    return -1;
}

// O(1) check: is the tail of the cached data old enough that new candles
// likely exist?  Only looks at the last element — no full scan.
bool tailIsStale(const std::vector<PriceCandle>& candles,
                 std::string_view interval) {
    const long maxGap = maxNormalGapHours(interval);
    if (maxGap < 0 || candles.empty()) return false;
    return (roughHoursNow() - roughHourIndex(candles.back().timestamp)) > maxGap;
}

// O(n) check: scan every consecutive pair for a gap wider than the maximum
// normal gap for this interval.  Only called periodically (see m_scanPeriod).
bool hasInternalGap(const std::vector<PriceCandle>& candles,
                    std::string_view interval) {
    const long maxGap = maxNormalGapHours(interval);
    if (maxGap < 0 || candles.size() < 2) return false;
    for (size_t i = 1; i < candles.size(); ++i) {
        const long gap = roughHourIndex(candles[i].timestamp)
                       - roughHourIndex(candles[i - 1].timestamp);
        if (gap > maxGap) return true;
    }
    return false;
}

} // anonymous namespace

DataBroker::DataBroker(std::vector<std::unique_ptr<IDataProvider>> providers,
                       std::unique_ptr<ILocalCache> cache,
                       std::chrono::seconds scanPeriod)
    : m_providers(std::move(providers))
    , m_cache(std::move(cache))
    , m_scanPeriod(scanPeriod) {}

std::optional<std::vector<PriceCandle>>
DataBroker::getHistory(std::string_view symbol, std::string_view interval,
                       std::string_view startDate) {
    // 1. Load whatever the cache already holds from startDate onwards.
    //    This may be empty (cold cache) or partial (cache hasn't been updated
    //    recently, or had a gap).
    std::vector<PriceCandle> cached;
    if (m_cache) {
        if (m_cache->tryLoad(symbol, interval, startDate, cached)) {
            LOG(Info) << "Cache loaded " << cached.size() << " candles for "
                      << symbol << "/" << interval << " from " << startDate;
        } else {
            LOG(Debug) << "Cache miss for " << symbol << "/" << interval;
        }
    }

    // 2. Determine where to start the provider fetch.
    //
    //    Two checks, from cheapest to most expensive:
    //
    //    a) O(1) tail check — always runs.  If the last cached candle is older
    //       than the maximum normal gap for this interval (e.g. a long weekend),
    //       new data certainly exists; fetch the full range.
    //
    //    b) O(n) gap scan — runs on the first request after startup and then at
    //       most once per m_scanPeriod (default 24 h) per symbol/interval pair.
    //       Detects holes inside the cached range that the tail check misses.
    //       If a gap is found, fall back to a full fetch so the provider fills it.
    const std::string scanKey = std::string(symbol) + "/" + std::string(interval);
    const std::time_t now     = std::time(nullptr);

    const bool stale = tailIsStale(cached, interval);

    bool gapFound = false;
    const auto it = m_lastScanTime.find(scanKey);
    const bool scanDue = it == m_lastScanTime.end() ||
                         (now - it->second) >= m_scanPeriod.count();
    if (!stale && !cached.empty() && scanDue) {
        gapFound = hasInternalGap(cached, interval);
        m_lastScanTime[scanKey] = now;
        if (gapFound)
            LOG(Info) << "Gap scan found hole in cached data for "
                      << symbol << "/" << interval
                      << "; fetching full range from " << startDate;
        else
            LOG(Debug) << "Gap scan passed for " << symbol << "/" << interval;
    }

    if (stale && !cached.empty())
        LOG(Info) << "Stale tail for " << symbol << "/" << interval
                  << "; fetching full range from " << startDate;

    // Head-gap: the cache exists but doesn't reach back to startDate.
    // Happens when a later (shorter) request populated the cache first.
    // Fetch from startDate so the full requested range is covered.
    const bool headGap = !cached.empty() &&
        cached.front().timestamp.substr(0, 10) > std::string(startDate).substr(0, 10);

    if (headGap)
        LOG(Info) << "Head gap for " << symbol << "/" << interval
                  << "; cache starts " << cached.front().timestamp
                  << " but startDate is " << startDate << "; fetching full range";

    const std::string fetchFrom = (cached.empty() || stale || gapFound || headGap)
        ? std::string(startDate)
        : cached.back().timestamp;

    std::vector<PriceCandle> fresh;
    for (auto& provider : m_providers) {
        std::vector<PriceCandle> candles;
        LOG(Debug) << "Trying provider for " << symbol << "/" << interval
                   << " from " << fetchFrom;

        if (provider->tryGetHistory(symbol, interval, fetchFrom, candles)) {
            LOG(Info) << "Provider contributed " << candles.size()
                      << " candles for " << symbol << "/" << interval;
            fresh.insert(fresh.end(),
                         std::make_move_iterator(candles.begin()),
                         std::make_move_iterator(candles.end()));
        } else {
            LOG(Warning) << "Provider failed for " << symbol << "/" << interval;
        }
    }

    // 3. No new data from any provider — degrade gracefully to the cache.
    if (fresh.empty()) {
        if (!cached.empty()) {
            LOG(Info) << "No new data from providers; returning "
                      << cached.size() << " cached candles for "
                      << symbol << "/" << interval;
            return cached;
        }
        LOG(Error) << "All providers exhausted for " << symbol << "/" << interval;
        return std::nullopt;
    }

    // 4. Sort and deduplicate fresh candles (providers may return newest-first
    //    or overlap across providers).
    std::ranges::sort(fresh, {}, &PriceCandle::timestamp);
    {
        auto tail = std::ranges::unique(fresh, {}, &PriceCandle::timestamp);
        fresh.erase(tail.begin(), tail.end());
    }

    // 5. Merge fresh into cached.  fresh is placed first in the merge so that
    //    std::ranges::unique keeps fresh values over cached for equal timestamps.
    std::vector<PriceCandle> merged;
    merged.reserve(cached.size() + fresh.size());
    std::ranges::merge(fresh, cached, std::back_inserter(merged),
                       [](const PriceCandle& a, const PriceCandle& b) {
                           return a.timestamp < b.timestamp;
                       });
    {
        auto tail = std::ranges::unique(merged, {}, &PriceCandle::timestamp);
        merged.erase(tail.begin(), tail.end());
    }

    LOG(Info) << "Assembled " << merged.size() << " candles for " << symbol
              << "/" << interval << " (" << fresh.size() << " new from provider)";
    persistToCache(symbol, interval, fresh);  // trySave merges into existing file
    return merged;
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
