#pragma once

#include <string>
#include "IDataProvider.hpp"

namespace trading {

class AlpacaProvider final : public IDataProvider {
public:
    /**
     * @param apiKey    Alpaca API key ID.
     * @param apiSecret Alpaca secret key.
     */
    AlpacaProvider(std::string apiKey, std::string apiSecret);

    [[nodiscard]] bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::string_view startDate,
        std::vector<PriceCandle>& outCandles
    ) override;

    [[nodiscard]] bool isRemoteSource() const noexcept override { return true; }

private:
    std::string m_apiKey;
    std::string m_apiSecret;
    std::string m_baseUrl{"https://data.alpaca.markets"};
};

} // namespace trading
