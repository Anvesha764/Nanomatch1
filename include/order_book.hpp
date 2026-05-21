#pragma once
#include "order.hpp"
#include <map>
#include <unordered_map>
#include <vector>
#include <functional>

using PriceLevel = std::vector<Order>;

struct Trade {
    OrderId  buy_order_id;
    OrderId  sell_order_id;
    Price    price;
    Quantity quantity;
};

class OrderBook {
public:
    std::vector<Trade> add_order(Order order);
    bool cancel_order(OrderId id);
    void print_book(int depth = 5) const;

private:
    std::map<Price, PriceLevel, std::greater<Price>> bids_;
    std::map<Price, PriceLevel> asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> order_index_;
    std::vector<Trade> match(Order& incoming);
};