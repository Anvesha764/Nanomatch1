// bench/bench_v2.cpp — only includes order_book_v2.hpp
//
// FlatHashMap has 1<<16 = 65536 slots.
// With 500k orders, it fills up and linear probing spins forever.
// Fix: reset the book every BATCH orders so the map never exceeds ~50% load.
#include "bench_shared.hpp"
#include "order_book_v2.hpp"

#include <algorithm>
#include <chrono>
#include <memory>

using Clock = std::chrono::high_resolution_clock;
using ns    = std::chrono::nanoseconds;

static void prime(OrderBook_v2& book, uint64_t& next_id)
{
    const Price base = 1000000;
    // 4 bid + 4 ask levels, 1 order each — well within all caps
    for (int i = 1; i <= 4; ++i) {
        book.add_order({next_id++, base - Price(i*100), 10, Side::BUY,  (Timestamp)next_id});
        book.add_order({next_id++, base + Price(i*100), 10, Side::SELL, (Timestamp)next_id});
    }
}

LatencyResult bench_v2(int /*warmup*/, int samples, uint64_t /*seed*/)
{
    // FlatHashMap cap = 65536. Each timed iteration does 2 inserts + 2 erases
    // (1 aggressive + 1 reseed). Reset every 20000 iterations to stay well
    // under the load factor limit.
    const int BATCH = 20000;
    const Price base = 1000000;

    std::vector<int64_t> lat;
    lat.reserve(samples);

    uint64_t next_id = 1;
    auto book = std::make_unique<OrderBook_v2>();
    prime(*book, next_id);

    for (int i = 0; i < samples; ++i) {
        // Reset book periodically to prevent FlatHashMap overflow
        if (i > 0 && i % BATCH == 0) {
            book = std::make_unique<OrderBook_v2>();
            prime(*book, next_id);
        }

        // Timed: aggressive BUY that hits the best ask (base+100)
        // qty=10 exactly matches the resting order qty=10 -> full fill -> level erased
        Side  s  = (i % 2 == 0) ? Side::BUY  : Side::SELL;
        Price px = (i % 2 == 0) ? base + 100  : base - 100;

        auto t0 = Clock::now();
        book->add_order({next_id++, px, 10, s, (Timestamp)next_id});
        auto t1 = Clock::now();
        lat.push_back(std::chrono::duration_cast<ns>(t1 - t0).count());

        // Reseed the consumed level (NOT timed)
        Side  rs  = (s  == Side::BUY)  ? Side::SELL : Side::BUY;
        Price rpx = (s  == Side::BUY)  ? base + 100 : base - 100;
        book->add_order({next_id++, rpx, 10, rs, (Timestamp)next_id});
    }

    std::sort(lat.begin(), lat.end());

    auto pct = [&](double p) -> int64_t {
        return lat[(size_t)(p / 100.0 * lat.size())];
    };
    int64_t sum = 0;
    for (auto x : lat) sum += x;

    return { lat, pct(50), pct(90), pct(99), pct(99.9),
             lat.front(), lat.back(), sum / (int64_t)lat.size() };
}