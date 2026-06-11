#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "data/DataBroker.hpp"
#include "data/IDataProvider.hpp"
#include "data/ILocalCache.hpp"

using namespace trading;
using namespace testing;

// ---------------------------------------------------------------------------
// Mocks
// ---------------------------------------------------------------------------

class MockDataProvider : public IDataProvider {
public:
    MOCK_METHOD(bool, tryGetHistory,
        (std::string_view symbol, std::string_view interval,
         std::string_view startDate, std::vector<PriceCandle>& outCandles),
        (override));
};

class MockLocalCache : public ILocalCache {
public:
    MOCK_METHOD(bool, trySave,
        (std::string_view symbol, std::string_view interval,
         std::span<const PriceCandle> candles),
        (override));
    MOCK_METHOD(bool, tryLoad,
        (std::string_view symbol, std::string_view interval,
         std::string_view startDate, std::vector<PriceCandle>& outCandles),
        (override));
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static PriceCandle makeCandle(std::string timestamp, double close = 100.0) {
    return PriceCandle{std::move(timestamp), close, close, close, close, 0};
}

static constexpr std::string_view kStart = "2024-01-01";

// Captures the span written to trySave into a vector for later inspection.
static auto captureSave(std::vector<PriceCandle>& out) {
    return [&out](auto /*symbol*/, auto /*interval*/,
                  std::span<const PriceCandle> s) {
        out.assign(s.begin(), s.end());
        return true;
    };
}

// ---------------------------------------------------------------------------
// Fixture  (two providers + cache)
// ---------------------------------------------------------------------------

class DataBrokerTest : public Test {
protected:
    void SetUp() override {
        auto cache = std::make_unique<MockLocalCache>();
        m_cachePtr = cache.get();

        auto p1 = std::make_unique<MockDataProvider>();
        auto p2 = std::make_unique<MockDataProvider>();
        m_p1 = p1.get();
        m_p2 = p2.get();

        std::vector<std::unique_ptr<IDataProvider>> providers;
        providers.push_back(std::move(p1));
        providers.push_back(std::move(p2));

        m_broker = std::make_unique<DataBroker>(std::move(providers), std::move(cache));
    }

    MockLocalCache*             m_cachePtr{};
    MockDataProvider*           m_p1{};
    MockDataProvider*           m_p2{};
    std::unique_ptr<DataBroker> m_broker;
};

// ---------------------------------------------------------------------------
// Cache hit
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CacheHit_ReturnsCachedData_ProvidersNotCalled) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-15"));
            return true;
        });
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).Times(0);
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).Times(0);
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).Times(0);

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->front().timestamp, "2024-01-15");
}

TEST_F(DataBrokerTest, CacheHit_ReturnsOnlyDataFromStartDate_NoFurtherFiltering) {
    // The cache is responsible for filtering; DataBroker uses whatever it returns as-is.
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            out.push_back(makeCandle("2024-01-02"));
            return true;
        });
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).Times(0);
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).Times(0);
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).Times(0);

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 2u);
    EXPECT_EQ((*result)[0].timestamp, "2024-01-01");
    EXPECT_EQ((*result)[1].timestamp, "2024-01-02");
}

TEST_F(DataBrokerTest, CacheMiss_FallsThroughToProviders) {
    // Cache returns false (no data from startDate) — broker goes to providers.
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-05"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(true));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->front().timestamp, "2024-01-05");
}

// ---------------------------------------------------------------------------
// Cache miss — provider exhaustion
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CacheMiss_AllProvidersFail_ReturnsNullopt) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).Times(0);

    EXPECT_FALSE(m_broker->getHistory("AAPL", "1day", kStart).has_value());
}

// ---------------------------------------------------------------------------
// Cache miss — single provider contributes
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CacheMiss_OnlyFirstProviderSucceeds_ReturnsDataAndPersists) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(true));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->front().timestamp, "2024-01-01");
}

TEST_F(DataBrokerTest, CacheMiss_OnlySecondProviderSucceeds_ReturnsDataAndPersists) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-02"));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(true));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1u);
    EXPECT_EQ(result->front().timestamp, "2024-01-02");
}

// ---------------------------------------------------------------------------
// Cooperative fetch — both providers contribute
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CooperativeFetch_BothProvidersSucceed_ResultsMergedAndSorted) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-03"));
            out.push_back(makeCandle("2024-01-01"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-04"));
            out.push_back(makeCandle("2024-01-02"));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(true));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 4u);
    EXPECT_EQ((*result)[0].timestamp, "2024-01-01");
    EXPECT_EQ((*result)[1].timestamp, "2024-01-02");
    EXPECT_EQ((*result)[2].timestamp, "2024-01-03");
    EXPECT_EQ((*result)[3].timestamp, "2024-01-04");
}

TEST_F(DataBrokerTest, CooperativeFetch_OverlappingTimestamps_Deduplicated) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            out.push_back(makeCandle("2024-01-02"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-02"));  // duplicate
            out.push_back(makeCandle("2024-01-03"));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(true));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 3u);
    EXPECT_EQ((*result)[0].timestamp, "2024-01-01");
    EXPECT_EQ((*result)[1].timestamp, "2024-01-02");
    EXPECT_EQ((*result)[2].timestamp, "2024-01-03");
}

// ---------------------------------------------------------------------------
// Cache save failure — data still returned to caller
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CacheSaveFails_DataStillReturned) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(Return(false));

    auto result = m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1u);
}

// ---------------------------------------------------------------------------
// Edge cases (no cache / no providers)
// ---------------------------------------------------------------------------

TEST(DataBrokerEdgeTest, NullCache_ProviderSucceeds_ReturnsCandles) {
    auto mockProvider = std::make_unique<MockDataProvider>();
    auto* providerPtr = mockProvider.get();

    std::vector<std::unique_ptr<IDataProvider>> providers;
    providers.push_back(std::move(mockProvider));

    DataBroker broker(std::move(providers), nullptr);

    EXPECT_CALL(*providerPtr, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            return true;
        });

    auto result = broker.getHistory("AAPL", "1day", kStart);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 1u);
}

TEST(DataBrokerEdgeTest, NoProviders_ReturnsNullopt) {
    DataBroker broker({}, nullptr);
    EXPECT_FALSE(broker.getHistory("AAPL", "1day", kStart).has_value());
}

// ---------------------------------------------------------------------------
// Argument forwarding
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, ForwardsSymbolIntervalAndStartDate_ToProviders) {
    EXPECT_CALL(*m_cachePtr, tryLoad("MSFT", "1h", "2023-06-01", _))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory("MSFT", "1h", "2023-06-01", _))
        .WillOnce(Return(false));
    EXPECT_CALL(*m_p2, tryGetHistory("MSFT", "1h", "2023-06-01", _))
        .WillOnce(Return(false));

    (void)m_broker->getHistory("MSFT", "1h", "2023-06-01");
}

TEST_F(DataBrokerTest, ForwardsSymbolAndInterval_ToCacheSave) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave("MSFT", "1h", _)).WillOnce(Return(true));

    (void)m_broker->getHistory("MSFT", "1h", kStart);
}

// ---------------------------------------------------------------------------
// Cache write content
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, CacheSaveContent_SingleProvider_SortedBeforePersisting) {
    std::vector<PriceCandle> saved;
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-03", 150.0));
            out.push_back(makeCandle("2024-01-01", 100.0));
            out.push_back(makeCandle("2024-01-02", 120.0));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(captureSave(saved));

    (void)m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_EQ(saved.size(), 3u);
    EXPECT_EQ(saved[0].timestamp, "2024-01-01");
    EXPECT_DOUBLE_EQ(saved[0].close, 100.0);
    EXPECT_EQ(saved[1].timestamp, "2024-01-02");
    EXPECT_DOUBLE_EQ(saved[1].close, 120.0);
    EXPECT_EQ(saved[2].timestamp, "2024-01-03");
    EXPECT_DOUBLE_EQ(saved[2].close, 150.0);
}

TEST_F(DataBrokerTest, CacheSaveContent_TwoProviders_MergedAndSorted) {
    std::vector<PriceCandle> saved;
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-04", 200.0));
            out.push_back(makeCandle("2024-01-02", 120.0));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-03", 150.0));
            out.push_back(makeCandle("2024-01-01", 100.0));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(captureSave(saved));

    (void)m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_EQ(saved.size(), 4u);
    EXPECT_EQ(saved[0].timestamp, "2024-01-01");
    EXPECT_EQ(saved[1].timestamp, "2024-01-02");
    EXPECT_EQ(saved[2].timestamp, "2024-01-03");
    EXPECT_EQ(saved[3].timestamp, "2024-01-04");
}

TEST_F(DataBrokerTest, CacheSaveContent_OverlappingTimestamps_Deduplicated) {
    std::vector<PriceCandle> saved;
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-01"));
            out.push_back(makeCandle("2024-01-02"));
            out.push_back(makeCandle("2024-01-03"));
            return true;
        });
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-01-02"));  // duplicate
            out.push_back(makeCandle("2024-01-03"));  // duplicate
            out.push_back(makeCandle("2024-01-04"));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(captureSave(saved));

    (void)m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_EQ(saved.size(), 4u);
    EXPECT_EQ(saved[0].timestamp, "2024-01-01");
    EXPECT_EQ(saved[1].timestamp, "2024-01-02");
    EXPECT_EQ(saved[2].timestamp, "2024-01-03");
    EXPECT_EQ(saved[3].timestamp, "2024-01-04");
}

TEST_F(DataBrokerTest, CacheSaveContent_OnlySuccessfulProviderDataPersisted) {
    std::vector<PriceCandle> saved;
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _))
        .WillOnce([](auto, auto, auto, std::vector<PriceCandle>& out) {
            out.push_back(makeCandle("2024-02-01", 300.0));
            out.push_back(makeCandle("2024-02-02", 310.0));
            return true;
        });
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).WillOnce(captureSave(saved));

    (void)m_broker->getHistory("AAPL", "1day", kStart);

    ASSERT_EQ(saved.size(), 2u);
    EXPECT_EQ(saved[0].timestamp, "2024-02-01");
    EXPECT_DOUBLE_EQ(saved[0].close, 300.0);
    EXPECT_EQ(saved[1].timestamp, "2024-02-02");
    EXPECT_DOUBLE_EQ(saved[1].close, 310.0);
}

// ---------------------------------------------------------------------------
// Provider / cache contract violations
// ---------------------------------------------------------------------------

TEST_F(DataBrokerTest, ProviderReturnsTrueButAddsNoCandles_ReturnsNullopt_NoSave) {
    EXPECT_CALL(*m_cachePtr, tryLoad(_, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*m_p1, tryGetHistory(_, _, _, _)).WillOnce(Return(true));  // adds nothing
    EXPECT_CALL(*m_p2, tryGetHistory(_, _, _, _)).WillOnce(Return(true));  // adds nothing
    EXPECT_CALL(*m_cachePtr, trySave(_, _, _)).Times(0);

    EXPECT_FALSE(m_broker->getHistory("AAPL", "1day", kStart).has_value());
}

