#pragma once

#include <string>
#include "IDataProvider.hpp"

namespace trading {

class TwelveDataProvider final : public IDataProvider {
public:
    /**
     * @param apiKey  Twelve Data API key used to authenticate REST requests.
     * @param baseUrl Base URL of the API host. Defaults to the live endpoint;
     *                override in tests to point at a local mock server.
     */
    explicit TwelveDataProvider(std::string apiKey,
                                 std::string baseUrl = "https://api.twelvedata.com");

    [[nodiscard]] bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) override;

    [[nodiscard]] bool isRemoteSource() const noexcept override { return true; }

private:
    std::string m_apiKey;
    std::string m_baseUrl;
    int         m_creditsUsed{};
};

} // namespace trading
