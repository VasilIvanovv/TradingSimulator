#pragma once

#include <string>
#include "IDataProvider.hpp"

namespace trading {

class TwelveDataProvider final : public IDataProvider {
public:
    /** @param apiKey Twelve Data API key used to authenticate REST requests. */
    explicit TwelveDataProvider(std::string apiKey);

    [[nodiscard]] bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) override;

    [[nodiscard]] bool isRemoteSource() const noexcept override { return true; }

private:
    std::string m_apiKey;
    std::string m_baseUrl{"https://api.twelvedata.com"};
    int         m_creditsUsed{};
};

} // namespace trading
