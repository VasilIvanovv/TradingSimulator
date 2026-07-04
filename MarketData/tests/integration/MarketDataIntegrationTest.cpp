#include <gtest/gtest.h>

#include "CsvLocalCache.hpp"
#include "DataBroker.hpp"
#include "TwelveDataProvider.hpp"

#include <httplib.h>
#include <atomic>
#include <filesystem>
#include <format>
#include <thread>

using namespace trading;

// ---------------------------------------------------------------------------
// TwelveDataProvider — JSON helpers
// Prices and volumes are string-encoded, as the real API returns them.
// ---------------------------------------------------------------------------

namespace TwelveData {

static std::string makeResponse(
    std::initializer_list<std::pair<std::string, double>> candles)
{
    std::string values = "[";
    bool first = true;
    for (const auto& [ts, price] : candles) {
        if (!first) values += ",";
        first = false;
        values += std::format(
            R"({{"datetime":"{}","open":"{:.4f}","high":"{:.4f}","low":"{:.4f}","close":"{:.4f}","volume":"1000000"}})",
            ts, price, price, price, price);
    }
    values += "]";
    return std::format(R"({{"status":"ok","values":{}}})", values);
}

static std::string makeErrorResponse(std::string_view message) {
    return std::format(R"({{"status":"error","message":"{}"}})", message);
}

} // namespace TwelveData

// ---------------------------------------------------------------------------
// TwelveDataProvider fixture
// ---------------------------------------------------------------------------

class TwelveDataIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_server.Get("/time_series",
            [this](const httplib::Request&, httplib::Response& res) {
                ++m_requestCount;
                res.set_content(m_responseBody, "application/json");
            });

        m_port = m_server.bind_to_any_port("127.0.0.1");
        ASSERT_GT(m_port, 0) << "Failed to bind mock HTTP server";
        m_serverThread = std::thread([this] { m_server.listen_after_bind(); });

        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_dataDir = std::filesystem::temp_directory_path() /
                    std::format("MDIntegration_{}", info->name());
        std::filesystem::remove_all(m_dataDir);
    }

    void TearDown() override {
        m_server.stop();
        if (m_serverThread.joinable())
            m_serverThread.join();
        std::filesystem::remove_all(m_dataDir);
    }

    DataBroker makeBroker() {
        std::vector<std::unique_ptr<IDataProvider>> providers;
        providers.push_back(std::make_unique<TwelveDataProvider>(
            "test-key", std::format("http://127.0.0.1:{}", m_port)));
        return DataBroker(std::move(providers),
                          std::make_unique<CsvLocalCache>(m_dataDir));
    }

    httplib::Server       m_server;
    std::thread           m_serverThread;
    int                   m_port{};
    std::atomic<int>      m_requestCount{};
    std::string           m_responseBody;
    std::filesystem::path m_dataDir;
};

// ---------------------------------------------------------------------------
// TwelveDataProvider tests
// ---------------------------------------------------------------------------

TEST_F(TwelveDataIntegrationTest, CacheMiss_ParsesAndReturnsCandles) {
    m_responseBody = TwelveData::makeResponse({
        {"2024-01-03", 187.0},   // provider returns newest-first (like real API)
        {"2024-01-02", 185.0},
        {"2024-01-01", 182.0},
    });

    auto broker = makeBroker();
    auto result = broker.getHistory("AAPL", "1day", "2024-01-01");

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3u);
    EXPECT_EQ(m_requestCount, 1);

    // DataBroker sorts ascending by timestamp
    EXPECT_EQ((*result)[0].timestamp, "2024-01-01");
    EXPECT_DOUBLE_EQ((*result)[0].close, 182.0);
    EXPECT_EQ((*result)[2].timestamp, "2024-01-03");
    EXPECT_DOUBLE_EQ((*result)[2].close, 187.0);
}

// Second call with the same broker and same args hits the on-disk cache;
// the mock server must not receive a second request.
TEST_F(TwelveDataIntegrationTest, SecondCall_HitsCache_ServerNotQueried) {
    m_responseBody = TwelveData::makeResponse({
        {"2024-01-01", 182.0},
        {"2024-01-02", 185.0},
    });

    auto broker = makeBroker();
    ASSERT_TRUE(broker.getHistory("AAPL", "1day", "2024-01-01").has_value());
    EXPECT_EQ(m_requestCount, 1);

    auto result = broker.getHistory("AAPL", "1day", "2024-01-01");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);
    EXPECT_EQ(m_requestCount, 1);  // still 1 — cache served the second call
}

// Data written to disk by one DataBroker instance is picked up by
// a completely separate instance pointing at the same cache directory.
TEST_F(TwelveDataIntegrationTest, DataSurvivesAcrossBrokerInstances) {
    m_responseBody = TwelveData::makeResponse({
        {"2024-01-01", 152.0},
        {"2024-01-02", 154.0},
    });

    { // first instance — fetches from provider, writes to disk
        auto broker = makeBroker();
        ASSERT_TRUE(broker.getHistory("AAPL", "1day", "2024-01-01").has_value());
    }
    EXPECT_EQ(m_requestCount, 1);

    { // second instance — cache on disk must serve it, no server hit
        auto broker = makeBroker();
        auto result = broker.getHistory("AAPL", "1day", "2024-01-01");
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result->size(), 2u);
        EXPECT_DOUBLE_EQ((*result)[0].close, 152.0);
        EXPECT_DOUBLE_EQ((*result)[1].close, 154.0);
    }
    EXPECT_EQ(m_requestCount, 1);  // second broker never hit the server
}

// The provider returns a Twelve Data API error; the whole call returns nullopt
// and nothing is written to the cache.
TEST_F(TwelveDataIntegrationTest, ApiError_ReturnsNullopt_NothingCached) {
    m_responseBody = TwelveData::makeErrorResponse("You have run out of API credits for the current minute.");

    auto broker = makeBroker();
    EXPECT_FALSE(broker.getHistory("AAPL", "1day", "2024-01-01").has_value());
    EXPECT_EQ(m_requestCount, 1);

    // A retry should go to the server again — nothing was cached
    m_responseBody = TwelveData::makeResponse({ {"2024-01-01", 182.0} });
    auto result = broker.getHistory("AAPL", "1day", "2024-01-01");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(m_requestCount, 2);
}

// startDate filtering is enforced by the cache layer on the second call;
// the full pipeline — fetch, persist, reload with filter — is exercised here.
TEST_F(TwelveDataIntegrationTest, StartDateFiltering_WorksEndToEnd) {
    m_responseBody = TwelveData::makeResponse({
        {"2024-03-01", 202.0},
        {"2024-02-01", 192.0},
        {"2024-01-01", 182.0},
    });

    auto broker = makeBroker();

    // First call fetches all three candles and writes them to disk
    (void)broker.getHistory("AAPL", "1day", "2024-01-01");
    EXPECT_EQ(m_requestCount, 1);

    // Second call hits cache, filtered to candles on or after 2024-02-01
    auto result = broker.getHistory("AAPL", "1day", "2024-02-01");
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);
    EXPECT_EQ((*result)[0].timestamp, "2024-02-01");
    EXPECT_EQ((*result)[1].timestamp, "2024-03-01");
    EXPECT_EQ(m_requestCount, 1);  // served from cache
}
