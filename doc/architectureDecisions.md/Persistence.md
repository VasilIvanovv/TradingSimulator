Persistence strategy: SQLite for account state, CSV for market data cache

The project has two distinct persistence needs that were evaluated separately, because they have different characteristics and different failure modes.

---

Account state — why a database, and why SQLite

Account state consists of three tightly coupled pieces: cash balance, open positions, and trade history. These three must always be consistent with each other. If a buy order fills, the cash decreases, the position increases, and a trade record is appended — all three changes belong together. If the process crashes between any two of those writes, the account is in a corrupt state: cash debited but position never credited, or position updated but no trade record written.

This is exactly the problem databases solve. SQLite wraps all three writes in a single transaction:

BEGIN;
  UPDATE account_state SET cash = ?;
  INSERT OR REPLACE INTO positions (symbol, quantity) VALUES (?, ?);
  INSERT INTO trade_history (...) VALUES (...);
COMMIT;

Either all three succeed, or none of them do. There is no in-between state.

Why SQLite specifically over PostgreSQL or another database:

- No separate server process. SQLite is a library that writes to a single file. For a desktop/cloud application with one active user at a time, spinning up a full database server adds deployment complexity with no benefit.
- Zero configuration. No connection strings, no ports, no authentication setup. The file path is the only configuration needed.
- Sufficient for the scale. Account state is a handful of rows — one cash row, one row per open position, one row per trade. SQLite handles billions of rows; this dataset never exceeds thousands.
- Atomic transactions. Despite being serverless, SQLite provides full ACID guarantees. Crash safety is handled correctly.

The tradeoff accepted: SQLite does not support concurrent writes from multiple processes. If this application ever needs multiple processes writing account state simultaneously, SQLite becomes a bottleneck. For the current single-process architecture this is not a concern.

---

Market data cache — why CSV, not a database

Market data (price candles) is fundamentally different from account state:

- Read-mostly. Candles are fetched once from the provider, written to disk, and then only read from that point forward. There are no updates to existing rows.
- Idempotent writes. If a cache write fails mid-way, nothing is corrupted — the next call simply re-fetches from the provider. There is no multi-field atomicity requirement.
- No relations. Each symbol/interval combination is completely independent of every other. There is no relational structure to exploit.
- Human-readable. A CSV file can be opened in Excel or a text editor, inspected, and manually corrected if needed. This is genuinely useful during development and debugging.
- Already working. CsvLocalCache has a full test suite proving it handles append, deduplication, and startDate filtering correctly.

Adding SQLite here would provide:
- A single file instead of a directory of CSVs (minor organisational benefit)
- SQL WHERE for date filtering instead of a linear scan (irrelevant at candle-dataset scale)

At the cost of:
- Removing human readability
- Adding SQLite as a dependency to the MarketData module
- Rewriting and re-testing CsvLocalCache for no correctness gain

The rule applied: use a database when you need atomicity, relations, or crash-safe multi-field writes. Use flat files when data is independent, idempotent, and human readability matters. Market data fits the second case; account state fits the first.

---

If requirements change

If the MarketData cache grows to millions of candles across hundreds of symbols and query performance becomes a concern, a SQLiteLocalCache implementing the existing ILocalCache interface can be added without touching DataBroker or any other caller. The interface boundary already exists for exactly this reason.
