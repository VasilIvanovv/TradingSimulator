#include "TwelveDataProvider.hpp"
#include "Logging.hpp"

#include <curl/curl.h>
#include <format>
#include <nlohmann/json.hpp>

namespace trading {

// ---------------------------------------------------------------------------
// Internal HTTP helper (anonymous namespace — not part of the public API)
// ---------------------------------------------------------------------------
namespace {

// Initialises libcurl once for the lifetime of the process.
struct CurlGlobal {
    CurlGlobal()  { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

struct CurlHandle {
    CURL* h{curl_easy_init()};
    ~CurlHandle() { if (h) curl_easy_cleanup(h); }
    explicit operator bool() const noexcept { return h != nullptr; }
    operator CURL*()         const noexcept { return h; }
};

size_t appendToString(char* ptr, size_t /*size*/, size_t nmemb, void* userdata) {
    static_cast<std::string*>(userdata)->append(ptr, nmemb);
    return nmemb;
}

// Performs an HTTP(S) GET and returns the full response body.
// Returns an empty string on any network or curl error.
std::string httpsGet(std::string_view url) {
    static const CurlGlobal curlGlobal;  // initialised on first call, cleaned up at exit

    std::string body;

    CurlHandle curl;
    if (!curl) return {};

    curl_easy_setopt(curl, CURLOPT_URL,           std::string(url).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendToString);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &body);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    if (curl_easy_perform(curl) != CURLE_OK) return {};
    return body;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// TwelveDataProvider
// ---------------------------------------------------------------------------

// TwelveData's hard cap on candles returned per request. Requests for a date
// range wider than this come back truncated with no indication from the API
// itself — see the warning logged below when this limit is hit.
constexpr int kMaxOutputSize = 5000;

TwelveDataProvider::TwelveDataProvider(std::string apiKey, std::string baseUrl)
    : m_apiKey(std::move(apiKey))
    , m_baseUrl(std::move(baseUrl)) {}

bool TwelveDataProvider::tryGetHistory(std::string_view symbol,
                                       std::string_view interval,
                                       std::string_view startDate,
                                       std::vector<PriceCandle>& outCandles) {
    const std::string url = std::format(
        "{}/time_series?symbol={}&interval={}&start_date={}&outputsize={}&apikey={}",
        m_baseUrl, symbol, interval, startDate, kMaxOutputSize, m_apiKey);

    LOG(Debug) << "TwelveData GET " << m_baseUrl << "/time_series symbol=" << symbol;

    const std::string body = httpsGet(url);
    if (body.empty()) {
        LOG(Warning) << "TwelveData: no response from " << m_baseUrl
                     << " for " << symbol << "/" << interval;
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
        LOG(Info) << "TwelveData: " << outCandles.size() << " candles for "
                  << symbol << "/" << interval
                  << " (credits used: " << m_creditsUsed << ")";

        if (outCandles.size() == static_cast<size_t>(kMaxOutputSize)) {
            LOG(Warning) << "TwelveData: result for " << symbol << "/" << interval
                        << " hit the " << kMaxOutputSize
                        << "-candle cap — older data within the requested "
                        << "range from " << startDate << " may be missing.";
        }

        return !outCandles.empty();

    } catch (const std::exception& e) {
        LOG(Error) << "TwelveData parse error for " << symbol << ": " << e.what();
        return false;
    }
}

} // namespace trading
