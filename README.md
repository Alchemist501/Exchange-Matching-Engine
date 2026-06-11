# ⚡ Low-Latency Exchange Matching Engine

A high-performance electronic exchange simulation platform written in C++20, implementing price-time priority order matching, pre-trade risk management, portfolio accounting, market statistics, REST APIs, and real-time WebSocket market data feeds.

Built with a focus on low-latency systems design and modern exchange infrastructure.

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support/20)
[![CMake](https://img.shields.io/badge/Build-CMake-green.svg)](https://cmake.org/)
[![GoogleTest](https://img.shields.io/badge/Testing-GoogleTest-blue.svg)](https://github.com/google/googletest)
[![REST API](https://img.shields.io/badge/REST--API-Crow-orange.svg)](https://github.com/Crowcpp/Crow)
[![WebSocket](https://img.shields.io/badge/WebSocket-Feed-yellow.svg)](https://github.com/Crowcpp/Crow)
[![License](https://img.shields.io/badge/License-MIT-purple.svg)](LICENSE)

```
+------------------------------------------------------+
|  ORDER BOOK          LIVE TRADES       MARKET STATS  |
|                                                      |
| Ask 102.4  ███████                                   |
| Ask 102.3  ███                                       |
| Bid 102.2  ███████████                               |
| Bid 102.1  █████                                     |
|                                                      |
| Spread : 0.02                                        |
| VWAP   : 102.31                                      |
+------------------------------------------------------+
```

### 🖥️ Live Demonstration (WebSocket & Depth Feed)
![Exchange Live Demo](assets/Demo_.gif)

---

## Overview

This project simulates the internal infrastructure of an electronic exchange.

Incoming client orders are validated by a pre-trade risk engine before being processed by a price-time priority matching engine.

Executed trades update trader portfolios, market statistics, and are broadcast through REST and WebSocket interfaces.

The system was designed with low latency and efficient memory management as primary goals.

---

## Motivation

Electronic exchanges process millions of orders every second while maintaining deterministic price-time priority. 

This project explores the internal architecture behind modern matching engines, focusing on low-latency execution, efficient memory layouts, and exchange infrastructure design.

---

## Features

- Price-Time Priority Matching
- Market Orders
- Limit Orders
- Partial Fills
- Order Cancellation
- Order Modification
- Pre-Trade Risk Checks
- Portfolio Management
- PnL Calculation
- VWAP Calculation
- Bid-Ask Spread
- Market Depth
- WebSocket Feed
- REST API
- Benchmark Suite
- GoogleTest Unit Tests

---

## Architecture

The system uses a pipeline of decoupled layers to ingest, validate, match, and broadcast trades:

```
                 +----------------+
                 | Client Orders  |
                 +--------+-------+
                          |
                          v
                +------------------+
                | Order Gateway    |
                +------------------+
                          |
                          v
                +------------------+
                | Risk Engine      |
                +------------------+
                          |
                          v
                +------------------+
                | Matching Engine  |
                +------------------+
                   |          |
         Trades    |          | Resting Orders
                   |          |
                   v          v
        +----------------+   Order Book
        | Portfolio      |
        +----------------+
                 |
                 v
        +----------------+
        | Statistics     |
        +----------------+
                 |
        +--------+--------+
        |                 |
        v                 v
 REST API          WebSocket Feed
```

---

## Order Lifecycle

Every order submitted follows this lifecycle:

```
Client ──► Gateway ──► Risk Validation ──► Matching Engine ──► Trade Generated ──► Portfolio Update ──► Statistics Update ──► Broadcast
```

* **Client**: An external participant places a limit or market order.
* **Gateway**: Ingests the order, assigns a unique sequential `orderId`, and timestamps the request.
* **Risk Validation**: Checks the order against limits. If it fails, the order is tagged `REJECTED` and execution terminates.
* **Matching Engine**: Crosses the incoming order against resting volume on the opposite side of the book.
* **Trade Generated**: For matching prices, `Trade` execution records are logged.
* **Portfolio Update**: Credited or debited cash and position inventory for both buyer and seller.
* **Statistics Update**: Recalculates VWAP, spread, volume, and mid-price.
* **Broadcast**: Pushes real-time JSON updates to connected WebSocket clients and updates HTTP endpoints.

---

## Matching Logic

The matching core implements standard crossing logic:

```
Buy Limit
   ↓
Best Ask
   ↓
Cross? ──► No ──► Remaining Quantity ──► Resting Liquidity
   ↓
  Yes
   ↓
 Trade
```

* **Price Priority**: The best price receives execution priority (highest bids and lowest asks are matched first).
* **Time Priority**: When multiple orders sit at the same price level, they are executed in FIFO (First In, First Out) order based on insertion timestamp.
* **Partial Fills**: Incoming orders can match against multiple resting orders. Unfilled remainders of limit orders are written to the book.
* **Resting Liquidity**: The unmatched volume left in the book becomes passive liquidity available to subsequent crossing orders.

### Algorithmic Complexity

| Operation | Complexity | Rationale |
| :--- | :--- | :--- |
| **Insert** | $O(\log N)$ | Insertion of a new price level inside the binary search tree. |
| **Match** | $O(\log N)$ | Accessing the best price level ($O(\log N)$) followed by $O(1)$ FIFO queue operations. |
| **Cancel** | $O(1)$ | Direct node removal from the queue using cached list iterators. |
| **Modify** | $O(1)$ / $O(\log N)$ | $O(1)$ for quantity reductions (preserving priority). $O(\log N)$ if price changes (losing priority). |
| **Lookup** | $O(1)$ | Direct lookup via hash map index. |

---

## Internal Data Structures

To avoid slow $O(N)$ searches, memory is structured using specific STL containers to maintain constant-time complexity:

| Structure | Purpose | Why |
| :--- | :--- | :--- |
| `std::unordered_map` | O(1) Order Lookup | Maps Order IDs to their exact location in the book, providing constant-time access to any order. |
| `std::list` | FIFO Queue | Implements the time-priority queue at each price level. Allows $O(1)$ insertions at the back and $O(1)$ deletions. |
| `std::map` | Price Levels | Red-Black Tree that automatically keeps price levels sorted ($O(\log N)$ inserts). Asks are sorted ascending, and Bids are sorted descending. |
| `std::list::iterator` | O(1) Cancel | Cached inside the lookup map. By referencing the iterator directly, we can erase a cancelled or modified order node from the list in $O(1)$ time without searching the list. |
| `std::vector` | Statistics | Stores trade execution logs contiguously in memory, minimizing CPU cache misses during sequential loops. |

---

## Design Decisions

### Why `std::list` instead of `std::vector` for price queues?
A `std::vector` stores elements contiguously. If an order is cancelled or modified from the middle of the queue, all subsequent elements must be shifted over, resulting in $O(N)$ overhead. A doubly-linked `std::list` allows node deletion in $O(1)$ time, which is essential for low-latency cancellations.

### Why `std::map` instead of `std::unordered_map` for price levels?
An exchange requires price levels to be strictly sorted (highest Bid at the top, lowest Ask at the top) to quickly find matches. `std::unordered_map` is unsorted, while `std::map` keeps keys sorted inside a Red-Black Tree, enabling $O(1)$ access to the best price levels and $O(\log N)$ insertion.

### Why cache iterators?
Searching a `std::list` is an $O(N)$ scan. By caching the `std::list::iterator` inside the `std::unordered_map` lookup table during order insertion, the engine can directly jump to the list node and erase it in $O(1)$ time without traversing the queue.

### Why single-threaded matching?
Executing order books on multiple threads introduces massive lock contention and cache-bouncing as threads fight for the same memory addresses. Pinned single-threaded matching ensures deterministic execution order and preserves L1/L2 cache locality, which is standard practice in commercial matching engines.

### Why REST + WebSocket?
REST APIs (HTTP POST/DELETE) provide a simple, reliable protocol for order routing gates. WebSockets provide a low-overhead, persistent TCP connection to stream high-frequency market depth and trades without the round-trip latency of HTTP polling.

---

## Risk Engine

Every order is validated against pre-trade risk controls before hitting the book:

```
Incoming Order ──► Max Size Check ──► Position Check ──► Cash Check ──► Accept / Reject
```

* **Max Size Check**: Rejects orders containing quantities that exceed the maximum limits (e.g. fat-finger protection).
* **Position Check**: Tracks net long/short positions per trader. Rejects orders that would cause the trader's position to exceed maximum thresholds.
* **Cash Check**: Verifies that the trader has enough cash balance to cover a BUY limit order's maximum cost (`price * quantity`).

---

## Portfolio Engine

Updates cash, inventory, and profit/loss metrics after every trade execution:

```
Trade ──► Cash Balance ──► Position ──► Average Price ──► Realized PnL ──► Unrealized PnL
```

* **Cash Balance**: Decreased for the buyer, increased for the seller.
* **Position**: Tracks asset units owned (long position) or owed (short position).
* **Average Price**: Computes the volume-weighted average entry price of open positions.
* **Realized PnL**: Recorded permanently when closing or trimming positions (e.g. covering a short position).
* **Unrealized PnL**: Calculated dynamically based on the current market mid price relative to the position's average entry price.

---

## Statistics Engine

Aggregates trading metrics in real-time:
* **VWAP (Volume-Weighted Average Price)**: $\frac{\sum (\text{Price} \times \text{Quantity})}{\sum \text{Quantity}}$
* **Mid Price**: Average of the best bid and ask.
* **Spread**: Difference between the best ask and the best bid.
* **Volume**: Total quantity of units traded.
* **Trade Count**: Cumulative execution events.
* **Best Bid**: The highest active buy limit price.
* **Best Ask**: The lowest active sell limit price.

---

## REST API

### Endpoints

#### 1. Place Order (`POST /orders`)
* **Request Body:**
  ```json
  {
    "traderId": "TraderA",
    "side": "BUY",
    "quantity": 100,
    "price": 101.50
  }
  ```
  *(Omit price or set to `-1.0` for a Market Order)*
* **Response (200 OK):**
  ```json
  {
    "orderId": 1000,
    "traderId": "TraderA",
    "side": "BUY",
    "price": 101.5,
    "quantity": 100,
    "remainingQuantity": 0,
    "status": "FILLED",
    "trades": [
      { "tradeId": 1, "price": 101.5, "quantity": 100 }
    ]
  }
  ```

#### 2. Cancel Order (`DELETE /orders/<id>`)
* **Response (200 OK):**
  ```json
  {
    "orderId": 1000,
    "status": "CANCELLED"
  }
  ```

#### 3. Modify Order (`PUT /orders/<id>`)
* **Request Body:**
  ```json
  {
    "price": 102.00,
    "quantity": 150
  }
  ```
* **Response (200 OK):** Returns new order parameters and a `priorityLost` boolean flag.

#### 4. Query Market Data
* **`GET /market/stats`**: Returns VWAP, spread, mid price, best bid/ask, and volume.
* **`GET /market/depth`**: Returns bid/ask depth tables up to 20 levels.

---

## WebSocket

Clients stream real-time trade execution feeds at `ws://localhost:8080/market`:

```json
{
  "event": "trade",
  "tradeId": 1,
  "buyOrderId": 1000,
  "sellOrderId": 999,
  "buyerId": "TraderA",
  "sellerId": "TraderB",
  "price": 101.25,
  "quantity": 5,
  "timestamp": 178234827382
}
```

---

## Benchmarks

A benchmark run matching **1,000,000 orders** pre-allocated in memory yields:

```
Orders:      1,000,000
Throughput:  1,798,561 orders/sec
Average:     0.531 μs
P50:         0.300 μs
P99:         1.100 μs
```

> [!NOTE]
> Benchmark performed using 1,000,000 pre-generated in-memory orders executed in Release mode. Measurements include matching, risk validation, and bookkeeping overhead.

### Environment
* **CPU**: *[Specify your CPU model here, e.g. Intel Core i7-12700H or AMD Ryzen]* (Benchmark was executed on a high-frequency Windows machine)
* **RAM**: *[Specify your local RAM here, e.g. 16 GB DDR4]*
* **Compiler**: Microsoft Visual Studio 2026 (MSVC 19.51)
* **Build Flags**: `/O3` (Release Optimization), C++20 Standard
* **OS**: Windows 11 Home (x64)

---

## Unit Tests

Validated using GoogleTest:
* `[✔] Price Priority` (crossing best resting prices first)
* `[✔] Time Priority` (FIFO matching at same price level)
* `[✔] Partial Fill` (consuming depth and writing remainders)
* `[✔] Cancellation` ($O(1)$ removal from lists and lookup maps)
* `[✔] Modification` (preserving priority on quantity reductions, losing it on price/qty increases)

---

## Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/Low-Latency-Matching-Engine.git
cd Low-Latency-Matching-Engine

# Create and configure build folder
cmake -G "Visual Studio 18 2026" -A x64 -B build -S .

# Build release targets
cmake --build build --config Release
```

---

## Run Server

Run the gateway API server with an active background trade generator:
```bash
.\build\Release\order_book_server.exe --port 8080 --simulate
```

---

## Run Benchmark

Run the microsecond-scale performance profiler:
```bash
.\build\benchmark\Release\order_book_benchmark.exe 1000000
```

---

## Future Work

* **Lock-free queues**: Replace mutexes with single-producer single-consumer (SPSC) ring buffers.
* **FIX protocol gateway**: Implement Financial Information eXchange interface.
* **Multi-symbol support**: Extend engine to match multiple tickers concurrently.
* **Persistent storage**: Integrate memory-mapped file persistence (MMAP) for recovery.
* **Replay engine**: Build historical order book state replay tools.
* **Multi-threaded matching**: Thread-per-ticker matching layout.
* **Performance profiling**: Cache-miss and branch-prediction fine-tuning via `perf`.
* **NUMA-aware execution**: Core affinity pinning to minimize context switching overheads.

---

## Tech Stack

* **C++20**
* **STL**
* **Crow**
* **JSON**
* **GoogleTest**
* **CMake**
* **WebSockets**
* **REST API**

---

## Repository Structure

```
OrderBook/
├── benchmark/
│   ├── CMakeLists.txt
│   └── BenchmarkRunner.cpp
├── include/
│   ├── Common.hpp
│   ├── **OrderBook.hpp**
│   ├── **MatchingEngine.hpp**
│   ├── **Statistics.hpp**
│   ├── **RiskEngine.hpp**
│   ├── **Portfolio.hpp**
│   ├── WebServer.hpp
│   └── Logger.hpp
├── src/
│   ├── OrderBook.cpp
│   ├── MatchingEngine.cpp
│   ├── Statistics.cpp
│   ├── RiskEngine.cpp
│   ├── Portfolio.cpp
│   ├── WebServer.cpp
│   └── main.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── MatchingTests.cpp
│   └── CancelTests.cpp
├── docs/
├── assets/
│   └── dashboard.png      # High-fidelity dashboard UI mockup
├── CMakeLists.txt
└── README.md
```
