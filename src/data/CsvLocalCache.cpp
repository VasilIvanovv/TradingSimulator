#include "data/CsvLocalCache.hpp"
#include <format>

namespace trading {

CsvLocalCache::CsvLocalCache(std::filesystem::path dataDir)
    : m_dataDir(std::move(dataDir))
{}

bool CsvLocalCache::trySave(
    std::string_view /*symbol*/,
    std::string_view /*interval*/,
    std::span<const PriceCandle> /*candles*/
) {
    return false;
}

bool CsvLocalCache::tryLoad(
    std::string_view /*symbol*/,
    std::string_view /*interval*/,
    std::string_view /*startDate*/,
    std::vector<PriceCandle>& /*outCandles*/
) {
    return false;
}

std::filesystem::path CsvLocalCache::buildPath(
    std::string_view symbol,
    std::string_view interval
) const {
    return m_dataDir / std::format("data_{}_{}.csv", symbol, interval);
}

} // namespace trading
