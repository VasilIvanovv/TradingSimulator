#include "Logging.hpp"
#include "data/DataBroker.hpp"
#include "data/TwelveDataProvider.hpp"

#include <format>
#include <iostream>

int main() {
    std::cout << "TradingSimulator v0.1.0\n\n";

    constexpr std::string_view apiKey    = "09bc29fdccc54c0ca12c5c7353d3cb1f";
    constexpr std::string_view symbol    = "AAPL";
    constexpr std::string_view interval  = "1day";
    constexpr std::string_view startDate = "2025-01-01";

    std::vector<std::unique_ptr<trading::IDataProvider>> providers;
    providers.push_back(std::make_unique<trading::TwelveDataProvider>(std::string(apiKey)));

    trading::DataBroker broker(std::move(providers), nullptr);

    LOG(Info) << "Fetching " << symbol << " " << interval << " from " << startDate;

    auto candles = broker.getHistory(symbol, interval, startDate);

    if (!candles.has_value()) {
        LOG(Error) << "No data returned.";
        return 1;
    }

    std::cout << std::format("{:<22} {:>10} {:>10} {:>10} {:>10} {:>12}\n",
                             "Timestamp", "Open", "High", "Low", "Close", "Volume");
    std::cout << std::string(76, '-') << '\n';

    for (const auto& c : *candles) {
        std::cout << std::format("{:<22} {:>10.4f} {:>10.4f} {:>10.4f} {:>10.4f} {:>12}\n",
                                 c.timestamp, c.open, c.high, c.low, c.close, c.volume);
    }

    std::cout << '\n' << std::format("Total: {} candles\n", candles->size());
    return 0;
}
