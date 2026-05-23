#pragma once
#include "order.hpp"
#include "price_level.hpp"
#include "price_level_array.hpp"
#include "flat_hash_map.hpp"
#include <vector>

// ---------------------------------------------------------------------------
// Trade record produced when two orders cross.
// ---------------------------------------------------------------------------
struct Trade {
    OrderId  buy_order_id;
    OrderId  sell_order_id;
    Price    price;
    Quantity quantity;
};

// ---------------------------------------------------------------------------
// Type aliases
// ---------------------------------------------------------------------------

using PriceLevel = PriceLevelQueue<64>;

// Bids: descending (highest price = best bid = slot 0)
using BidBook = PriceLevelArray<256, /*Descending=*/true>;

// Asks: ascending (lowest price = best ask = slot 0)
using AskBook = PriceLevelArray<256, /*Descending=*/false>;

// Order index: maps OrderId → (Side, Price).
// 1<<16 = 65536 slots — fine for unit tests and benchmarks up to ~48k live
// orders.  For production-scale workloads bump to 1<<20.
using IndexValue = std::pair<Side, Price>;
using OrderIndex = FlatHashMap<IndexValue, (1 << 16)>;

// ---------------------------------------------------------------------------
// OrderBook_v2
//
// Hot-path allocations: zero.
// Data structures: all inline flat arrays, no STL tree/hash containers.
// ---------------------------------------------------------------------------
class OrderBook_v2 {
public:
    std::vector<Trade> add_order(Order order);
    bool cancel_order(OrderId id);
    void print_book(int depth = 5) const;

    std::size_t bid_levels() const noexcept { return bids_.size(); }
    std::size_t ask_levels() const noexcept { return asks_.size(); }

private:
    BidBook    bids_;
    AskBook    asks_;
    OrderIndex order_index_;

    std::vector<Trade> match(Order& incoming);
};
