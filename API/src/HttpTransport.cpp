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
void noContent(httplib::Response& res)                              { res.status = 204; }
void badRequest(httplib::Response& res, const std::string& msg)    { respond(res, 400, toJsonError(msg)); }
void unauthorized(httplib::Response& res, const std::string& msg)  { respond(res, 401, toJsonError(msg)); }
void serverError(httplib::Response& res, const std::string& msg)   { respond(res, 500, toJsonError(msg)); }
void gatewayError(httplib::Response& res, const std::string& msg)  { respond(res, 502, toJsonError(msg)); }

} // namespace

HttpTransport::HttpTransport(ApiHandler& handler, AuthHandler& auth,
                             JwtService& jwt, int port)
    : m_handler(handler), m_auth(auth), m_jwt(jwt), m_port(port) {
    registerRoutes();
}

HttpTransport::~HttpTransport() { stop(); }

void HttpTransport::start() {
    LOG(Info) << "HTTP API listening on port " << m_port;
    m_server.listen("0.0.0.0", m_port);
}

void HttpTransport::startAsync() {
    m_thread = std::thread([this] { start(); });
}

void HttpTransport::stop() {
    m_server.stop();
    if (m_thread.joinable())
        m_thread.join();
}

std::optional<int> HttpTransport::extractUserId(const httplib::Request& req) const {
    const auto it = req.headers.find("Authorization");
    if (it == req.headers.end())
        return std::nullopt;

    const auto& value = it->second;
    constexpr std::string_view prefix = "Bearer ";
    if (value.size() <= prefix.size() || value.substr(0, prefix.size()) != prefix)
        return std::nullopt;

    return m_jwt.verify(value.substr(prefix.size()));
}

void HttpTransport::registerRoutes() {
    m_server.set_default_headers(
        {{"Access-Control-Allow-Origin",  "*"},
         {"Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS"},
         {"Access-Control-Allow-Headers", "Content-Type, Authorization"}});

    m_server.Options(".*", [](const httplib::Request&,
                               httplib::Response& res) { res.status = 204; });

    // -----------------------------------------------------------------------
    // Auth endpoints — no token required
    // -----------------------------------------------------------------------

    // POST /auth/register  body: { username, password }
    m_server.Post("/auth/register",
                  [this](const httplib::Request& req, httplib::Response& res) {
                      try {
                          ok(res, toJson(m_auth.registerUser(
                                      parseRegisterRequest(req.body))));
                      } catch (const std::invalid_argument& e) {
                          badRequest(res, e.what());
                      } catch (const std::exception& e) {
                          serverError(res, e.what());
                      }
                  });

    // POST /auth/login  body: { username, password }
    m_server.Post("/auth/login",
                  [this](const httplib::Request& req, httplib::Response& res) {
                      try {
                          ok(res, toJson(
                                     m_auth.loginUser(parseLoginRequest(req.body))));
                      } catch (const std::invalid_argument& e) {
                          unauthorized(res, e.what());
                      } catch (const std::exception& e) {
                          serverError(res, e.what());
                      }
                  });

    // -----------------------------------------------------------------------
    // Trading endpoints — JWT required on every request
    // -----------------------------------------------------------------------

    // GET /account
    m_server.Get("/account",
                 [this](const httplib::Request& req, httplib::Response& res) {
                     const auto uid = extractUserId(req);
                     if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
                     try {
                         ok(res, toJson(m_handler.getAccount(*uid)));
                     } catch (const std::exception& e) {
                         serverError(res, e.what());
                     }
                 });

    // POST /orders  body: { symbol, side, quantity, price, timestamp? }
    m_server.Post("/orders",
                  [this](const httplib::Request& req, httplib::Response& res) {
                      const auto uid = extractUserId(req);
                      if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
                      try {
                          ok(res, toJson(m_handler.placeOrder(
                                     *uid, parsePlaceOrderRequest(req.body))));
                      } catch (const std::exception& e) {
                          badRequest(res, e.what());
                      }
                  });

    // POST /rules  body: { symbol, triggerPrice, side, quantity }
    m_server.Post("/rules",
                  [this](const httplib::Request& req, httplib::Response& res) {
                      const auto uid = extractUserId(req);
                      if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
                      try {
                          m_handler.addRule(*uid, parseAddRuleRequest(req.body));
                          noContent(res);
                      } catch (const std::exception& e) {
                          badRequest(res, e.what());
                      }
                  });

    // DELETE /rules/{symbol}
    m_server.Delete(R"(/rules/([^/]+))",
                    [this](const httplib::Request& req, httplib::Response& res) {
                        const auto uid = extractUserId(req);
                        if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
                        try {
                            m_handler.removeRules(*uid, req.matches[1].str());
                            noContent(res);
                        } catch (const std::exception& e) {
                            serverError(res, e.what());
                        }
                    });

    // GET /history?symbol=AAPL&interval=1day&start=2024-01-01
    m_server.Get("/history",
                 [this](const httplib::Request& req, httplib::Response& res) {
                     const auto uid = extractUserId(req);
                     if (!uid) { unauthorized(res, "Missing or invalid token"); return; }
                     try {
                         const auto symbol   = req.get_param_value("symbol");
                         const auto interval = req.get_param_value("interval");
                         const auto start    = req.get_param_value("start");

                         if (symbol.empty() || interval.empty() || start.empty()) {
                             badRequest(res, "Required query params: symbol, interval, start");
                             return;
                         }

                         const auto result = m_handler.getHistory(*uid, {symbol, interval, start});
                         if (!result)
                             gatewayError(res, "Data provider returned no data for " + symbol);
                         else
                             ok(res, toJson(*result));
                     } catch (const std::exception& e) {
                         serverError(res, e.what());
                     }
                 });
}

} // namespace trading
