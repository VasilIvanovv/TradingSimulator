#include "ApiHandler.hpp"
#include "AuthHandler.hpp"
#include "CsvLocalCache.hpp"
#include "HttpTransport.hpp"
#include "JwtService.hpp"
#include "Logging.hpp"
#include "PaperAccount.hpp"
#include "SqliteAccountStore.hpp"
#include "TradingController.hpp"
#include "TwelveDataProvider.hpp"
#include "SqliteUserStore.hpp"

#include <iostream>

int main() {
    constexpr int          port       = 8080;
    constexpr const char*  apiKey     = "09bc29fdccc54c0ca12c5c7353d3cb1f";
    constexpr const char*  interval   = "1day";
    constexpr const char*  startDate  = "2025-01-01";
    constexpr const char*  dbPath     = "trading.db";
    constexpr const char*  jwtSecret  = "change-me-in-production";
    constexpr double       startCash  = 10'000.0;

    trading::SqliteUserStore userStore(dbPath);
    trading::JwtService jwtService(jwtSecret);

    // Each authenticated user gets their own TradingController on first request.
    trading::ApiHandler handler([&](int userId) {
        std::vector<std::unique_ptr<trading::IDataProvider>> providers;
        providers.push_back(std::make_unique<trading::TwelveDataProvider>(apiKey));

        return std::make_unique<trading::TradingController>(
            std::make_unique<trading::PaperAccount>(
                startCash,
                std::make_unique<trading::SqliteAccountStore>(dbPath, userId)),
            std::make_unique<trading::DataBroker>(
                std::move(providers),
                std::make_unique<trading::CsvLocalCache>("cache")),
            interval, startDate);
    });

    trading::AuthHandler   auth(userStore, jwtService);
    trading::HttpTransport transport(handler, auth, jwtService, port);

    std::cout << "TradingSimulator v0.1.0\n";
    std::cout << "API listening on http://localhost:" << port << "\n\n";
    std::cout << "Public endpoints:\n";
    std::cout << "  POST /auth/register      { username, password }\n";
    std::cout << "  POST /auth/login         { username, password }\n\n";
    std::cout << "Protected endpoints (Authorization: Bearer <token>):\n";
    std::cout << "  GET  /account\n";
    std::cout << "  POST /orders             { symbol, side, quantity, price, timestamp? }\n";
    std::cout << "  POST /rules              { symbol, triggerPrice, side, quantity }\n";
    std::cout << "  DELETE /rules/{symbol}\n";
    std::cout << "  GET  /history?symbol=AAPL&interval=1day&start=2025-01-01\n\n";

    transport.start(); // blocks until stopped
    return 0;
}
