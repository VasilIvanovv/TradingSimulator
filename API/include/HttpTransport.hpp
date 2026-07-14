#pragma once

#include "ApiHandler.hpp"
#include <httplib.h>
#include <thread>

namespace trading {

/**
 * @brief HTTP/1.1 transport adapter built on cpp-httplib.
 *
 * Registers REST routes and translates each HTTP request into an @c ApiHandler
 * call, then serializes the result back to JSON. Knows nothing about the trading
 * domain directly — all business logic lives in @c ApiHandler.
 *
 * ### Endpoints
 * | Method   | Path                | Body / query params                              |
 * |----------|---------------------|--------------------------------------------------|
 * | GET      | /account            | —                                                |
 * | POST     | /orders             | `{ symbol, side, quantity, price, timestamp? }`  |
 * | POST     | /rules              | `{ symbol, triggerPrice, side, quantity }`       |
 * | DELETE   | /rules/{symbol}     | —                                                |
 * | GET      | /history            | `?symbol=&interval=&start=`                      |
 *
 * ### Adding a new transport
 * Create a class that holds an @c ApiHandler& and calls its methods. No changes
 * to @c ApiHandler, @c ApiJson, or any domain code are required.
 */
class HttpTransport {
public:
    /**
     * @param handler Shared handler that processes all requests.
     *                Must outlive this transport.
     * @param port    TCP port to listen on.
     */
    HttpTransport(ApiHandler& handler, int port);
    ~HttpTransport();

    /**
     * @brief Start the server and block until stop() is called.
     * Suitable for use as the main thread's entry point.
     */
    void start();

    /**
     * @brief Start the server in a background thread (non-blocking).
     * Call stop() to shut it down gracefully.
     */
    void startAsync();

    /**
     * @brief Stop the server and join the background thread (if any).
     * Safe to call even if the server was never started.
     */
    void stop();

private:
    void registerRoutes();

    ApiHandler&     m_handler;
    httplib::Server m_server;
    int             m_port;
    std::thread     m_thread;
};

} // namespace trading
