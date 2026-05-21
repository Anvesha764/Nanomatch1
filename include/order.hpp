// include/order.hpp
#pragma once
#include "types.hpp"

struct Order {
    OrderId   id;
    Price     price;
    Quantity  quantity;
    Quantity  filled_qty;   // how much has been matched
    Side      side;
    Timestamp timestamp;    // for time priority

    Order(OrderId id, Price px, Quantity qty, Side s, Timestamp ts)
        : id(id), price(px), quantity(qty), filled_qty(0), side(s), timestamp(ts) {}

    Quantity remaining() const { return quantity - filled_qty; }
    bool     is_filled() const { return filled_qty >= quantity; }
};