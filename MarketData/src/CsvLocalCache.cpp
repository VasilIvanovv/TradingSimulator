#include "CsvLocalCache.hpp"
#include "Logging.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>

namespace trading {

namespace {

PriceCandle parseLine(std::string_view line) {
    // Split on commas without allocation; parse numbers with from_chars (no
    // locale, no heap, error-code only — exception thrown only on bad data).
    auto nextField = [](std::string_view& sv) -> std::string_view {
        const auto pos = sv.find(',');
        const auto field = (pos == std::string_view::npos) ? sv : sv.substr(0, pos);
        sv.remove_prefix(pos == std::string_view::npos ? sv.size() : pos + 1);
        return field;
    };

    auto parseDouble = [](std::string_view sv, double& out) {
        const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
        if (ec != std::errc{})
            throw std::invalid_argument(std::string("bad double: ").append(sv));
    };

    auto parseLong = [](std::string_view sv, long& out) {
        const auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), out);
        if (ec != std::errc{})
            throw std::invalid_argument(std::string("bad long: ").append(sv));
    };

    PriceCandle candle;
    candle.timestamp = std::string(nextField(line));
    double d{};
    parseDouble(nextField(line), d); candle.open  = d;
    parseDouble(nextField(line), d); candle.high  = d;
    parseDouble(nextField(line), d); candle.low   = d;
    parseDouble(nextField(line), d); candle.close = d;
    long l{};
    parseLong(nextField(line), l); candle.volume = l;
    return candle;
}

// Reads every row from path (skipping the header), in file order. Returns an
// empty vector if the file doesn't exist; logs and skips lines that fail to
// parse rather than failing the whole read.
std::vector<PriceCandle> readAllCandles(const std::filesystem::path& path) {
    std::vector<PriceCandle> rows;
    std::ifstream file(path);
    if (!file) return rows;

    std::string line;
    std::getline(file, line); // discard header

    while (std::getline(file, line)) {
        if (line.empty())
            continue;
        try {
            rows.push_back(parseLine(line));
        } catch (const std::exception& e) {
            LOG(Warning) << "CsvLocalCache: skipping malformed line in "
                         << path.string() << ": " << e.what();
        }
    }
    return rows;
}

} // anonymous namespace

CsvLocalCache::CsvLocalCache(std::filesystem::path dataDir)
    : m_dataDir(std::move(dataDir)) {}

bool CsvLocalCache::trySave(std::string_view symbol, std::string_view interval,
                            std::span<const PriceCandle> candles) {
    const auto path = buildPath(symbol, interval);

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        LOG(Error) << "CsvLocalCache: failed to create directory "
                   << path.parent_path().string() << ": " << ec.message();
        return false;
    }

    std::vector<PriceCandle> existing = readAllCandles(path);

    // Both sequences are sorted ascending by timestamp (the file is always
    // written sorted; callers pass candles sorted per ILocalCache contract).
    // Linear O(n) merge beats the old O(n log n) std::map approach.
    //
    // `candles` (new) goes first so that for equal timestamps the new entry
    // precedes the old one in the merged output — std::ranges::unique then
    // keeps the first of each run, which is the new value (new wins).
    std::vector<PriceCandle> merged;
    merged.reserve(existing.size() + candles.size());
    std::ranges::merge(candles, existing, std::back_inserter(merged),
                       [](const PriceCandle& a, const PriceCandle& b) {
                           return a.timestamp < b.timestamp;
                       });

    const auto dups = std::ranges::unique(merged, {}, &PriceCandle::timestamp);
    merged.erase(dups.begin(), dups.end());

    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        LOG(Error) << "CsvLocalCache: failed to open " << path.string()
                   << " for writing";
        return false;
    }

    file << "timestamp,open,high,low,close,volume\n";
    for (const auto& c : merged) {
        file << std::format("{},{},{},{},{},{}\n", c.timestamp, c.open, c.high,
                            c.low, c.close, c.volume);
    }

    if (!file.good()) {
        LOG(Error) << "CsvLocalCache: write failure for " << path.string();
        return false;
    }

    LOG(Debug) << "CsvLocalCache: saved " << merged.size()
               << " candles (added/updated " << candles.size() << ") to "
               << path.string();
    return true;
}

bool CsvLocalCache::tryLoad(std::string_view symbol, std::string_view interval,
                            std::string_view startDate,
                            std::vector<PriceCandle>& outCandles) {
    const auto path = buildPath(symbol, interval);
    if (!std::filesystem::exists(path)) {
        LOG(Debug) << "CsvLocalCache: no cache file at " << path.string();
        return false;
    }

    std::vector<PriceCandle> loaded;
    for (auto& candle : readAllCandles(path)) {
        if (candle.timestamp >= startDate) {
            loaded.push_back(std::move(candle));
        }
    }

    if (loaded.empty()) {
        LOG(Debug) << "CsvLocalCache: no candles from " << startDate << " in "
                   << path.string();
        return false;
    }

    outCandles.insert(outCandles.end(), std::make_move_iterator(loaded.begin()),
                      std::make_move_iterator(loaded.end()));

    LOG(Info) << "CsvLocalCache: loaded " << loaded.size() << " candles for "
              << symbol << "/" << interval << " from " << startDate;
    return true;
}

std::filesystem::path
CsvLocalCache::buildPath(std::string_view symbol,
                         std::string_view interval) const {
    return m_dataDir / symbol / std::format("{}.csv", interval);
}

} // namespace trading
