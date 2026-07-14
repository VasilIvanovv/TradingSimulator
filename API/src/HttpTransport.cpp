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

void ok(httplib::Response& res, const std::string& body) { respond(res, 200, body); }
void noContent(httplib::Response& res)                   { res.status = 204; }
void badRequest(httplib::Response& res, const std::string& msg) { respond(res, 400, toJsonError(msg)); }
void serverError(httplib::Response& res, const std::string& msg) { respond(res, 500, toJsonError(msg)); }
void gatewayError(httplib::Response& res, const std::string& msg) { respond(res, 502, toJsonError(msg)); }

} // namespace

HttpTransport::HttpTransport(ApiHandler& handler, int port)
    : m_handler(handler), m_port(port) {
    registerRoutes();
}

HttpTransport::~HttpTransport() {
    stop();
}

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

void HttpTransport::registerRoutes() {
    // Add CORS headers to every response so browser-based frontends can call
    // this API from a different origin (e.g. React dev server on port 3000).
    m_server.set_default_headers({
        { "Access-Control-Allow-Origin",  "*" },
        { "Access-Control-Allow-Methods", "GET, POST, DELETE, OPTIONS" },
        { "Access-Control-Allow-Headers", "Content-Type" }
    });

    // Handle preflight OPTIONS requests sent by browsers before POST/DELETE.
    m_server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        res.status = 204;
    });

    // GET /account
    m_server.Get("/account", [this](const httplib::Request&, httplib::Response& res) {
        try {
            ok(res, toJson(m_handler.getAccount()));
        } catch (const std::exception& e) {
            serverError(res, e.what());
        }
    });

    // POST /orders  body: { symbol, side, quantity, price, timestamp? }
    m_server.Post("/orders", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            ok(res, toJson(m_handler.placeOrder(parsePlaceOrderRequest(req.body))));
        } catch (const std::exception& e) {
            badRequest(res, e.what());
        }
    });

    // POST /rules  body: { symbol, triggerPrice, side, quantity }
    m_server.Post("/rules", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            m_handler.addRule(parseAddRuleRequest(req.body));
            noContent(res);
        } catch (const std::exception& e) {
            badRequest(res, e.what());
        }
    });

    // DELETE /rules/{symbol}
    m_server.Delete(R"(/rules/([^/]+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            m_handler.removeRules(req.matches[1].str());
            noContent(res);
        } catch (const std::exception& e) {
            serverError(res, e.what());
        }
    });

    // GET /history?symbol=AAPL&interval=1day&start=2024-01-01
    m_server.Get("/history", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            const auto symbol   = req.get_param_value("symbol");
            const auto interval = req.get_param_value("interval");
            const auto start    = req.get_param_value("start");

            if (symbol.empty() || interval.empty() || start.empty()) {
                badRequest(res, "Required query params: symbol, interval, start");
                return;
            }

            const auto result = m_handler.getHistory({ symbol, interval, start });
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
