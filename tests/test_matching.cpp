#include "order_book_v2.hpp"   // only this — avoids Trade redefinition
#include <cassert>
#include <iostream>

void test_basic_match() {
    OrderBook_v2 book;
    book.add_order({1, 10050, 100, Side::BUY,  1});
    auto trades = book.add_order({2, 10050, 100, Side::SELL, 2});
    assert(trades.size() == 1);
    assert(trades[0].price    == 10050);
    assert(trades[0].quantity == 100);
    assert(book.bid_levels()  == 0);
    assert(book.ask_levels()  == 0);
    std::cout << "[PASS] test_basic_match\n";
}

void test_partial_fill() {
    OrderBook_v2 book;
    book.add_order({1, 10050, 200, Side::BUY, 1});
    auto trades = book.add_order({2, 10050, 50, Side::SELL, 2});
    assert(trades.size()      == 1);
    assert(trades[0].quantity == 50);
    assert(book.bid_levels()  == 1);
    std::cout << "[PASS] test_partial_fill\n";
}

void test_no_cross() {
    OrderBook_v2 book;
    book.add_order({1, 10000, 100, Side::BUY,  1});
    auto trades = book.add_order({2, 10100, 100, Side::SELL, 2});
    assert(trades.empty());
    assert(book.bid_levels() == 1);
    assert(book.ask_levels() == 1);
    std::cout << "[PASS] test_no_cross\n";
}

void test_cancel_order() {
    OrderBook_v2 book;
    book.add_order({1, 10050, 100, Side::BUY, 1});
    bool cancelled = book.cancel_order(1);
    assert(cancelled);
    assert(book.bid_levels() == 0);
    auto trades = book.add_order({2, 10050, 100, Side::SELL, 2});
    assert(trades.empty());
    std::cout << "[PASS] test_cancel_order\n";
}

void test_price_time_priority() {
    OrderBook_v2 book;
    book.add_order({1, 10050, 100, Side::BUY, 1});
    book.add_order({2, 10050, 100, Side::BUY, 2});
    auto trades = book.add_order({3, 10050, 100, Side::SELL, 3});
    assert(trades.size()          == 1);
    assert(trades[0].buy_order_id == 1);
    std::cout << "[PASS] test_price_time_priority\n";
}

// Tests that a sell order matches the HIGHEST resting bid, not the lowest.
void test_sell_matches_highest_bid() {
    OrderBook_v2 book;
    book.add_order({1, 10040, 100, Side::BUY, 1});
    book.add_order({2, 10060, 100, Side::BUY, 2});
    auto trades = book.add_order({3, 10050, 100, Side::SELL, 3});
    assert(trades.size()          == 1);
    assert(trades[0].price        == 10060);
    assert(trades[0].buy_order_id == 2);
    std::cout << "[PASS] test_sell_matches_highest_bid\n";
}

void test_sweep_multiple_levels() {
    OrderBook_v2 book;
    book.add_order({1, 10060, 50, Side::BUY, 1});
    book.add_order({2, 10050, 50, Side::BUY, 2});
    book.add_order({3, 10040, 50, Side::BUY, 3});
    auto trades = book.add_order({4, 10030, 150, Side::SELL, 4});
    assert(trades.size()     == 3);
    assert(book.bid_levels() == 0);
    std::cout << "[PASS] test_sweep_multiple_levels\n";
}

int main() {
    test_basic_match();
    test_partial_fill();
    test_no_cross();
    test_cancel_order();
    test_price_time_priority();
    test_sell_matches_highest_bid();
    test_sweep_multiple_levels();
    std::cout << "\nAll tests passed.\n";
    return 0;
}
