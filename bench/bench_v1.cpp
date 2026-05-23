// bench/bench_v1.cpp — only includes order_book_v1.hpp
// Uses the exact same workload as latency_histogram_v1.cpp (the one that works).
#include "bench_shared.hpp"
#include "order_book_v1.hpp"
#include <algorithm>
#include <chrono>
#include <random>

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

LatencyResult bench_v1(int warmup, int samples, uint64_t seed)
{
    OrderBook book;
    std::mt19937_64 rng(seed);
    uint64_t next_id = 1;
    Price base = 1000000;

    // v1 uses std::map — no capacity limits, so wide price range is fine
    for (int i = 0; i < 500; ++i) {
        book.add_order({next_id++, base - 1000*(i%10), 100, Side::BUY,  (Timestamp)next_id});
        book.add_order({next_id++, base + 1000*(i%10), 100, Side::SELL, (Timestamp)next_id});
    }
    for (int i = 0; i < warmup; ++i) {
        Price px = base + (int64_t)(rng() % 2001) - 1000;
        book.add_order({next_id++, px, 100, Side::BUY, (Timestamp)next_id});
    }

    std::vector<int64_t> lat;
    lat.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        Price px = base + (int64_t)(rng() % 2001) - 1000;
        Side  s  = (i % 3 == 0) ? Side::SELL : Side::BUY;
        auto t0 = Clock::now();
        book.add_order({next_id++, px, 100, s, (Timestamp)next_id});
        auto t1 = Clock::now();
        lat.push_back(std::chrono::duration_cast<ns>(t1 - t0).count());
    }
    std::sort(lat.begin(), lat.end());

    auto pct = [&](double p) -> int64_t {
        return lat[(size_t)(p / 100.0 * lat.size())];
    };
    int64_t sum = 0; for (auto x : lat) sum += x;
    return { lat, pct(50), pct(90), pct(99), pct(99.9), lat.front(), lat.back(), sum/(int64_t)lat.size() };
}