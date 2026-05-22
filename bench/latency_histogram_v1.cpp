#include "order_book.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <vector>
#include <random>

using Clock = std::chrono::high_resolution_clock;
using ns = std::chrono::nanoseconds;

int main() {
    const int WARMUP  = 10000;
    const int SAMPLES = 500000;

    OrderBook book;
    std::mt19937_64 rng(42);
    uint64_t next_id = 1;
    Price base = 1000000;

    for (int i = 0; i < 500; ++i) {
        book.add_order({next_id++, base - 1000*(i%10), 100, Side::BUY,  (Timestamp)next_id});
        book.add_order({next_id++, base + 1000*(i%10), 100, Side::SELL, (Timestamp)next_id});
    }
    for (int i = 0; i < WARMUP; ++i) {
        Price px = base + (int64_t)(rng() % 2001) - 1000;
        book.add_order({next_id++, px, 100, Side::BUY, (Timestamp)next_id});
    }

    std::vector<int64_t> latencies;
    latencies.reserve(SAMPLES);

    for (int i = 0; i < SAMPLES; ++i) {
        Price px = base + (int64_t)(rng() % 2001) - 1000;
        Side  s  = (i % 3 == 0) ? Side::SELL : Side::BUY;

        auto t0 = Clock::now();
        book.add_order({next_id++, px, 100, s, (Timestamp)next_id});
        auto t1 = Clock::now();

        latencies.push_back(
            std::chrono::duration_cast<ns>(t1 - t0).count());
    }

    std::sort(latencies.begin(), latencies.end());

    auto pct = [&](double p) -> int64_t {
        return latencies[(size_t)(p / 100.0 * latencies.size())];
    };

    std::cout << "\n";
    std::cout << "|==================================|\n";
    std::cout << "|  NanoMatch v1 Latency Report     |\n";
    std::cout << "|  (STL baseline: map + vector)    |\n";
    std::cout << "|==================================|\n";
    std::cout << "|  Samples : " << std::setw(10) << SAMPLES     << "        |\n";
    std::cout << "|==================================|\n";
    std::cout << "|  Min     : " << std::setw(7)  << pct(0)      << " ns         |\n";
    std::cout << "|  p50     : " << std::setw(7)  << pct(50)     << " ns         |\n";
    std::cout << "|  p90     : " << std::setw(7)  << pct(90)     << " ns         |\n";
    std::cout << "|  p99     : " << std::setw(7)  << pct(99)     << " ns         |\n";
    std::cout << "|  p99.9   : " << std::setw(7)  << pct(99.9)   << " ns         |\n";
    std::cout << "|  Max     : " << std::setw(7)  << pct(100)    << " ns         |\n";
    std::cout << "|==================================|\n";

    std::cout << "\nLatency Distribution:\n";
    struct Bucket { const char* label; int64_t limit; };
    Bucket buckets[] = {
        {"  <100ns  ", 100},
        {"100-200ns ", 200},
        {"200-500ns ", 500},
        {" 0.5-1us  ", 1000},
        {"   1-5us  ", 5000},
        {"    >5us  ", INT64_MAX}
    };

    std::size_t prev = 0;
    for (auto& b : buckets) {
        std::size_t count = (b.limit == INT64_MAX)
            ? latencies.size()
            : (std::size_t)(std::lower_bound(
                latencies.begin(), latencies.end(), b.limit)
                - latencies.begin());

        std::size_t bucket_count = count - prev;
        int bar = (int)(bucket_count * 40 / SAMPLES);
        std::cout << b.label << " |"
                  << std::string(bar, '=')
                  << " " << bucket_count << "\n";
        prev = count;
    }

    return 0;
}
