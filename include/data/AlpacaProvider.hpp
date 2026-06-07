#pragma once

#include <string>
#include "IDataProvider.hpp"

namespace trading {

class AlpacaProvider final : public IDataProvider {
public:
    AlpacaProvider(std::string apiKey, std::string apiSecret);

    [[nodiscard]] bool tryGetHistory(
        std::string_view symbol,
        std::string_view interval,
        std::vector<PriceCandle>& outCandles
    ) override;

    [[nodiscard]] bool isRemoteSource() const noexcept override { return true; }

private:
    std::string m_apiKey;
    std::string m_apiSecret;
    std::string m_baseUrl{"https://data.alpaca.markets"};
};

} // namespace trading
