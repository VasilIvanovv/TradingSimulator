#include "data/TwelveDataProvider.hpp"
#include "Logging.hpp"

#include <format>
#include <nlohmann/json.hpp>
#include <windows.h>
#include <winhttp.h>

namespace trading {

// ---------------------------------------------------------------------------
// Internal HTTP helper (anonymous namespace — not part of the public API)
// ---------------------------------------------------------------------------
namespace {

struct WinHttpHandle {
    HINTERNET h{nullptr};
    ~WinHttpHandle() { if (h) WinHttpCloseHandle(h); }
    explicit operator bool() const noexcept { return h != nullptr; }
    operator HINTERNET()    const noexcept { return h; }
};

// Performs an HTTPS GET and returns the full response body.
// Returns an empty string on any network or Windows error.
std::string httpsGet(std::string_view host, std::string_view path) {
    auto toWide = [](std::string_view s) {
        return std::wstring(s.begin(), s.end());
    };

    WinHttpHandle session{WinHttpOpen(
        L"TradingSimulator/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    )};
    if (!session) return {};

    WinHttpHandle connection{WinHttpConnect(
        session, toWide(host).c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0
    )};
    if (!connection) return {};

    WinHttpHandle request{WinHttpOpenRequest(
        connection,
        L"GET",
        toWide(path).c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    )};
    if (!request) return {};

    if (!WinHttpSendRequest(request,
                            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
        return {};

    if (!WinHttpReceiveResponse(request, nullptr)) return {};

    std::string body;
    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
        std::string chunk(available, '\0');
        DWORD bytesRead = 0;
        WinHttpReadData(request, chunk.data(), available, &bytesRead);
        body.append(chunk.data(), bytesRead);
    }
    return body;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// TwelveDataProvider
// ---------------------------------------------------------------------------

TwelveDataProvider::TwelveDataProvider(std::string apiKey)
    : m_apiKey(std::move(apiKey))
{}

bool TwelveDataProvider::tryGetHistory(
    std::string_view symbol,
    std::string_view interval,
    std::vector<PriceCandle>& outCandles
) {
    const std::string path = std::format(
        "/time_series?symbol={}&interval={}&outputsize=5000&apikey={}",
        symbol, interval, m_apiKey
    );

    LOG(Debug) << "TwelveData GET api.twelvedata.com" << path;

    const std::string body = httpsGet("api.twelvedata.com", path);
    if (body.empty()) {
        LOG(Warning) << "TwelveData: no response for " << symbol << "/" << interval;
        return false;
    }

    try {
        const auto json = nlohmann::json::parse(body);

        if (json.value("status", "") == "error") {
            LOG(Warning) << "TwelveData API error: " << json.value("message", "unknown");
            return false;
        }

        const auto& values = json.at("values");
        outCandles.clear();
        outCandles.reserve(values.size());

        for (const auto& row : values) {
            PriceCandle candle;
            candle.timestamp = row.at("datetime").get<std::string>();
            candle.open      = std::stod(row.at("open") .get<std::string>());
            candle.high      = std::stod(row.at("high") .get<std::string>());
            candle.low       = std::stod(row.at("low")  .get<std::string>());
            candle.close     = std::stod(row.at("close").get<std::string>());
            candle.volume    = row.contains("volume")
                                   ? std::stol(row["volume"].get<std::string>())
                                   : 0L;
            outCandles.push_back(std::move(candle));
        }

        ++m_creditsUsed;
        LOG(Info) << "TwelveData: " << outCandles.size()
                  << " candles for " << symbol << "/" << interval
                  << " (credits used: " << m_creditsUsed << ")";
        return !outCandles.empty();

    } catch (const std::exception& e) {
        LOG(Error) << "TwelveData parse error for " << symbol << ": " << e.what();
        return false;
    }
}

} // namespace trading
