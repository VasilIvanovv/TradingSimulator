#pragma once

#include <filesystem>
#include "ILocalCache.hpp"

namespace trading {

class CsvLocalCache final : public ILocalCache {
public:
    /** @param dataDir Directory where per-symbol CSV files are stored. */
    explicit CsvLocalCache(std::filesystem::path dataDir);

    [[nodiscard]] bool trySave(
        std::string_view symbol,
        std::string_view interval,
        std::span<const PriceCandle> candles
    ) override;

    [[nodiscard]] bool tryLoad(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) override;

private:
    std::filesystem::path m_dataDir;

    [[nodiscard]] std::filesystem::path buildPath(
        std::string_view symbol,
        std::string_view interval
    ) const;
};

} // namespace trading
