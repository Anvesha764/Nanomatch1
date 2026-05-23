#pragma once
#include "types.hpp" 

// ---------------------------------------------------------------------------
// Order
// ---------------------------------------------------------------------------
struct alignas(32) Order {
    OrderId   id         = 0;
    Price     price      = 0;
    Quantity  quantity   = 0;
    Quantity  filled_qty = 0;   // how much has been matched so far
    Side      side       = Side::BUY;
    OrderType type       = OrderType::LIMIT;
    Timestamp timestamp  = 0;

    Order() = default;

    Order(OrderId id_, Price px, Quantity qty, Side s, Timestamp ts,
          OrderType t = OrderType::LIMIT)
        : id(id_), price(px), quantity(qty), filled_qty(0),
          side(s), type(t), timestamp(ts) {}

    // How much quantity is still unfilled
    Quantity remaining() const { return quantity - filled_qty; }

    // True once the order is completely matched
    bool is_filled() const { return filled_qty >= quantity; }
};