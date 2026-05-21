// src/main.cpp
#include "order_book.hpp"
#include <iostream>
#include <chrono>

int main() {
    OrderBook book;
    uint64_t ts = 0;

    // seed the book with some resting orders
    book.add_order({1, 10050, 100, Side::BUY,  ++ts});
    book.add_order({2, 10040, 200, Side::BUY,  ++ts});
    book.add_order({3, 10060, 150, Side::SELL, ++ts});
    book.add_order({4, 10070, 100, Side::SELL, ++ts});

    book.print_book();

    // incoming aggressive BUY that crosses the spread
    std::cout << "\n>> Incoming SELL order: price=10030, qty=120\n";
auto trades = book.add_order({5, 10030, 120, Side::SELL, ++ts});

    for (auto& t : trades)
        std::cout << "TRADE: buy=" << t.buy_order_id
                  << " sell=" << t.sell_order_id
                  << " px=" << t.price
                  << " qty=" << t.quantity << "\n";

    book.print_book();
    return 0;
}