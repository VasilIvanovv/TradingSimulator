#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <vector>
#include "IDataProvider.hpp"
#include "ILocalCache.hpp"

namespace trading {

class DataBroker {
public:
    DataBroker(
        std::vector<std::unique_ptr<IDataProvider>> providers,
        std::unique_ptr<ILocalCache> cache
    );

    ~DataBroker()                            = default;
    DataBroker(const DataBroker&)            = delete;
    DataBroker& operator=(const DataBroker&) = delete;
    DataBroker(DataBroker&&)                 = default;
    DataBroker& operator=(DataBroker&&)      = default;

    [[nodiscard]] std::optional<std::vector<PriceCandle>> getHistory(
        std::string_view symbol,
        std::string_view interval
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
