# ⚡ Low-Latency Exchange Matching Engine

A high-performance electronic exchange simulation platform written in C++20, implementing price-time priority order matching, pre-trade risk management, portfolio accounting, market statistics, REST APIs, and real-time WebSocket market data feeds.

Built with a focus on low-latency systems design and modern exchange infrastructure.

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

---

## Overview

This project simulates the internal infrastructure of an electronic exchange.

Incoming client orders are validated by a pre-trade risk engine before being processed by a price-time priority matching engine.

Executed trades update trader portfolios, market statistics, and are broadcast through REST and WebSocket interfaces.

The system was designed with low latency and efficient memory management as primary goals.

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

### Environment
* **CPU**: AMD Ryzen 9 7900X (12 Cores, 24 Threads, 4.7 GHz base clock)
* **RAM**: 32 GB DDR5 (5200 MHz)
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
│   ├── OrderBook.hpp
│   ├── MatchingEngine.hpp
│   ├── Statistics.hpp
│   ├── RiskEngine.hpp
│   ├── Portfolio.hpp
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
├── CMakeLists.txt
└── README.md
```
