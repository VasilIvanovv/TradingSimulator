IBrokerAccount abstraction: PaperAccount behind an interface

The simulator currently executes all orders against a paper (simulated) account. PaperAccount tracks cash, positions, and trade history in memory and persists them to SQLite. It could have been used directly everywhere an account is needed.

Instead, PaperAccount implements an interface — IBrokerAccount — and TradingController owns a std::unique_ptr<IBrokerAccount>, not a std::unique_ptr<PaperAccount>.

---

Why

The simulator is paper trading today. The long-term target is a cloud backend serving real users who will eventually want to connect to a real broker (Interactive Brokers, Alpaca, Tradier, etc.) and execute orders with real money.

When that happens, a RealBrokerAccount class implementing IBrokerAccount replaces PaperAccount at the construction site in main.cpp (or wherever the controller is assembled). TradingController, MarketWatcher, ApiHandler, and every other part of the system remain unchanged — they only ever call executeOrder, getAvailableCash, getAllPositions, and getTradeHistory through the interface.

Without the interface, adding real broker support would require touching TradingController and every other class that currently holds a PaperAccount directly.

---

What the interface enforces

IBrokerAccount defines the contract that any account implementation must satisfy:

- executeOrder — attempt to fill an order, return a receipt indicating filled or rejected
- getAvailableCash — current spendable balance
- getPosition / getAllPositions — current holdings
- getTradeHistory — immutable record of all filled trades

A real broker implementation of this interface would translate these calls into API requests to the broker's REST or WebSocket API, handle authentication, and map the broker's response format back to the same ExecutionReceipt type the rest of the system already uses.

---

The same pattern applied elsewhere

IBrokerAccount is one instance of the same design principle used throughout the project:

- IDataProvider allows TwelveDataProvider and AlpacaProvider to be swapped or combined without DataBroker knowing which one is active.
- ILocalCache allows CsvLocalCache to be replaced with a SQLite-backed cache without DataBroker knowing.
- IAccountStore allows SqliteAccountStore to be replaced with any other persistence backend without PaperAccount knowing.
- IDecisionEngine allows algorithmic strategies to be added without MarketWatcher knowing their implementation.

The pattern: define the contract at the boundary, program to the interface, inject the concrete implementation from the outside. New implementations can be added without modifying existing code.

---

Tradeoff accepted

A virtual dispatch call per order execution instead of a direct call. At paper trading scale (orders per minute at most) this cost is immeasurable. If the system ever needed to execute millions of orders per second in a backtesting loop, the virtual call overhead could be revisited — but that is not the current use case.
