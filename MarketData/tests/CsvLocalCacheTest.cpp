#include <gtest/gtest.h>

#include "CsvLocalCache.hpp"

#include <filesystem>
#include <format>
#include <fstream>

using namespace trading;
using namespace testing;

namespace {

PriceCandle makeCandle(std::string timestamp, double open, double high,
                       double low, double close, long volume) {
    return PriceCandle{std::move(timestamp), open, high, low, close, volume};
}

void expectCandleEq(const PriceCandle &actual, const PriceCandle &expected) {
    EXPECT_EQ(actual.timestamp, expected.timestamp);
    EXPECT_DOUBLE_EQ(actual.open, expected.open);
    EXPECT_DOUBLE_EQ(actual.high, expected.high);
    EXPECT_DOUBLE_EQ(actual.low, expected.low);
    EXPECT_DOUBLE_EQ(actual.close, expected.close);
    EXPECT_EQ(actual.volume, expected.volume);
}

// Mirrors CsvLocalCache's internal layout (dataDir/symbol/interval.csv) so
// tests can locate and seed files directly without exposing buildPath().
std::filesystem::path expectedFilePath(const std::filesystem::path &dataDir,
                                       std::string_view symbol,
                                       std::string_view interval) {
    return dataDir / symbol / std::format("{}.csv", interval);
}

void writeRawCsv(const std::filesystem::path &path,
                 const std::vector<std::string> &dataLines) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::trunc);
    file << "timestamp,open,high,low,close,volume\n";
    for (const auto &line : dataLines) {
        file << line << '\n';
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Fixture — each test gets its own scratch directory, wiped before and after.
// ---------------------------------------------------------------------------

class CsvLocalCacheTest : public Test {
  protected:
    void SetUp() override {
        const auto *testInfo = UnitTest::GetInstance()->current_test_info();
        m_dataDir = std::filesystem::temp_directory_path() /
                    std::format("CsvLocalCacheTest_{}", testInfo->name());
        std::filesystem::remove_all(
            m_dataDir); // clear stale leftovers from a prior crash
        m_cache = std::make_unique<CsvLocalCache>(m_dataDir);
    }

    void TearDown() override {
        std::filesystem::remove_all(m_dataDir); // leave no trace, pass or fail
    }

    std::filesystem::path m_dataDir;
    std::unique_ptr<CsvLocalCache> m_cache;
};

// ---------------------------------------------------------------------------
// Round trip
// ---------------------------------------------------------------------------

TEST_F(CsvLocalCacheTest, SaveThenLoad_RoundTrip_PreservesAllFields) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 100.0, 105.5, 99.25, 102.75, 123456),
        makeCandle("2024-01-02", 102.75, 108.0, 101.0, 107.5, 654321),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 2u);
    expectCandleEq(loaded[0], toSave[0]);
    expectCandleEq(loaded[1], toSave[1]);
}

TEST_F(CsvLocalCacheTest, Load_HandlesLargeVolumeAndFractionalPrices) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 123.456789, 124.987654, 122.111111, 123.999999,
                   1234567890L),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 1u);
    expectCandleEq(loaded[0], toSave[0]);
}

TEST_F(CsvLocalCacheTest, MultipleSaveLoadCycles_AccumulateViaMerge) {
    for (int i = 0; i < 5; ++i) {
        const std::string ts = std::format("2024-01-{:02}", i + 1);
        const std::vector<PriceCandle> toSave{makeCandle(ts, i, i, i, i, i)};
        ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

        std::vector<PriceCandle> loaded;
        ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
        ASSERT_EQ(loaded.size(), static_cast<size_t>(i + 1));
        EXPECT_EQ(loaded.back().timestamp, ts);
    }
}

// ---------------------------------------------------------------------------
// trySave — file/directory layout and isolation
// ---------------------------------------------------------------------------

TEST_F(CsvLocalCacheTest, Save_CreatesSymbolSubdirectoryAndIntervalFile) {
    const std::vector<PriceCandle> candles{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", candles));

    EXPECT_TRUE(std::filesystem::is_directory(m_dataDir / "AAPL"));
    EXPECT_TRUE(
        std::filesystem::exists(expectedFilePath(m_dataDir, "AAPL", "1day")));
}

TEST_F(CsvLocalCacheTest, Save_EmptyCandles_ReturnsTrue_ButLoadFindsNoData) {
    const std::vector<PriceCandle> empty;
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", empty));

    std::vector<PriceCandle> loaded;
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    EXPECT_TRUE(loaded.empty());
}

TEST_F(CsvLocalCacheTest, Save_DisjointRanges_MergesRatherThanOverwriting) {
    // DataBroker only passes the candles it just fetched, which may cover a
    // narrower range than what's already cached — a second save must not
    // wipe out the first save's data.
    const std::vector<PriceCandle> first{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    const std::vector<PriceCandle> second{
        makeCandle("2024-02-01", 2, 2, 2, 2, 2)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", first));
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", second));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-01");
    EXPECT_EQ(loaded[1].timestamp, "2024-02-01");
}

TEST_F(CsvLocalCacheTest, Save_OverlappingTimestamp_NewValueOverwritesOld) {
    const std::vector<PriceCandle> first{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    const std::vector<PriceCandle> second{
        makeCandle("2024-01-01", 9, 9, 9, 9, 9)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", first));
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", second));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    ASSERT_EQ(loaded.size(), 1u);
    expectCandleEq(loaded[0], second[0]);
}

TEST_F(CsvLocalCacheTest, Save_MergedResult_IsSortedByTimestamp) {
    const std::vector<PriceCandle> first{
        makeCandle("2024-01-03", 3, 3, 3, 3, 3)};
    const std::vector<PriceCandle> second{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1),
        makeCandle("2024-01-02", 2, 2, 2, 2, 2),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", first));
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", second));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-01");
    EXPECT_EQ(loaded[1].timestamp, "2024-01-02");
    EXPECT_EQ(loaded[2].timestamp, "2024-01-03");
}

TEST_F(CsvLocalCacheTest, Save_DifferentSymbols_StoredSeparately) {
    const std::vector<PriceCandle> aaplData{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    const std::vector<PriceCandle> msftData{
        makeCandle("2024-01-01", 2, 2, 2, 2, 2)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", aaplData));
    ASSERT_TRUE(m_cache->trySave("MSFT", "1day", msftData));

    std::vector<PriceCandle> aapl, msft;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", aapl));
    ASSERT_TRUE(m_cache->tryLoad("MSFT", "1day", "2000-01-01", msft));

    ASSERT_EQ(aapl.size(), 1u);
    ASSERT_EQ(msft.size(), 1u);
    EXPECT_DOUBLE_EQ(aapl[0].open, 1.0);
    EXPECT_DOUBLE_EQ(msft[0].open, 2.0);
}

TEST_F(CsvLocalCacheTest, Save_DifferentIntervalsSameSymbol_StoredSeparately) {
    const std::vector<PriceCandle> dailyData{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    const std::vector<PriceCandle> hourlyData{
        makeCandle("2024-01-01", 2, 2, 2, 2, 2)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", dailyData));
    ASSERT_TRUE(m_cache->trySave("AAPL", "1h", hourlyData));

    std::vector<PriceCandle> daily, hourly;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", daily));
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1h", "2000-01-01", hourly));

    ASSERT_EQ(daily.size(), 1u);
    ASSERT_EQ(hourly.size(), 1u);
    EXPECT_DOUBLE_EQ(daily[0].open, 1.0);
    EXPECT_DOUBLE_EQ(hourly[0].open, 2.0);
}

TEST_F(CsvLocalCacheTest, SaveThenLoad_SymbolWithSpecialCharacters) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    ASSERT_TRUE(m_cache->trySave("BRK.B", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("BRK.B", "1day", "2000-01-01", loaded));
    ASSERT_EQ(loaded.size(), 1u);
}

TEST_F(CsvLocalCacheTest,
       Save_ReturnsFalse_WhenSymbolDirectoryPathIsBlockedByFile) {
    std::filesystem::create_directories(m_dataDir);
    // Pre-create a regular file where the "AAPL" subdirectory needs to go,
    // forcing create_directories() to fail.
    std::ofstream blocker(m_dataDir / "AAPL");
    blocker << "not a directory";
    blocker.close();

    const std::vector<PriceCandle> candles{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    EXPECT_FALSE(m_cache->trySave("AAPL", "1day", candles));
}

// ---------------------------------------------------------------------------
// tryLoad — startDate filtering
// ---------------------------------------------------------------------------

TEST_F(CsvLocalCacheTest, Load_NoCacheFile_ReturnsFalse_OutCandlesUnchanged) {
    std::vector<PriceCandle> loaded{makeCandle("preexisting", 9, 9, 9, 9, 9)};
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].timestamp, "preexisting");
}

TEST_F(CsvLocalCacheTest, Load_FiltersOutCandlesBeforeStartDate) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1),
        makeCandle("2024-01-02", 2, 2, 2, 2, 2),
        makeCandle("2024-01-03", 3, 3, 3, 3, 3),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2024-01-02", loaded));

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-02");
    EXPECT_EQ(loaded[1].timestamp, "2024-01-03");
}

TEST_F(CsvLocalCacheTest,
       Load_StartDateExactlyMatchesACandle_InclusiveBoundary) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1),
        makeCandle("2024-01-02", 2, 2, 2, 2, 2),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2024-01-02", loaded));

    ASSERT_EQ(loaded.size(), 1u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-02");
}

TEST_F(CsvLocalCacheTest, Load_StartDateAfterAllData_ReturnsFalse) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2099-01-01", loaded));
    EXPECT_TRUE(loaded.empty());
}

TEST_F(CsvLocalCacheTest, Load_StartDateBeforeAllData_ReturnsAllCandles) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-01", 1, 1, 1, 1, 1),
        makeCandle("2024-01-02", 2, 2, 2, 2, 2),
    };
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "1900-01-01", loaded));
    EXPECT_EQ(loaded.size(), 2u);
}

TEST_F(CsvLocalCacheTest,
       Load_AppendsToNonEmptyOutCandles_PreservesExistingContent) {
    const std::vector<PriceCandle> toSave{
        makeCandle("2024-01-02", 1, 1, 1, 1, 1)};
    ASSERT_TRUE(m_cache->trySave("AAPL", "1day", toSave));

    std::vector<PriceCandle> loaded{makeCandle("2024-01-01", 0, 0, 0, 0, 0)};
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].timestamp,
              "2024-01-01"); // pre-existing entry untouched
    EXPECT_EQ(loaded[1].timestamp, "2024-01-02"); // newly loaded entry appended
}

// ---------------------------------------------------------------------------
// tryLoad — malformed / unexpected file content
// ---------------------------------------------------------------------------

TEST_F(CsvLocalCacheTest, Load_SkipsMalformedLines_ContinuesWithValidOnes) {
    writeRawCsv(expectedFilePath(m_dataDir, "AAPL", "1day"),
                {
                    "2024-01-01,100,105,99,102,1000",
                    "this,is,not,a,valid,row",
                    "2024-01-03,103,107,101,106,3000",
                });

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 2u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-01");
    EXPECT_EQ(loaded[1].timestamp, "2024-01-03");
}

TEST_F(CsvLocalCacheTest, Load_SkipsBlankLines) {
    writeRawCsv(expectedFilePath(m_dataDir, "AAPL", "1day"),
                {
                    "2024-01-01,100,105,99,102,1000",
                    "",
                    "2024-01-02,101,106,100,103,2000",
                });

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    ASSERT_EQ(loaded.size(), 2u);
}

TEST_F(CsvLocalCacheTest, Load_OnlyMalformedLines_ReturnsFalse) {
    writeRawCsv(expectedFilePath(m_dataDir, "AAPL", "1day"),
                {
                    "garbage,not,a,valid,candle,row",
                });

    std::vector<PriceCandle> loaded;
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    EXPECT_TRUE(loaded.empty());
}

TEST_F(CsvLocalCacheTest, Load_PreservesFileOrder_DoesNotResort) {
    // Deliberately out of chronological order — the cache must not re-sort;
    // sorting is DataBroker's responsibility, not the storage layer's.
    writeRawCsv(expectedFilePath(m_dataDir, "AAPL", "1day"),
                {
                    "2024-01-03,3,3,3,3,3",
                    "2024-01-01,1,1,1,1,1",
                    "2024-01-02,2,2,2,2,2",
                });

    std::vector<PriceCandle> loaded;
    ASSERT_TRUE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));

    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_EQ(loaded[0].timestamp, "2024-01-03");
    EXPECT_EQ(loaded[1].timestamp, "2024-01-01");
    EXPECT_EQ(loaded[2].timestamp, "2024-01-02");
}

TEST_F(CsvLocalCacheTest, Load_HeaderOnlyFile_ReturnsFalse) {
    writeRawCsv(expectedFilePath(m_dataDir, "AAPL", "1day"), {});

    std::vector<PriceCandle> loaded;
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    EXPECT_TRUE(loaded.empty());
}

TEST_F(CsvLocalCacheTest, Load_ReturnsFalse_WhenPathIsDirectoryNotFile) {
    // A directory sits where the cache file should be.
    std::filesystem::create_directories(
        expectedFilePath(m_dataDir, "AAPL", "1day"));

    std::vector<PriceCandle> loaded;
    EXPECT_FALSE(m_cache->tryLoad("AAPL", "1day", "2000-01-01", loaded));
    EXPECT_TRUE(loaded.empty());
}
