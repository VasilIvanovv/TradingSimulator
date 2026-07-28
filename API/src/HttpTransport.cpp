#include "HttpTransport.hpp"
#include "ApiJson.hpp"
#include "Logging.hpp"

namespace trading {

namespace {

constexpr const char* kJson = "application/json";

void respond(httplib::Response& res, int status, const std::string& body) {
    res.status = status;
    res.set_content(body, kJson);
}

void ok(httplib::Response& res, const std::string& body)           { respond(res, 200, body); }
void created(httplib::Response& res, const std::string& body)      { respond(res, 201, body); }
void noContent(httplib::Response& res)                              { res.status = 204; }
void badRequest(httplib::Response& res, const std::string& msg)    { respond(res, 400, toJsonError(msg)); }
void unauthorized(httplib::Response& res, const std::string& msg)  { respond(res, 401, toJsonError(msg)); }
void forbidden(httplib::Response& res, const std::string& msg)     { respond(res, 403, toJsonError(msg)); }
void notFound(httplib::Response& res, const std::string& msg)      { respond(res, 404, toJsonError(msg)); }
void conflict(httplib::Response& res, const std::string& msg)      { respond(res, 409, toJsonError(msg)); }
void serverError(httplib::Response& res, const std::string& msg)   { respond(res, 500, toJsonError(msg)); }
void gatewayError(httplib::Response& res, const std::string& msg)  { respond(res, 502, toJsonError(msg)); }

} // namespace

HttpTransport::HttpTransport(ApiHandler& handler, AuthHandler& auth,
                             JwtService& jwt, SymbolManager& symbols,
                             LogoManager& logos, int port, std::string bindHost)
    : m_apiHandler(handler), m_authHandler(auth), m_jwtService(jwt),
      m_symbolManager(symbols), m_logoManager(logos),
      m_listenPort(port), m_bindHost(std::move(bindHost)) {
    registerRoutes();
}

HttpTransport::~HttpTransport() { stop(); }

void HttpTransport::start() {
    LOG(Info) << "HTTP API listening on port " << m_listenPort;
    m_httpServer.listen(m_bindHost.c_str(), m_listenPort);
}

void HttpTransport::startAsync() {
    m_serverThread = std::thread([this] { start(); });
}

void HttpTransport::stop() {
    m_httpServer.stop();
    if (m_serverThread.joinable())
        m_serverThread.join();
}

std::optional<int> HttpTransport::extractUserId(const httplib::Request& req) const {
    const auto it = req.headers.find("Authorization");
    if (it == req.headers.end()) return std::nullopt;

    const auto& value = it->second;
    constexpr std::string_view prefix = "Bearer ";
    if (value.size() <= prefix.size() || value.substr(0, prefix.size()) != prefix)
        return std::nullopt;

    return m_jwtService.verify(value.substr(prefix.size()));
}

static std::optional<int> extractAccountId(const httplib::Request& req) {
    const auto s = req.get_param_value("accountId");
    if (s.empty()) return std::nullopt;
    try { return std::stoi(s); }
    catch (...) { return std::nullopt; }
}

void HttpTransport::registerRoutes() {
    m_httpServer.set_default_headers(
        {{"Access-Control-Allow-Origin",  "*"},
         {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
         {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});

    m_httpServer.Options(".*", [](const httplib::Request&,
                                   httplib::Response& res) { res.status = 204; });

    // -----------------------------------------------------------------------
    // Public catalogue endpoints
    // -----------------------------------------------------------------------

    m_httpServer.Get("/symbols",
        [this](const httplib::Request&, httplib::Response& res) {
            ok(res, toJson(m_symbolManager.getSymbols()));
        });

    m_httpServer.Get(R"(/logos/([A-Za-z0-9.]+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            const std::string symbol = req.matches[1].str();
            const auto bytes = m_logoManager.get(symbol);
            if (!bytes) { res.status = 404; return; }
            res.set_content(bytes->data(), static_cast<size_t>(bytes->size()), "image/png");
        });

    // -----------------------------------------------------------------------
    // Auth endpoints
    // -----------------------------------------------------------------------

    m_httpServer.Post("/auth/register",
        [this](const httplib::Request& req, httplib::Response& res) {
            try {
                const auto result = m_authHandler.registerUser(parseRegisterRequest(req.body));
                if (!result) { badRequest(res, result.error()); return; }
                ok(res, toJson(*result));
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });

    m_httpServer.Post("/auth/login",
        [this](const httplib::Request& req, httplib::Response& res) {
            try {
                const auto result = m_authHandler.loginUser(parseLoginRequest(req.body));
                if (!result) { unauthorized(res, result.error()); return; }
                ok(res, toJson(*result));
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // -----------------------------------------------------------------------
    // Account management endpoints
    // -----------------------------------------------------------------------

    // GET /accounts  → list accounts (auto-creates Default if none)
    m_httpServer.Get("/accounts",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try { ok(res, toJson(m_apiHandler.listAccounts(*uid))); }
            catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // POST /accounts  body: { name }
    m_httpServer.Post("/accounts",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try {
                const auto r = parseAccountNameRequest(req.body);
                const auto account = m_apiHandler.createAccount(*uid, r.name);
                if (!account) { conflict(res, "Account limit reached (max 10)"); return; }
                created(res, toJson(*account));
            } catch (const std::exception& e) { badRequest(res, e.what()); }
        });

    // PUT /accounts/:id  body: { name }
    m_httpServer.Put(R"(/accounts/(\d+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try {
                const int accountId = std::stoi(req.matches[1].str());
                const auto r = parseAccountNameRequest(req.body);
                if (!m_apiHandler.renameAccount(*uid, accountId, r.name))
                    notFound(res, "Account not found");
                else
                    noContent(res);
            } catch (const std::exception& e) { badRequest(res, e.what()); }
        });

    // DELETE /accounts/:id
    m_httpServer.Delete(R"(/accounts/(\d+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try {
                const int accountId = std::stoi(req.matches[1].str());
                if (!m_apiHandler.deleteAccount(*uid, accountId))
                    notFound(res, "Account not found");
                else
                    noContent(res);
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // POST /accounts/:id/deposit  body: { amount }
    m_httpServer.Post(R"(/accounts/(\d+)/deposit)",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try {
                const int accountId = std::stoi(req.matches[1].str());
                const auto r = parseDepositRequest(req.body);
                if (r.amount <= 0) { badRequest(res, "amount must be positive"); return; }
                m_apiHandler.deposit(*uid, accountId, r.amount);
                noContent(res);
            } catch (const std::exception& e) { badRequest(res, e.what()); }
        });

    // POST /accounts/deposit-all  body: { amount }
    m_httpServer.Post("/accounts/deposit-all",
        [this](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            try {
                const auto r = parseDepositRequest(req.body);
                if (r.amount <= 0) { badRequest(res, "amount must be positive"); return; }
                ok(res, toJson(m_apiHandler.depositAll(*uid, r.amount)));
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // -----------------------------------------------------------------------
    // Trading endpoints — JWT + accountId required
    // -----------------------------------------------------------------------

    auto requireAccount = [](const httplib::Request& req,
                              httplib::Response& res,
                              std::optional<int>& outAccountId) -> bool {
        outAccountId = extractAccountId(req);
        if (!outAccountId) {
            res.status = 400;
            res.set_content(toJsonError("Missing required query param: accountId"),
                            kJson);
            return false;
        }
        return true;
    };

    // GET /account?accountId=X
    m_httpServer.Get("/account",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try { ok(res, toJson(m_apiHandler.getAccount(*uid, *aid))); }
            catch (const std::exception& e) { forbidden(res, e.what()); }
        });

    // POST /orders?accountId=X
    m_httpServer.Post("/orders",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try {
                ok(res, toJson(m_apiHandler.placeOrder(
                    *uid, *aid, parsePlaceOrderRequest(req.body))));
            } catch (const std::exception& e) { badRequest(res, e.what()); }
        });

    // GET /rules?accountId=X
    m_httpServer.Get("/rules",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try { ok(res, toJson(m_apiHandler.getRules(*uid, *aid))); }
            catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // POST /rules?accountId=X
    m_httpServer.Post("/rules",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try {
                m_apiHandler.addRule(*uid, *aid, parseAddRuleRequest(req.body));
                noContent(res);
            } catch (const std::exception& e) { badRequest(res, e.what()); }
        });

    // DELETE /rules/:symbol?accountId=X
    m_httpServer.Delete(R"(/rules/([^/]+))",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try {
                m_apiHandler.removeRules(*uid, *aid, req.matches[1].str());
                noContent(res);
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });

    // GET /history?accountId=X&symbol=AAPL&interval=1day&start=2024-01-01
    m_httpServer.Get("/history",
        [this, requireAccount](const httplib::Request& req, httplib::Response& res) {
            const auto uid = extractUserId(req);
            if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
            std::optional<int> aid;
            if (!requireAccount(req, res, aid)) return;
            try {
                const auto symbol   = req.get_param_value("symbol");
                const auto interval = req.get_param_value("interval");
                const auto start    = req.get_param_value("start");
                if (symbol.empty() || interval.empty() || start.empty()) {
                    badRequest(res, "Required query params: symbol, interval, start");
                    return;
                }
                const auto result = m_apiHandler.getHistory(*uid, *aid, {symbol, interval, start});
                if (!result) gatewayError(res, "Data provider returned no data for " + symbol);
                else         ok(res, toJson(*result));
            } catch (const std::exception& e) { serverError(res, e.what()); }
        });
}

} // namespace trading
