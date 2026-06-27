# Start Here

This project should begin with a small, stable foundation before any trading logic is added.

## Phase 1: Foundations

### 1. Keep the core domain pure
Create the first data models in `include/trading/core/`:

- `OhlcvBar`
- `Timestamp` alias
- `Price` alias
- `Quantity` alias

Rules:
- no I/O
- no strategy code
- no simulation logic
- no heap allocations inside the model

The first goal is to make the market data shape explicit and stable.

---

### 2. Define the CSV format contract
Before writing a parser, decide the expected CSV format.

Recommended first format:

```csv
timestamp,open,high,low,close,volume
2024-01-01T00:00:00Z,42000.0,42500.0,41800.0,42300.0,1234.56
```

Decide:
- whether a header is required
- timestamp format
- delimiter
- whether rows are sorted
- whether missing values are allowed

This prevents parser redesign later.

---

### 3. Build a minimal CSV reader
Create a reader that:
- loads a file
- parses rows into `OhlcvBar`
- returns `std::vector<OhlcvBar>`

For the first version, prioritize clarity and correctness over maximum speed.
Then improve it with:
- buffered I/O
- `std::string_view`
- `std::from_chars`
- reduced allocations

---

### 4. Create a small executable demo
Your first executable should only:
- accept a CSV path
- load bars
- print how many bars were parsed
- print the first and last bar

That gives you a complete vertical slice.

---

## Phase 2: Simulation Core

After ingestion works, add:

- `Order`
- `Fill`
- `Position`
- `Portfolio`
- `Strategy` interface
- `BacktestEngine`

At that point, the application can process historical bars and simulate trades.

---

## Phase 3: Analytics

Once trades can be simulated, add:
- total return
- drawdown
- Sharpe ratio
- win rate
- trade statistics

---

## Suggested File Layout

```text
TradingSimulator/
  CMakeLists.txt
  ARCHITECTURE.md
  START_HERE.md
  include/
    trading/
      core/
        ohlcv.hpp
        types.hpp
      data/
        csv_reader.hpp
  src/
    main.cpp
    data/
      csv_reader.cpp
```

---

## Recommended First Coding Order

1. create `include/trading/core/types.hpp`
2. create `include/trading/core/ohlcv.hpp`
3. create `include/trading/data/csv_reader.hpp`
4. create `src/data/csv_reader.cpp`
5. create `src/main.cpp`
6. wire everything into CMake
7. test with a small sample CSV

---

## Design Principles

- use contiguous storage
- keep data models trivial
- isolate parsing from simulation
- avoid polymorphism in the hot path unless needed
- measure performance before optimizing

---

## Best Starting Point

Start with `OhlcvBar` and the CSV contract.

Everything else depends on those two decisions.
