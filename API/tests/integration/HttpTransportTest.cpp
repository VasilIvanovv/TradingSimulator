#include <gtest/gtest.h>

#include "ApiHandler.hpp"
#include "AuthHandler.hpp"
#include "CsvLocalCache.hpp"
#include "DataBroker.hpp"
#include "HttpTransport.hpp"
#include "JwtService.hpp"
#include "LogoManager.hpp"
#include "PaperAccount.hpp"
#include "SqliteAccountManager.hpp"
#include "SqliteAccountStore.hpp"
#include "SqliteUserStore.hpp"
#include "SymbolManager.hpp"
#include "TradingController.hpp"

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <sodium.h>
#include <filesystem>
#include <thread>

using namespace trading;
using json = nlohmann::json;

static constexpr int         kPort     = 18081;
static constexpr const char* kBindHost = "127.0.0.1";

// ---------------------------------------------------------------------------
// Fixture — starts the HTTP server once for the whole test suite.
// ---------------------------------------------------------------------------
class HttpTransportTest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        s_userStore      = std::make_unique<SqliteUserStore>(":memory:");
        s_jwtService     = std::make_unique<JwtService>("integration-test-secret");
        s_accountManager = std::make_unique<SqliteAccountManager>(":memory:");

        s_handler = std::make_unique<ApiHandler>(
            [](int accountId) {
                return std::make_unique<TradingController>(
                    std::make_unique<PaperAccount>(
                        10'000.0,
                        std::make_unique<SqliteAccountStore>(":memory:", accountId)),
                    std::make_unique<DataBroker>(
                        std::vector<std::unique_ptr<IDataProvider>>{},
                        nullptr),
                    "1day", "2025-01-01");
            },
            *s_accountManager);

        s_auth = std::make_unique<AuthHandler>(*s_userStore, *s_jwtService,
                                               HashingParams{crypto_pwhash_OPSLIMIT_MIN,
                                                             crypto_pwhash_MEMLIMIT_MIN});

        // Minimal no-op stubs: tests don't exercise /symbols or /logos.
        s_symbolManager = std::make_unique<SymbolManager>("nonexistent_symbols.json");
        s_logoCache     = std::make_unique<LogoManager>(
            "test_logo_cache",
            [&]() -> std::vector<std::string> { return {}; });

        s_transport = std::make_unique<HttpTransport>(
            *s_handler, *s_auth, *s_jwtService,
            *s_symbolManager, *s_logoCache, kPort, kBindHost);
        s_transport->startAsync();

        // Poll until the server accepts connections (up to 5 s in Debug builds).
        // Both connection timeout AND read timeout are bounded so the probe
        // never hangs if the server accepts the TCP connection before its
        // request-processing loop is fully up.
        bool ready = false;
        for (int i = 0; i < 50 && !ready; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            auto probe = httplib::Client(kBindHost, kPort);
            probe.set_connection_timeout(0, 200'000); // 200 ms
            probe.set_read_timeout(1, 0);             // 1 s
            if (probe.Get("/account")) ready = true;  // 401 counts as ready
        }
        if (!ready) throw std::runtime_error("HttpTransport did not start within 5 s");
    }

    static void TearDownTestSuite() {
        s_transport->stop();
        s_transport.reset();
        s_logoCache.reset();
        s_symbolManager.reset();
        s_auth.reset();
        s_handler.reset();
        s_accountManager.reset();
        s_jwtService.reset();
        s_userStore.reset();
    }

    static httplib::Client makeClient() {
        httplib::Client c(kBindHost, kPort);
        c.set_read_timeout(5, 0); // 5 s per request — fail the test, not hang
        return c;
    }

    // Register a fresh user and return their token.
    static std::string registerAndGetToken(const std::string& username,
                                           const std::string& password = "testpass") {
        auto client = makeClient();
        const auto res = client.Post("/auth/register",
                                     json{{"username", username},
                                          {"password", password}}.dump(),
                                     "application/json");
        if (!res) throw std::runtime_error("registerAndGetToken: server unreachable");
        return json::parse(res->body).at("token").get<std::string>();
    }

    // Call GET /accounts (which auto-creates the Default account) and return
    // the first account's id — needed as ?accountId= on trading endpoints.
    static int getDefaultAccountId(const std::string& token) {
        auto client = makeClient();
        httplib::Headers headers{{"Authorization", "Bearer " + token}};
        const auto res = client.Get("/accounts", headers);
        if (!res) throw std::runtime_error("getDefaultAccountId: server unreachable");
        return json::parse(res->body).at(0).at("id").get<int>();
    }

    static std::unique_ptr<SqliteUserStore>      s_userStore;
    static std::unique_ptr<JwtService>           s_jwtService;
    static std::unique_ptr<SqliteAccountManager> s_accountManager;
    static std::unique_ptr<ApiHandler>           s_handler;
    static std::unique_ptr<AuthHandler>          s_auth;
    static std::unique_ptr<SymbolManager>        s_symbolManager;
    static std::unique_ptr<LogoManager>          s_logoCache;
    static std::unique_ptr<HttpTransport>        s_transport;
};

std::unique_ptr<SqliteUserStore>      HttpTransportTest::s_userStore;
std::unique_ptr<JwtService>           HttpTransportTest::s_jwtService;
std::unique_ptr<SqliteAccountManager> HttpTransportTest::s_accountManager;
std::unique_ptr<ApiHandler>           HttpTransportTest::s_handler;
std::unique_ptr<AuthHandler>          HttpTransportTest::s_auth;
std::unique_ptr<SymbolManager>        HttpTransportTest::s_symbolManager;
std::unique_ptr<LogoManager>          HttpTransportTest::s_logoCache;
std::unique_ptr<HttpTransport>        HttpTransportTest::s_transport;

// ---------------------------------------------------------------------------
// Auth endpoint tests
// ---------------------------------------------------------------------------

TEST_F(HttpTransportTest, Register_ValidCredentials_Returns200WithToken) {
    auto client = makeClient();
    const auto res = client.Post("/auth/register",
                                 json{{"username", "user_reg1"},
                                      {"password", "pass"}}.dump(),
                                 "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const auto body = json::parse(res->body);
    EXPECT_FALSE(body.at("token").get<std::string>().empty());
}

TEST_F(HttpTransportTest, Register_DuplicateUsername_Returns400) {
    auto client = makeClient();
    client.Post("/auth/register",
                json{{"username", "user_dup"}, {"password", "pass"}}.dump(),
                "application/json");
    const auto res = client.Post("/auth/register",
                                 json{{"username", "user_dup"},
                                      {"password", "pass"}}.dump(),
                                 "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}

TEST_F(HttpTransportTest, Login_CorrectCredentials_Returns200WithToken) {
    auto client = makeClient();
    client.Post("/auth/register",
                json{{"username", "user_login"}, {"password", "mypass"}}.dump(),
                "application/json");

    const auto res = client.Post("/auth/login",
                                 json{{"username", "user_login"},
                                      {"password", "mypass"}}.dump(),
                                 "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_FALSE(json::parse(res->body).at("token").get<std::string>().empty());
}

TEST_F(HttpTransportTest, Login_WrongPassword_Returns401) {
    auto client = makeClient();
    client.Post("/auth/register",
                json{{"username", "user_wrongpw"}, {"password", "realpass"}}.dump(),
                "application/json");

    const auto res = client.Post("/auth/login",
                                 json{{"username", "user_wrongpw"},
                                      {"password", "wrongpass"}}.dump(),
                                 "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

// ---------------------------------------------------------------------------
// Auth middleware tests
// ---------------------------------------------------------------------------

TEST_F(HttpTransportTest, GetAccount_NoToken_Returns401) {
    auto client = makeClient();
    const auto res = client.Get("/account");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(HttpTransportTest, GetAccount_InvalidToken_Returns401) {
    auto client = makeClient();
    httplib::Headers headers{{"Authorization", "Bearer not.a.real.token"}};
    const auto res = client.Get("/account", headers);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST_F(HttpTransportTest, GetAccount_ValidToken_Returns200) {
    const auto token     = registerAndGetToken("user_acct");
    const auto accountId = getDefaultAccountId(token);
    auto client = makeClient();
    httplib::Headers headers{{"Authorization", "Bearer " + token}};
    const auto res = client.Get("/account?accountId=" + std::to_string(accountId), headers);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    const auto body = json::parse(res->body);
    EXPECT_DOUBLE_EQ(body.at("cash").get<double>(), 10000.0);
}

TEST_F(HttpTransportTest, PlaceOrder_ValidToken_Returns200) {
    const auto token     = registerAndGetToken("user_order");
    const auto accountId = getDefaultAccountId(token);
    auto client = makeClient();
    httplib::Headers headers{{"Authorization", "Bearer " + token}};
    const auto res = client.Post(
        "/orders?accountId=" + std::to_string(accountId), headers,
        json{{"symbol","AAPL"},{"side","buy"},{"quantity",10},{"price",100.0}}.dump(),
        "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body).at("status").get<std::string>(), "filled");
}

TEST_F(HttpTransportTest, TwoUsers_HaveSeparateAccounts) {
    const auto token1    = registerAndGetToken("user_sep1");
    const auto token2    = registerAndGetToken("user_sep2");
    const auto accountId1 = getDefaultAccountId(token1);
    const auto accountId2 = getDefaultAccountId(token2);
    auto client = makeClient();

    // user1 buys AAPL on their default account
    httplib::Headers h1{{"Authorization", "Bearer " + token1}};
    client.Post("/orders?accountId=" + std::to_string(accountId1), h1,
                json{{"symbol","AAPL"},{"side","buy"},{"quantity",5},{"price",100.0}}.dump(),
                "application/json");

    // user2's account should be untouched
    httplib::Headers h2{{"Authorization", "Bearer " + token2}};
    const auto res = client.Get("/account?accountId=" + std::to_string(accountId2), h2);
    ASSERT_TRUE(res);
    const auto body = json::parse(res->body);
    EXPECT_DOUBLE_EQ(body.at("cash").get<double>(), 10000.0);
    EXPECT_TRUE(body.at("positions").empty());
}
