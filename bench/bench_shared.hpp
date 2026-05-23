#pragma once
// bench/bench_shared.hpp
// Shared declarations for the latency_compare split-TU design.
// Neither book header is included here — only types.hpp.
#include "types.hpp"
#include <cstdint>
#include <vector>

struct LatencyResult {
    std::vector<int64_t> samples;   // sorted, nanoseconds
    int64_t p50, p90, p99, p999, min_val, max_val, mean_val;
};

// Implemented in bench_v1.cpp — only includes order_book_v1.hpp
LatencyResult bench_v1(int warmup, int samples, uint64_t seed);

// Implemented in bench_v2.cpp — only includes order_book_v2.hpp
LatencyResult bench_v2(int warmup, int samples, uint64_t seed);