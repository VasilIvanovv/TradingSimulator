Transport-agnostic API design: ApiHandler separated from HttpTransport

When adding an HTTP API layer, the simplest approach is to put the business logic directly inside the route handler callbacks:

    server.Get("/account", [&](const httplib::Request&, httplib::Response& res) {
        double cash = controller.getAvailableCash();
        auto positions = controller.getAllPositions();
        res.set_content(buildJson(cash, positions), "application/json");
    });

This works but ties the domain logic to cpp-httplib. If the transport ever changes — WebSocket, gRPC, a test harness, a command-line interface — every handler has to be rewritten because the logic and the transport are the same code.

---

The decision

ApiHandler is a plain C++ class with typed methods (getAccount, placeOrder, addRule, etc.) that call TradingController directly. It has no knowledge of HTTP, request objects, response objects, or JSON.

HttpTransport is a separate class that owns the httplib::Server, registers routes, and does nothing but translate: parse the HTTP request, call the appropriate ApiHandler method, serialize the result to JSON, write it to the HTTP response.

The JSON serialization (ApiJson) is also separate from both, so the same parse/serialize functions can be reused by any future transport that also uses JSON.

---

Why this matters for this project specifically

The project targets multiple clients: web, mobile, and Windows. It is likely that at some point a different transport will be needed — WebSocket for push notifications if the poll interval becomes very short, or a different protocol for mobile. When that happens, a new transport class is written that holds an ApiHandler& and calls the same methods. ApiHandler, ApiJson, and all domain code remain unchanged.

Adding a WebSocket transport later looks like this:

    // New file: WsTransport.cpp
    // Everything else stays the same.
    conn.onMessage([&](const std::string& msg) {
        auto env = parseEnvelope(msg);
        if (env.type == "place_order")
            conn.send(toJson(handler.placeOrder(fromJson<PlaceOrderRequest>(env.payload))));
    });

Running both transports simultaneously (HTTP for REST clients, WebSocket for live updates) is also possible: instantiate both, point both at the same ApiHandler instance.

---

The rule this follows

The same principle already applied throughout the rest of the project: TradingController does not know whether orders come from a UI, a rule, or an engine. DataBroker does not know whether prices are cached in CSV or SQLite. The transport layer should not know what it is transporting, and the business logic should not know how it is being called.

---

Tradeoff accepted

One extra class and one extra file compared to embedding logic in route handlers. The indirection is small and the boundary is clear: anything that touches httplib belongs in HttpTransport, anything that touches TradingController belongs in ApiHandler.
