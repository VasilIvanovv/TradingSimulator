#pragma once

#include "ApiHandler.hpp"
#include "AuthHandler.hpp"
#include "JwtService.hpp"
#include "LogoManager.hpp"
#include "SymbolManager.hpp"
#include <httplib.h>
#include <optional>
#include <string>
#include <thread>

namespace trading {

/**
 * @brief HTTP/1.1 transport adapter built on cpp-httplib.
 *
 * Registers REST routes and translates each HTTP request into an @c ApiHandler
 * or @c AuthHandler call, then serializes the result back to JSON. Knows
 * nothing about the trading domain directly — all business logic lives in those
 * handlers.
 *
 * ### Public endpoints (no token required)
 * | Method | Path               | Body                          |
 * |--------|--------------------|-------------------------------|
 * | POST   | /auth/register     | `{ username, password }`      |
 * | POST   | /auth/login        | `{ username, password }`      |
 *
 * ### Protected endpoints (require: Authorization: Bearer <token>)
 * | Method | Path               | Body / query params                             |
 * |--------|--------------------|-------------------------------------------------|
 * | GET    | /account           | —                                               |
 * | POST   | /orders            | `{ symbol, side, quantity, price, timestamp? }` |
 * | POST   | /rules             | `{ symbol, triggerPrice, side, quantity }`      |
 * | DELETE | /rules/{symbol}    | —                                               |
 * | GET    | /history           | `?symbol=&interval=&start=`                     |
 *
 * ### Public endpoints (no token required)
 * | Method | Path               | Notes                                           |
 * |--------|--------------------|-------------------------------------------------|
 * | GET    | /symbols           | Full tradeable symbol catalogue                 |
 * | GET    | /logos/{symbol}    | PNG logo for the symbol (CDN-cached on disk)    |
 */
class HttpTransport {
public:
    HttpTransport(ApiHandler& handler, AuthHandler& auth, JwtService& jwt,
                  SymbolManager& symbols, LogoManager& logos,
                  int port, std::string bindHost = "0.0.0.0");
    ~HttpTransport();

    /** @brief Start the server and block until stop() is called. */
    void start();

    /** @brief Start the server in a background thread (non-blocking). */
    void startAsync();

    /** @brief Stop the server and join the background thread (if any). */
    void stop();

private:
    void registerRoutes();

    /// Extract and verify the JWT from the Authorization header.
    /// Returns the user id on success, nullopt if missing or invalid.
    std::optional<int> extractUserId(const httplib::Request& req) const;

    ApiHandler&     m_apiHandler;
    AuthHandler&    m_authHandler;
    JwtService&     m_jwtService;
    SymbolManager&  m_symbolManager;
    LogoManager&    m_logoManager;
    httplib::Server m_httpServer;
    int             m_listenPort;
    std::string     m_bindHost;
    std::thread     m_serverThread;
};

} // namespace trading
