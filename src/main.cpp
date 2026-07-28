#include "ApiHandler.hpp"
#include "AuthHandler.hpp"
#include "CsvLocalCache.hpp"
#include "HttpTransport.hpp"
#include "JwtService.hpp"
#include "LogoManager.hpp"
#include "Logging.hpp"
#include "PaperAccount.hpp"
#include "SqliteAccountManager.hpp"
#include "SqliteAccountStore.hpp"
#include "SqliteUserStore.hpp"
#include "SymbolManager.hpp"
#include "TradingController.hpp"
#include "TwelveDataProvider.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

static void loadDotEnv(const char* path = ".env") {
    std::ifstream file(path);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key   = line.substr(0, eq);
        const std::string value = line.substr(eq + 1);
        _putenv_s(key.c_str(), value.c_str());
    }
}

static std::optional<std::string> getEnv(const char* name) {
    char*  value = nullptr;
    size_t len   = 0;
    if (_dupenv_s(&value, &len, name) != 0 || !value) return std::nullopt;
    std::string result(value);
    free(value);
    return result;
}

int main() {
    loadDotEnv();

    constexpr int         port      = 8080;
    constexpr const char* interval  = "1day";
    constexpr const char* startDate = "2025-01-01";
    constexpr const char* dbPath    = "trading.db";
    constexpr double      startCash = 10'000.0;

    const auto apiKey    = getEnv("TWELVEDATA_API_KEY");
    const auto jwtSecret = getEnv("JWT_SECRET");

    if (!apiKey) {
        std::cerr << "Error: TWELVEDATA_API_KEY environment variable is not set.\n";
        return 1;
    }
    if (!jwtSecret) {
        std::cerr << "Error: JWT_SECRET environment variable is not set.\n";
        return 1;
    }

    trading::SqliteUserStore    userStore(dbPath);
    trading::SqliteAccountManager accountManager(dbPath);
    trading::JwtService         jwtService(*jwtSecret);

    // Each account gets its own TradingController, created on first request.
    trading::ApiHandler handler(
        [&](int accountId) {
            std::vector<std::unique_ptr<trading::IDataProvider>> providers;
            providers.push_back(std::make_unique<trading::TwelveDataProvider>(*apiKey));

            return std::make_unique<trading::TradingController>(
                std::make_unique<trading::PaperAccount>(
                    startCash,
                    std::make_unique<trading::SqliteAccountStore>(dbPath, accountId)),
                std::make_unique<trading::DataBroker>(
                    std::move(providers),
                    std::make_unique<trading::CsvLocalCache>("cache")),
                interval, startDate);
        },
        accountManager);

    trading::SymbolManager symbolManager("symbols.json");
    symbolManager.startAutoReload();

    trading::LogoManager logoCache("logo_cache",
        [&symbolManager]() {
            auto info = symbolManager.getSymbols();
            std::vector<std::string> symbols;
            symbols.reserve(info.size());
            for (const auto& s : info) symbols.push_back(s.symbol);
            return symbols;
        });
    logoCache.startAutoRefresh();

    trading::AuthHandler   auth(userStore, jwtService);
    trading::HttpTransport transport(handler, auth, jwtService,
                                     symbolManager, logoCache, port);

    std::cout << "TradingSimulator v0.1.0\n";
    std::cout << "API listening on http://localhost:" << port << "\n\n";

    transport.start();
    return 0;
}
