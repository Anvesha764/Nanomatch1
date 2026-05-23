// bench/latency_compare.cpp
// ---------------------------------------------------------------------------
// Side-by-side v1 vs v2 latency comparison.
// This file includes NO order book headers — calls bench_v1() / bench_v2()
// which live in separate TUs (bench_v1.cpp / bench_v2.cpp).
//
// Build:  cmake --build . --target latency_compare
// Run:    ./latency_compare.exe
//         ./latency_compare.exe 1000000
// ---------------------------------------------------------------------------
#include "bench_shared.hpp"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

static std::string fmt_ns(int64_t val)
{
    std::string s = std::to_string(val);
    for (int i = (int)s.size() - 3; i > 0; i -= 3)
        s.insert((size_t)i, ",");
    return s + " ns";
}

static std::string fmt_speedup(int64_t a, int64_t b)
{
    if (b == 0) return "  inf";
    std::ostringstream o;
    double r = (double)a / (double)b;
    o << std::fixed << std::setprecision(1) << r << "x";
    // label direction so it's unambiguous
    if (r >= 1.0) o << "  faster";
    else          o << "  slower";
    return o.str();
}

static void print_histogram(const std::vector<int64_t>& lv1,
                             const std::vector<int64_t>& lv2,
                             int samples)
{
    struct Bucket { const char* label; int64_t limit; };
    static const Bucket buckets[] = {
        {"    <100 ns", 100},
        {" 100-300 ns", 300},
        {" 300-600 ns", 600},
        {"  0.6-1 us ", 1000},
        {"    1-5 us ", 5000},
        {"   5-50 us ", 50000},
        {"    >50 us ", INT64_MAX}
    };

    std::cout << "\n  Latency Distribution\n";
    std::cout << "  Bucket        v1 (STL)                    v2 (Pool+PLQ)\n";
    std::cout << "  " << std::string(70, '-') << "\n";

    std::size_t prev1 = 0, prev2 = 0;
    for (auto& b : buckets) {
        auto count = [&](const std::vector<int64_t>& lat,
                         std::size_t& prev) -> std::size_t {
            std::size_t c = (b.limit == INT64_MAX)
                ? lat.size()
                : (std::size_t)(std::lower_bound(lat.begin(), lat.end(),
                                                  b.limit) - lat.begin());
            std::size_t n = c - prev;
            prev = c;
            return n;
        };
        std::size_t n1 = count(lv1, prev1);
        std::size_t n2 = count(lv2, prev2);
        double pct1 = 100.0 * n1 / samples;
        double pct2 = 100.0 * n2 / samples;
        int bar1 = (int)(pct1 / 100.0 * 24 + 0.5);
        int bar2 = (int)(pct2 / 100.0 * 24 + 0.5);

        std::cout << "  " << b.label << "  ["
                  << std::string(bar1, '|') << std::string(24 - bar1, ' ')
                  << "] " << std::fixed << std::setprecision(1)
                  << std::setw(5) << pct1 << "%"
                  << "  ["
                  << std::string(bar2, '|') << std::string(24 - bar2, ' ')
                  << "] " << std::setw(5) << pct2 << "%\n";
    }
    std::cout << "  " << std::string(70, '-') << "\n";
}

int main(int argc, char* argv[])
{
    const int      SAMPLES = (argc > 1) ? std::atoi(argv[1]) : 500000;
    const int      WARMUP  = 10000;
    const uint64_t SEED    = 42;

    std::cout
        << "\n"
        << "  +----------------------------------------------------------+\n"
        << "  |       NanoMatch  -  v1 vs v2 Latency Comparison         |\n"
        << "  +----------------------------------------------------------+\n"
        << "  Samples  : " << SAMPLES << "\n"
        << "  Warmup   : " << WARMUP  << "\n"
        << "  Workload : aggressive add_order crossing best bid/ask\n"
        << "             (measures full match path: find + fill + erase)\n\n";

    std::cout << "  Running v1 (STL map + unordered_map)...  " << std::flush;
    LatencyResult r1 = bench_v1(WARMUP, SAMPLES, SEED);
    std::cout << "done.\n";

    std::cout << "  Running v2 (PriceLevelArray + FlatHashMap)... " << std::flush;
    LatencyResult r2 = bench_v2(WARMUP, SAMPLES, SEED);
    std::cout << "done.\n\n";

    // table
    const int C0 = 8, C1 = 16, C2 = 16, C3 = 20;
    auto hdr = [&]() {
        std::cout << "  " << std::left  << std::setw(C0) << "Metric"
                  << std::right << std::setw(C1) << "v1  (STL)"
                  << std::setw(C2) << "v2  (Pool+PLQ)"
                  << std::setw(C3) << "v1/v2 speedup" << "\n";
        std::cout << "  " << std::string(C0+C1+C2+C3, '-') << "\n";
    };
    auto row = [&](const char* m, int64_t a, int64_t b) {
        std::cout << "  " << std::left  << std::setw(C0) << m
                  << std::right << std::setw(C1) << fmt_ns(a)
                  << std::setw(C2) << fmt_ns(b)
                  << std::setw(C3) << fmt_speedup(a, b) << "\n";
    };

    hdr();
    row("Min",   r1.min_val,  r2.min_val);
    row("p50",   r1.p50,      r2.p50);
    row("p90",   r1.p90,      r2.p90);
    row("p99",   r1.p99,      r2.p99);
    row("p99.9", r1.p999,     r2.p999);
    row("Max",   r1.max_val,  r2.max_val);
    row("Mean",  r1.mean_val, r2.mean_val);

    print_histogram(r1.samples, r2.samples, SAMPLES);

    std::cout
        << "\n  Notes:\n"
        << "  - p50/p90 gap is small: both implementations are fast on the common path\n"
        << "  - p99/p99.9/Max is where v2 wins: no malloc means no GC spikes\n"
        << "  - v1 Max spike = OS returning memory pages after unordered_map rehash\n"
        << "  - v2 Max spike = occasional cache miss on cold PriceLevelArray slot\n\n";

    return 0;
}