#include "order_book_v1.hpp"
#include <iostream>
#include <chrono>

int main() {
    OrderBook book;
    uint64_t ts = 0;

    std::cout << "--- NANOMATCH v1: CORE ENGINE TEST ---\n\n";

    // Add resting orders — none of these cross, so no trades yet
    book.add_order({1, 10040, 10, Side::BUY,  ++ts});
    book.add_order({2, 10050, 15, Side::BUY,  ++ts});
    book.add_order({3, 10050,  5, Side::BUY,  ++ts});
    book.add_order({4, 10070, 20, Side::SELL, ++ts});

    book.print_book();

    // Incoming SELL that crosses the bids
    std::cout << "\n>> Incoming SELL order: price=10040, qty=12\n";
    auto trades = book.add_order({5, 10040, 12, Side::SELL, ++ts});

    for (auto& t : trades)
        std::cout << "TRADE: buy=" << t.buy_order_id
                  << " sell=" << t.sell_order_id
                  << " px=" << t.price
                  << " qty=" << t.quantity << "\n";

    book.print_book();
    return 0;
}