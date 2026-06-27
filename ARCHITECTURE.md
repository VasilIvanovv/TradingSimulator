# Trading Simulator Architecture

## Goal

Build a high-performance backtesting engine in modern C++ with a clean separation between:

- domain model
- data ingestion
- simulation engine
- trading strategies
- performance analytics
- application entry point

The guiding principle is to keep the hot path allocation-free, cache-friendly, and easy to test.

---

## Recommended Layering

### 1) Core domain layer
This is the foundation of the project.

Contains:
- OHLCV bar model
- order and fill types
- portfolio state
- position representation
- time/price/quantity aliases
- result and metric structures

Rules:
- no file I/O
- no CLI code
- no strategy logic
- no dependency on CSV parsing

This layer should be plain data and small utilities only.

---

### 2) Data ingestion layer - Data broker, etc...
Responsible for loading historical market data.

Contains:
- CSV reader
- timestamp parsing
- column validation
- optional symbol/file metadata

Responsibilities:
- read raw text efficiently
- transform it into `OhlcvBar` objects
- avoid unnecessary allocations

Rules:
- parser does not know about strategies
- parser does not execute trades
- parser only converts data

---

### 3) Simulation engine
The engine drives the backtest loop.

Typical flow:
1. receive a sequence of bars
2. feed each bar to the active strategy
3. create orders or signals
4. simulate fills
5. update portfolio state
6. record metrics

This is where performance matters most.

---

### 4) Strategy layer
Each strategy implements a common interface.

Examples:
- moving average crossover
- RSI mean reversion
- breakout strategy

Rules:
- strategies should only depend on the core domain model
- strategies should not read files
- strategies should not print output
- strategies should be easy to swap and test

---

### 5) Analytics layer
Calculates final and intermediate performance statistics.

Examples:
- total return
- max drawdown
- Sharpe ratio
- win rate
- profit factor

This layer should accept finished trade/portfolio data and produce reports.

---

### 6) Application layer
The executable is only the glue.

Responsibilities:
- parse command-line arguments
- load CSV
- construct strategy
- run backtest
- print or export results

Keep this layer thin.

---

## Suggested Folder Structure

```text
TradingSimulator/
  CMakeLists.txt
  README.md
  ARCHITECTURE.md
  include/
    trading/
      core/
        types.hpp
        ohlcv.hpp
        order.hpp
        portfolio.hpp
      data/
        csv_reader.hpp
      engine/
        backtest_engine.hpp
      strategies/
        strategy.hpp
      analytics/
        metrics.hpp
  src/
    main.cpp
    data/
      csv_reader.cpp
    engine/
      backtest_engine.cpp
```

If you want to start smaller, begin with:

```text
include/trading/ohlcv.hpp
include/trading/data/csv_reader.hpp
src/main.cpp
```

and expand later.

---

## OHLCV Design Recommendation

Use a simple plain-old-data style structure.

Example fields:
- timestamp
- open
- high
- low
- close
- volume

Design goals:
- trivially copyable if possible
- no heap allocation
- no virtual methods
- no behavior inside the model

This makes it cheap to store in `std::vector` and efficient in tight loops.

---

## CSV Parsing Strategy

For performance, prefer:

- large buffered file reads
- `std::string_view` slicing
- `std::from_chars` for numeric conversion where practical
- no per-field string allocation
- minimal intermediate objects

Recommended parser pipeline:
1. read one line or one block
2. split fields using views
3. parse timestamp and numbers
4. construct `OhlcvBar`
5. append to output container

---

## Recommended First Milestone

Build a vertical slice first:

1. create CMake project
2. define `OhlcvBar`
3. create a CSV reader interface
4. parse a sample CSV file
5. print the number of bars loaded

That gives you a working base before moving on to strategies and portfolio logic.

---

## What to Build Next

After this foundation, the next priorities should be:

1. `OhlcvBar` header
2. CSV reader interface
3. parser implementation
4. small CLI demo
5. strategy interface
6. portfolio/order simulator
7. analytics report

---

## Performance Principles

Keep these in mind from the start:

- avoid dynamic allocation in the inner loop
- prefer contiguous storage
- separate parsing from simulation
- keep data structures small
- keep interfaces stable and explicit
- measure before optimizing

---

## Suggested Development Order

1. architecture and folder structure
2. core data types
3. CSV ingestion
4. minimal backtest loop
5. strategy interface
6. order execution and portfolio tracking
7. analytics
8. tests and benchmarking
