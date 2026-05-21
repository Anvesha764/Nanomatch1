// src/main_v2.cpp
#include "order_book_v2.hpp"
#include <iostream>
#include <chrono>

int main() {
    OrderBook_v2 book;
    uint64_t ts = 0;

    // seed the book — same as v1
    book.add_order({1, 10050, 100, Side::BUY,  ++ts});
    book.add_order({2, 10040, 200, Side::BUY,  ++ts});
    book.add_order({3, 10060, 150, Side::SELL, ++ts});
    book.add_order({4, 10070, 100, Side::SELL, ++ts});

    book.print_book();

    std::cout << "\n>> Incoming SELL order: price=10030, qty=120\n";
    auto trades = book.add_order({5, 10030, 120, Side::SELL, ++ts});

    for (auto& t : trades)
        std::cout << "TRADE: buy=" << t.buy_order_id
                  << " sell=" << t.sell_order_id
                  << " px=" << t.price
                  << " qty=" << t.quantity << "\n";

    book.print_book();

    // ── Benchmark: 100k orders, measure time ──────────────────
    std::cout << "\n>> Benchmarking 100,000 orders...\n";

    OrderBook_v2 bench_book;
    using Clock = std::chrono::high_resolution_clock;

    auto t0 = Clock::now();
    for (int i = 0; i < 100000; ++i) {
        Side  side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        Price px   = 10000 + (i % 100) * 10;
        bench_book.add_order({(OrderId)(i + 100), px, 100,
                               side, (Timestamp)(i + 100)});
    }
    auto t1 = Clock::now();

auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

    std::cout << "100,000 orders in " << ns << " ns\n"
              << "Average           " << ns / 100000 << " ns/order\n";

    return 0;
}