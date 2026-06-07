#include "data/TwelveDataProvider.hpp"

namespace trading {

TwelveDataProvider::TwelveDataProvider(std::string apiKey)
    : m_apiKey(std::move(apiKey)) {}

bool TwelveDataProvider::tryGetHistory(std::string_view /*symbol*/,
                                       std::string_view /*interval*/,
                                       std::vector<PriceCandle> & /*outCandles*/
) {
  return false;
}

} // namespace trading
