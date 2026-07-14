Three-layer architecture: MarketData, Execution, Tracking

The core problem the simulator solves involves three fundamentally different concerns:

- Knowing what the market is doing (price data)
- Deciding what to do about it (rules and engines)
- Carrying out those decisions (order execution and account management)

These were separated into three distinct layers rather than written as a single class or module.

---

Why separate layers at all

A monolithic design — one class that fetches prices, evaluates rules, and executes orders — is the natural first draft. It works for a prototype but creates two problems as the system grows:

First, the concerns are on different change frequencies. The way prices are fetched changes when a new provider is added (Alpaca, Polygon, etc.) or when the caching strategy changes. The way orders are executed changes when real broker integration is added. The way rules are evaluated changes when new strategy types are introduced. In a monolith, a change to any one of these touches the same code that handles the other two, making changes riskier and tests harder to write.

Second, testing becomes painful. To test a limit rule firing correctly, you do not want to make real HTTP calls to a price provider or actually debit a cash balance. Separated layers mean each can be tested with a fake implementation of the adjacent interface.

---

Why these three boundaries specifically

MarketData is separated because it is the only layer that talks to the outside world. It owns the provider abstraction (IDataProvider), the caching strategy (ILocalCache), and all the logic for merging and deduplicating data from multiple sources. Nothing else in the system needs to know whether a price came from Twelve Data, Alpaca, or a local CSV file.

Execution is separated because it owns financial state. Cash, positions, and trade history are sensitive data that must be updated atomically and persisted reliably. Separating this layer means the persistence strategy (SQLite, in-memory, future real broker) can be swapped behind the IBrokerAccount interface without any other layer knowing.

Tracking is separated because it owns the decision logic. UserLimitTracker and IDecisionEngine are the places where "what should happen and when" is defined. This layer consumes prices from MarketData and produces OrderTickets for Execution, but it knows nothing about how prices are fetched or how orders are carried out.

---

TradingController as the wiring layer

The three layers do not communicate directly with each other. TradingController owns all three and wires them together via lambdas in its constructor. This means each layer has zero compile-time or link-time dependency on the others — MarketData does not include any Execution headers, Tracking does not include any MarketData headers, and so on.

The practical benefit is that each layer can be built, tested, and reasoned about in complete isolation. TradingController is the only place in the codebase that knows all three layers exist simultaneously.

---

Tradeoff accepted

More files, more interfaces, more indirection than a monolith. For a project of this size, that is a real cost. The bet is that the system will grow — new providers, real broker integration, new strategy types, a frontend — and that the separation will pay off as each of those changes can be made in one layer without touching the others.
