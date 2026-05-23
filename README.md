# NanoMatch

A limit order book and matching engine written in C++20. Built to understand what actually happens inside an exchange — not the finance layer, but the systems layer underneath it.

p99 latency under 400ns on the optimized path. Zero heap allocations on the hot path. Parses real NASDAQ ITCH 5.0 binary format.

---

## What it does

Maintains a live order book of resting buy/sell orders and matches incoming orders by price-time priority — best price first, and among equal prices, the earliest order wins. That's the rule every exchange runs on.

The project is split into four phases, each adding a layer:

- **Phase 1** — basic matching engine with STL containers (`std::map`, `std::vector`)
- **Phase 2** — swap in a fixed circular buffer per price level and a flat hash map to kill heap allocation on the hot path
- **Phase 3** — wire in a binary ITCH 5.0 parser and run against synthetic market data
- **Phase 4** — add a lock-free SPSC ring buffer and async trade logger that writes to CSV without touching the matching thread

---

## Numbers

Tested on Windows 11, Intel Core i5, GCC 16.1.0 with `-O3`, 500k samples:

### Latency: v1 (STL baseline) vs v2 (optimized)

| Metric | v1 — STL (`map` + `unordered_map`) | v2 — PriceLevelArray + FlatHashMap | Speedup |
|---|---|---|---|
| p50 latency | 300 ns | 200 ns | 1.5x |
| p90 latency | 400 ns | 300 ns | 1.3x |
| p99 latency | 900 ns | 400 ns | 2.2x |
| p99.9 latency | 6,900 ns | 600 ns | 11.5x |
| Max latency | 6,510,200 ns | 219,000 ns | 29.7x |
| Mean latency | 355 ns | 256 ns | 1.4x |

> Workload: aggressive `add_order` crossing the best bid/ask, measuring the full match path (find + fill + erase). 500k samples, seed 42, GCC `-O3 -march=native`.

The story is in the tail. p50 and p90 are similar because both implementations execute a single match quickly. The gap opens at p99 and beyond: v1's `unordered_map` periodically rehashes and `_Rb_tree` calls `malloc` per new price level — those allocations show up as latency spikes. v2 has zero heap allocation on the hot path so the tail stays clean.

### System throughput (v2)

| | |
|---|---|
| Throughput (ITCH parser) | 2.08M msg/sec |
| Throughput (order matching) | 1.38M msg/sec |
| Trades logged | 398,206 |
| Trades dropped | 0 |
| `malloc` calls on hot path | 0 |

---

## Build

Requires GCC 11+ and CMake 3.20+.

```bash
git clone https://github.com/Anvesha764/Nanomatch1.git
cd Nanomatch1
mkdir build && cd build

# Windows (MSYS2 MinGW64)
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Linux
cmake .. -DCMAKE_BUILD_TYPE=Release

cmake --build .
```

This builds the following targets:

| Target | Description |
|---|---|
| `nanomatch_v1` | Phase 1 — STL matching engine |
| `nanomatch_v2` | Phase 2 — optimized matching engine |
| `nanomatch_v3` | Phase 3 — ITCH parser + ingestion |
| `nanomatch_v4` | Phase 4 — SPSC trade logger |
| `generate_itch` | Synthetic ITCH data generator |
| `latency_hist` | Latency histogram for v2 |
| `latency_hist_v1` | Latency histogram for v1 |
| `latency_compare` | Side-by-side v1 vs v2 comparison |
| `run_tests` | Unit tests |

---

## Running

**Phase 1 — basic match:**
```bash
./nanomatch_v1.exe
```

**Phase 2 — optimized:**
```bash
./nanomatch_v2.exe
```

**Phase 3 — ITCH ingestion:**
```bash
./generate_itch.exe                          # writes test_data.itch
./nanomatch_v3.exe test_data.itch AAPL
```

**Phase 4 — with trade logger:**
```bash
./nanomatch_v4.exe test_data.itch AAPL      # writes trades.csv
```

**Side-by-side latency comparison (v1 vs v2):**
```bash
./latency_compare.exe                        # 500k samples
./latency_compare.exe 1000000               # custom sample count
```

**Latency histogram — v2:**
```bash
./latency_hist.exe
```

**Latency histogram — v1:**
```bash
./latency_hist_v1.exe
```

**Tests:**
```bash
./run_tests.exe
```

---

## Design decisions worth noting

**Integer prices.** Everything is stored as `int64_t` ticks (e.g. `1000000` = $100.00). Float equality is broken for this use case.

**PriceLevelQueue.** Each price level is a fixed-size circular buffer stored inline in the array slot. No pointer chasing, no heap allocation after construction. `push_back` and `pop_front` are index arithmetic. Capacity: 64 orders per level.

**PriceLevelArray.** Replaces `std::map<Price, PriceLevelQueue>`. Flat sorted array of `{price, queue}` slots — binary search for lookup, memmove for insert/erase. Cache-friendly vs. pointer-chasing of red-black tree nodes. Capacity: 256 price levels per side.

**FlatHashMap.** Replaces `std::unordered_map<OrderId, {Side, Price}>`. Open-addressing with linear probing, all storage inline — no heap allocation, no rehash on the hot path. Fibonacci hashing for good distribution of sequential order IDs.

**ITCH parser.** NASDAQ ITCH 5.0 is big-endian packed binary. Manual `bswap` helpers avoid `<arpa/inet.h>` and keep the parser portable across Windows and Linux.

**SPSC queue.** The trade logger runs on a dedicated thread. The matching thread pushes `TradeEvent` structs into a lock-free single-producer single-consumer ring buffer (power-of-two capacity, `std::atomic` head/tail, acquire/release ordering). No mutex, no condition variable, zero contention on the hot path.

**Synthetic data.** `generate_itch.cpp` produces 1M add-order messages for `AAPL` with prices uniformly distributed in a ±$0.50 band around $100.00, all at 100 shares. This creates a higher match rate than real market data (~50% of orders fill immediately) and is intentionally designed to stress the matching path rather than model realistic order flow.