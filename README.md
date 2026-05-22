# NanoMatch

A limit order book and matching engine written in C++20. Built to understand what actually happens inside an exchange — not the finance layer, but the systems layer underneath it.

p99 latency under 700ns on the optimized path. Zero heap allocations on the hot path. Parses real NASDAQ ITCH 5.0 binary format.

---

## What it does

Maintains a live order book of resting buy/sell orders and matches incoming orders by price-time priority — best price first, and among equal prices, the earliest order wins. That's the rule every exchange runs on.

The project is split into four phases, each adding a layer:

- **Phase 1** — basic matching engine with STL containers (`std::map`, `std::vector`)
- **Phase 2** — swap in a fixed circular buffer per price level and a pool allocator to kill heap allocation on the hot path
- **Phase 3** — wire in a binary ITCH 5.0 parser and run against synthetic market data
- **Phase 4** — add a lock-free SPSC ring buffer and async trade logger that writes to CSV without touching the matching thread

---

## Numbers

Tested on Windows 11, Intel Core i5, GCC 16.1.0 with `-O3`:

### Latency: v1 (STL baseline) vs v2 (optimized)

| Metric | v1 — `std::map` + `std::vector` | v2 — Pool + PriceLevelQueue | Improvement |
|---|---|---|---|
| p50 latency | 200 ns | 200 ns | — |
| p90 latency | 500 ns | 500 ns | — |
| p99 latency | 900 ns | 700 ns | 1.3× faster |
| p99.9 latency | 4,800 ns | 3,400 ns | 1.4× faster |

> Tested on Windows 11, Intel Core i5, GCC 16.1.0 with `-O3`, 500k samples each.

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

This builds seven targets: `nanomatch_v1`, `nanomatch_v2`, `nanomatch_v3`, `nanomatch_v4`, `generate_itch`, `latency_hist`, and `latency_hist_v1`.

---

## Running

**Phase 1 — basic match:**
```bash
./nanomatch_v1.exe
```

**Phase 2 — optimized + benchmark:**
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

**Latency histogram (v2 optimized):**
```bash
./latency_hist.exe                           # 500k samples, p50/p90/p99/p999
```

**Latency histogram (v1 STL baseline):**
```bash
./latency_hist_v1.exe                        # same 500k samples, v1 OrderBook
```

---

## Design decisions worth noting

**Integer prices.** Everything is stored as `int64_t` ticks (e.g. `1000000` = $100.00). Float equality is broken for this use case.

**PriceLevelQueue.** Each price level is a fixed-size circular buffer stored inline in the map node. No pointer chasing, no heap allocation after construction. `push_back` and `pop_front` are index arithmetic.

**Pool allocator.** `std::map` normally calls `malloc` per node. The pool allocator pre-allocates a slab and hands out blocks via a free list. Zero system calls on the hot path after init.

**ITCH parser.** NASDAQ ITCH 5.0 is big-endian packed binary.
