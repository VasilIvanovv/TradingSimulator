#include "data/DataBroker.hpp"
#include "Logging.hpp"

namespace trading {

DataBroker::DataBroker(std::vector<std::unique_ptr<IDataProvider>> providers,
                       std::unique_ptr<ILocalCache> cache)
    : m_providers(std::move(providers)), m_cache(std::move(cache)) {}

std::optional<std::vector<PriceCandle>>
DataBroker::getHistory(std::string_view symbol, std::string_view interval) {
  for (auto &provider : m_providers) {
    std::vector<PriceCandle> candles;

    LOG(Debug) << "Trying provider for " << symbol << "/" << interval;

    if (provider->tryGetHistory(symbol, interval, candles)) {
      LOG(Info) << "Fetched " << candles.size() << " candles for " << symbol
                << "/" << interval;
      if (provider->isRemoteSource()) {
        persistToCache(symbol, interval, candles);
      }
      return candles;
    }

    LOG(Warning) << "Provider failed for " << symbol << "/" << interval
                 << ", trying next...";
  }

  LOG(Error) << "All providers exhausted for " << symbol << "/" << interval;
  return std::nullopt;
}

void DataBroker::persistToCache(std::string_view symbol,
                                std::string_view interval,
                                const std::vector<PriceCandle> &candles) {
  if (!m_cache)
    return;
  if (!m_cache->trySave(symbol, interval, candles)) {
    LOG(Warning) << "Cache write failed for " << symbol << "/" << interval;
  }
}

} // namespace trading
