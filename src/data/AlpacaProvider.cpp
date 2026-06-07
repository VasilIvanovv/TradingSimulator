#include "data/AlpacaProvider.hpp"

namespace trading {

AlpacaProvider::AlpacaProvider(std::string apiKey, std::string apiSecret)
    : m_apiKey(std::move(apiKey)), m_apiSecret(std::move(apiSecret)) {}

bool AlpacaProvider::tryGetHistory(std::string_view /*symbol*/,
                                   std::string_view /*interval*/,
                                   std::vector<PriceCandle> & /*outCandles*/
) {
  return false;
}

} // namespace trading
