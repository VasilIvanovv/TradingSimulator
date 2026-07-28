#include "TwelveDataProvider.hpp"
#include "Logging.hpp"

#include <httplib.h>
#include <format>
#include <nlohmann/json.hpp>

namespace trading {

namespace {

std::string httpGet(const std::string& baseUrl, const std::string& path) {
    httplib::Client client(baseUrl);
    client.set_connection_timeout(10);
    client.set_read_timeout(15);
    client.set_follow_location(true);
    const auto res = client.Get(path);
    if (!res || res->status != 200) return {};
    return res->body;
}

} // anonymous namespace

constexpr int kMaxOutputSize = 5000;

TwelveDataProvider::TwelveDataProvider(std::string apiKey, std::string baseUrl)
    : m_apiKey(std::move(apiKey))
    , m_baseUrl(std::move(baseUrl)) {}

bool TwelveDataProvider::tryGetHistory(std::string_view symbol,
                                       std::string_view interval,
                                       std::string_view startDate,
                                       std::vector<PriceCandle>& outCandles) {
    const std::string path = std::format(
        "/time_series?symbol={}&interval={}&start_date={}&outputsize={}&apikey={}",
        symbol, interval, startDate, kMaxOutputSize, m_apiKey);

    LOG(Debug) << "TwelveData GET " << m_baseUrl << "/time_series symbol=" << symbol;

    const std::string body = httpGet(m_baseUrl, path);
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
