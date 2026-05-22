#pragma once
#include "order.hpp"
#include "price_level.hpp"
#include <map>
#include <unordered_map>
#include <functional>
#include <vector>

struct Trade {
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
};

using PriceLevel = PriceLevelQueue<64>;

using BidMap = std::map<Price, PriceLevel, std::greater<Price>>;
using AskMap = std::map<Price, PriceLevel>;

class OrderBook_v2 {
public:
    std::vector<Trade> add_order(Order order);
    bool cancel_order(OrderId id);
    void print_book(int depth = 5) const;

    std::size_t bid_levels() const { return bids_.size(); }
    std::size_t ask_levels() const { return asks_.size(); }

private:
    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderId, std::pair<Side, Price>> order_index_;

    std::vector<Trade> match(Order& incoming);
};
