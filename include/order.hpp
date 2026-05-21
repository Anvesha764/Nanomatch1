// include/order.hpp
#pragma once
#include "types.hpp"

struct Order {
    OrderId   id       = 0;
    Price     price    = 0;
    Quantity  quantity = 0;
    Quantity  filled_qty = 0;
    Side      side     = Side::BUY;
    Timestamp timestamp = 0;

    // Default constructor (needed by std::array)
    Order() = default;

    // Parameterized constructor
    Order(OrderId id, Price px, Quantity qty, Side s, Timestamp ts)
        : id(id), price(px), quantity(qty), filled_qty(0),
          side(s), timestamp(ts) {}

    Quantity remaining() const { return quantity - filled_qty; }
    bool     is_filled() const { return filled_qty >= quantity; }
};