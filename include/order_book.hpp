// include/order_book.hpp
#pragma once
#include "order.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>

// Price level: all orders at one price, in FIFO order
using PriceLevel = std::vector<Order>;

struct Trade {
    OrderId  buy_order_id;
    OrderId  sell_order_id;
    Price    price;
    Quantity quantity;
};

class OrderBook {
public:
    // returns list of trades generated
    std::vector<Trade> add_order(Order order);
    bool               cancel_order(OrderId id);
    void               print_book(int depth = 5) const;

private:
    // bids: highest price first  → use greater<Price>
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    // asks: lowest price first   → default less<Price>
    std::map<Price, PriceLevel>                       asks_;

    // fast cancel: id → (side, price)
    std::unordered_map<OrderId, std::pair<Side, Price>> order_index_;

    std::vector<Trade> match(Order& incoming);
};