HTTP lib over websocket

Stateful connections — the server must maintain an open connection per client. With HTTP, each request is independent — the server handles it and forgets. With WebSocket, the server must track every connected client, handle disconnects, reconnects, and timeouts. More code, more things that can go wrong.

Harder to test — you can't test with curl. You need a WebSocket client. HTTP endpoints you can hit with a browser, Postman, or a one-liner.

Infrastructure complexity — many reverse proxies, load balancers, and cloud services have special configuration requirements for WebSocket (connection timeouts, sticky sessions). HTTP just works everywhere.

Harder to debug — HTTP requests/responses are visible in browser dev tools, logs, and proxies. WebSocket frames are a stream — harder to inspect.

Reconnection is your problem — if the connection drops, the client must detect it and reconnect. With HTTP polling, a dropped connection is invisible — the next poll just works.

For your specific project, the honest answer is: WebSocket isn't faster in any way that matters here. The watcher fires every 60 seconds. Whether the client learns about a fill 0ms or 5 seconds after it happens is irrelevant for paper trading. The duplex advantage is also unused — the client sends commands (place order, add rule), the server responds. That's just HTTP.

WebSocket wins when you have many clients receiving high-frequency events — a live order book updating 100 times per second for 10,000 users. That's a real-time trading platform. You're building a simulator.

The rule: use the simplest thing that meets the requirement. Right now that's HTTP. If you later add live price streaming (tick-by-tick), reconsider.