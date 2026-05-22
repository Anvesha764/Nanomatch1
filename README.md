# NanoMatch
### Ultra-Low Latency Order Matching Engine in C++20

> Built to simulate the core infrastructure of a high-frequency trading exchange.  
> Processes **1.38M+ orders/sec** with **p99 latency under 800ns**.

---

## Table of Contents
- [What This Project Does](#what-this-project-does)
- [Architecture](#architecture)
- [Tech Stack](#tech-stack)
- [Project Structure](#project-structure)
- [Environment Setup](#environment-setup)
- [Build Instructions](#build-instructions)
- [Running the Project](#running-the-project)
- [Performance Results](#performance-results)
- [Key Design Decisions](#key-design-decisions)
- [What I'd Do Next](#what-id-do-next)

---

## What This Project Does

An **Order Matching Engine** is the heart of every stock exchange - it maintains a live order book of all buy and sell orders and matches them by **price-time priority** (best price first; among equal prices, earliest order first).

NanoMatch builds this from scratch with HFT-grade systems engineering:

| Component | Description |
|---|---|
| **Limit Order Book** | Price-time priority matching for buy/sell limit orders |
| **PriceLevelQueue** | Fixed circular buffer per price level - zero heap allocation on hot path |
| **Pool Allocator** | Pre-allocated memory slab - zero `malloc` calls during matching |
| **ITCH 5.0 Parser** | Binary parser for real NASDAQ market data format |
| **SPSC Ring Buffer** | Lock-free Single-Producer Single-Consumer queue for trade logging |
| **Trade Logger** | Async CSV trade logger with zero drops on 1M order run |
| **Latency Profiler** | p50/p90/p99/p999 histogram over 500,000 samples |

---

## Architecture

```
|-------------------------------------------------------------|
│                        MATCHING THREAD                      │
│                                                             │
│   |-------------|    |--------------|    |---------------|  │
│   │ ITCH Parser │--->│  Order Book  │--->│ Trade Events  │  │
│   │ (fread I/O) │    │ (pool alloc) │    │               │  │
│   |-------------|    |--------------|    |-------┬-------|  │
│                                                  │          │
│                                                  |          │
│                                       |-----------------|   │
│                                       │ SPSC Ring Buffer│   │
│                                       |--------┬--------|   │
|------------------------------------------------┼------------|
                                                 │
|--------------------------------------------------------------|
│                        LOGGER THREAD                         │
│                    trades.csv output                         │
|--------------------------------------------------------------|
```

The matching thread never touches I/O - it only pushes `TradeEvent` structs onto the ring buffer. The logger thread drains it asynchronously.

---

## Tech Stack

| Tool | Purpose |
|---|---|
| C++20 | Core language - concepts, designated initializers, `std::atomic` |
| CMake 3.20+ | Build system |
| NASDAQ ITCH 5.0 | Real binary market data format |
| `std::atomic` | Lock-free SPSC ring buffer |
| `alignas(64)` | False sharing elimination |
| Google Benchmark | Throughput measurement (Phase 5) |
| Windows / MinGW64 | Development environment |

---

## Project Structure

```
nanomatch/
│
|-- CMakeLists.txt              # Build configuration - 5 executables
|-- README.md                   # This file
|-- .gitignore                  # Excludes build/, binaries, data files
│
|-- include/                    # All header files
│   |-- types.hpp               # OrderId, Price, Quantity, Side
│   |-- order.hpp               # Order struct with default constructor
│   |-- order_book.hpp          # Phase 1 - STL baseline LOB
│   |-- order_book_v1.hpp       # Phase 1 copy (for comparison)
│   |-- order_book_v2.hpp       # Phase 2 - optimized LOB
│   |-- pool_allocator.hpp      # STL-compatible pool allocator
│   |-- price_level.hpp         # Fixed circular buffer per price level
│   |-- itch_parser.hpp         # ITCH 5.0 message structs + callbacks
│   |-- spsc_queue.hpp          # Lock-free ring buffer
│   |-- trade_logger.hpp        # Trade logger (CSV output)
│
|-- src/                        # Implementation files
│   |-- main.cpp                # Phase 1 entry point
│   |-- order_book.cpp          # Phase 1 matching logic
│   |-- order_book_v1.cpp       # Phase 1 copy
│   |-- main_v2.cpp             # Phase 2 entry point + benchmark
│   |-- order_book_v2.cpp       # Phase 2 matching logic
│   |-- main_v3.cpp             # Phase 3 - ITCH ingestion
│   |-- itch_parser.cpp         # Binary ITCH file parser
│   |-- main_v4.cpp             # Phase 4 - trade logger wired in
│   |-- generate_itch.cpp       # Synthetic test data generator
│
|-- bench/                      # Benchmarks
│   |-- bench_orderbook.cpp     # Google Benchmark suite
│   |-- latency_histogram.cpp   # p50/p99/p999 manual profiler
│
|-- tests/
    |-- test_matching.cpp       # Unit tests
```

---

## Environment Setup

### Requirements

| Dependency | Version | Install |
|---|---|---|
| GCC | 11+ | MSYS2: `pacman -S mingw-w64-x86_64-gcc` |
| CMake | 3.20+ | MSYS2: `pacman -S mingw-w64-x86_64-cmake` |
| Git | any | MSYS2: `pacman -S git` |

> Developed and tested on **Windows 11 with MSYS2 MinGW64 + GCC 16.1.0**.  
> Also works on Ubuntu 20.04+ with `sudo apt install build-essential cmake`.  
> Note: `mmap` and `perf` features require Linux. Windows build uses `fread` equivalent.

---

### Step-by-step setup

**1. Clone the repo**
```bash
git clone https://github.com/Anvesha764/Nanomatch1.git
cd Nanomatch1
```

**2. Create build directory and configure**
```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
```

> On Linux, omit `-G "MinGW Makefiles"`:
> ```bash
> cmake .. -DCMAKE_BUILD_TYPE=Release
> ```

**3. Build all targets**
```bash
cmake --build .
```

This produces:
```
nanomatch_v1.exe    - Phase 1: STL baseline
nanomatch_v2.exe    - Phase 2: optimized + benchmark
nanomatch_v3.exe    - Phase 3: ITCH parser
nanomatch_v4.exe    - Phase 4: trade logger
generate_itch.exe   - synthetic data generator
latency_hist.exe    - latency histogram
```

---

## Running the Project

### Phase 1 - Core matching engine
```bash
./nanomatch_v1.exe
```
```
=== ORDER BOOK ===
   ASK QTY |    PRICE | BID QTY
-----------------------------------
       100 |    10070 |
       150 |    10060 |
-----------------------------------
           |    10050 | 100
           |    10040 | 200
==================
>> Incoming SELL order: price=10030, qty=120
TRADE: buy=1 sell=5 px=10050 qty=100
TRADE: buy=2 sell=5 px=10040 qty=20
```

### Phase 2 - Optimized engine + benchmark
```bash
./nanomatch_v2.exe
```
```
>> Benchmarking 100,000 orders...
100,000 orders in ~14,000,000 ns
Average: 141 ns/order
```

### Phase 3 - ITCH data ingestion
```bash
# Generate 1M synthetic ITCH messages
./generate_itch.exe

# Run parser
./nanomatch_v3.exe test_data.itch AAPL
```
```
Total messages : 1000000
Trades matched : 398206
Throughput     : 2,083,301 msg/sec
```

### Phase 4 - Trade logger
```bash
./nanomatch_v4.exe test_data.itch AAPL
```
```
Trades matched : 398206
Trades logged  : 398206
Trades dropped : 0
```
Produces `trades.csv` with all matched trades.

### Phase 5 - Latency histogram
```bash
./latency_hist.exe
```
```
|==================================|
|   NanoMatch Latency Report       |
|==================================|
|  Samples :     500000            |
|==================================|
|  Min     :    100 ns             |
|  p50     :    300 ns             |
|  p90     :    500 ns             |
|  p99     :    800 ns             |
|  p99.9   :   6300 ns             |
|==================================|
```

---

## Performance Results

Tested on: Windows 11, Intel Core i5, MSYS2 MinGW64, GCC 16.1.0, `-O3`

| Metric | Result |
|---|---|
| p50 latency | **300 ns** |
| p90 latency | **500 ns** |
| p99 latency | **800 ns** |
| p99.9 latency | **6,300 ns** |
| Throughput (ITCH parser) | **2.08M msg/sec** |
| Throughput (order matching) | **1.38M msg/sec** |
| Trades logged | **398,206** |
| Trades dropped | **0** |
| `malloc` calls on hot path | **0** |

---

## Key Design Decisions

### 1. Integer prices, never floats
Prices stored as `int64_t` ticks (e.g. `1000000` = $100.00 at 4 decimal places). IEEE 754 floating point fails exact equality checks. Integer arithmetic compiles to single CPU instructions with zero rounding error.

### 2. PriceLevelQueue - circular buffer (Phase 2)
Each price level holds a fixed-size FIFO of orders stored inline in the map node - no pointer chasing. `push_back` and `pop_front` are O(1) index arithmetic. No heap allocation after construction.

### 3. Pool Allocator (Phase 2)
`std::map` normally calls `malloc` for every node insertion. The pool allocator pre-allocates a slab and hands out blocks via a free list in O(1). Zero system calls after construction.

### 4. ITCH 5.0 Binary Parser (Phase 3)
Real NASDAQ market data is big-endian packed binary structs. We use `#pragma pack(1)` and manual `bswap` intrinsics to parse each message in a single memory read. Callback pattern decouples the parser from the order book for testability.

### 5. SPSC Ring Buffer with acquire/release semantics (Phase 4)
`memory_order_release` on writes and `memory_order_acquire` on reads enforce the happens-before relationship without any mutex. `alignas(64)` separates head and tail onto different cache lines, eliminating false sharing between producer and consumer.

### 6. Separate v1/v2 executables
Both STL baseline (`nanomatch_v1`) and optimized (`nanomatch_v2`) are built from the same codebase. This makes the performance delta measurable and demonstrable.

---

## What I'd Do Next

- **Linux migration** - enable `mmap` + `MADV_SEQUENTIAL` for zero-copy file I/O instead of `fread`
- **True multithreaded SPSC** - run logger as a real background thread on Linux where pthreads behave correctly
- **Google Benchmark integration** - p50/p99 via `bench/bench_orderbook.cpp` with `--benchmark_repetitions`
- **CPU flame graphs** - `perf record` + Brendan Gregg's FlameGraph on Linux to find exact bottlenecks
- **Kernel bypass networking** - DPDK to receive live market data without OS involvement
- **Order Replace ('U' message)** - implement ITCH replace as atomic cancel + re-add

---

## References

- [NASDAQ ITCH 5.0 Specification](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts NQTVITCHspecification.pdf)
- [Preshing - Acquire and Release Semantics](https://preshing.com/20120913/acquire-and-release-semantics/)
- [Brendan Gregg - Flame Graphs](https://www.brendangregg.com/flamegraphs.html)
- [Google Benchmark](https://github.com/google/benchmark)

---

## Author

**Anvesha Singh** - IIT Guwahati  
Built as a systems engineering portfolio project demonstrating C++ low-latency techniques applied to financial exchange infrastructure.
