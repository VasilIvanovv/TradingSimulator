#include "ApiHandler.hpp"
#include "CsvLocalCache.hpp"
#include "HttpTransport.hpp"
#include "Logging.hpp"
#include "PaperAccount.hpp"
#include "TradingController.hpp"
#include "TwelveDataProvider.hpp"

#include <iostream>

int main() {
    constexpr int         port      = 8080;
    constexpr const char* apiKey    = "09bc29fdccc54c0ca12c5c7353d3cb1f";
    constexpr const char* interval  = "1day";
    constexpr const char* startDate = "2025-01-01";

    std::vector<std::unique_ptr<trading::IDataProvider>> providers;
    providers.push_back(std::make_unique<trading::TwelveDataProvider>(apiKey));

    trading::TradingController controller(
        std::make_unique<trading::PaperAccount>(10'000.0),
        std::make_unique<trading::DataBroker>(
            std::move(providers),
            std::make_unique<trading::CsvLocalCache>("cache")
        ),
        interval, startDate
    );

    trading::ApiHandler    handler(controller);
    trading::HttpTransport transport(handler, port);

    std::cout << "TradingSimulator v0.1.0\n";
    std::cout << "API listening on http://localhost:" << port << "\n\n";
    std::cout << "Endpoints:\n";
    std::cout << "  GET  /account\n";
    std::cout << "  POST /orders          { symbol, side, quantity, price, timestamp? }\n";
    std::cout << "  POST /rules           { symbol, triggerPrice, side, quantity }\n";
    std::cout << "  DELETE /rules/{symbol}\n";
    std::cout << "  GET  /history?symbol=AAPL&interval=1day&start=2025-01-01\n\n";

    transport.start(); // blocks until stopped
    return 0;
}
