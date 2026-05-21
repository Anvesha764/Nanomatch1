// src/order_book.cpp
#include "order_book.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

std::vector<Trade> OrderBook::add_order(Order order) {
    auto trades = match(order);

    if (!order.is_filled()) {
        if (order.side == Side::BUY) {
            bids_[order.price].push_back(order);
        } else {
            asks_[order.price].push_back(order);
        }
        order_index_[order.id] = {order.side, order.price};
    }
    return trades;
}

std::vector<Trade> OrderBook::match(Order& incoming) {
    std::vector<Trade> trades;

    auto do_match = [&]<typename BookType>(BookType& book) {
        while (incoming.remaining() > 0 && !book.empty()) {
            auto it = book.begin();
            Price level_price = it->first;

            bool crosses = (incoming.side == Side::BUY)
              ? incoming.price >= level_price 
              : level_price >= incoming.price;  

            if (!crosses) break;

            PriceLevel& level = it->second;

            while (incoming.remaining() > 0 && !level.empty()) {
                Order& resting = level.front();
                Quantity fill_qty = std::min(incoming.remaining(),
                                             resting.remaining());

                trades.push_back({
                    (incoming.side == Side::BUY) ? incoming.id : resting.id,
                    (incoming.side == Side::BUY) ? resting.id  : incoming.id,
                    level_price,
                    fill_qty
                });

                incoming.filled_qty += fill_qty;
                resting.filled_qty  += fill_qty;

                if (resting.is_filled()) {
                    order_index_.erase(resting.id);
                    level.erase(level.begin());
                }
            }
            if (level.empty()) book.erase(it);
        }
    };

    if (incoming.side == Side::BUY)
        do_match(asks_);
    else
        do_match(bids_);

    return trades;
}

bool OrderBook::cancel_order(OrderId id) {
    auto it = order_index_.find(id);
    if (it == order_index_.end()) return false;

    auto [side, price] = it->second;

    if (side == Side::BUY) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            auto& level = level_it->second;
            level.erase(std::remove_if(level.begin(), level.end(),
                [id](const Order& o){ return o.id == id; }),
                level.end());
            if (level.empty()) bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            auto& level = level_it->second;
            level.erase(std::remove_if(level.begin(), level.end(),
                [id](const Order& o){ return o.id == id; }),
                level.end());
            if (level.empty()) asks_.erase(level_it);
        }
    }

    order_index_.erase(it);
    return true;
}

void OrderBook::print_book(int depth) const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::setw(10) << "ASK QTY"
              << " | " << std::setw(8) << "PRICE"
              << " | " << "BID QTY\n";
    std::cout << std::string(35, '-') << "\n";

    int i = 0;
    std::vector<std::pair<Price, Quantity>> ask_levels;
    for (auto& [px, lvl] : asks_) {
        Quantity total = 0;
        for (auto& o : lvl) total += o.remaining();
        ask_levels.push_back({px, total});
        if (++i >= depth) break;
    }
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it)
        std::cout << std::setw(10) << it->second
                  << " | " << std::setw(8) << it->first << " |\n";

    std::cout << std::string(35, '-') << "\n";

    i = 0;
    for (auto& [px, lvl] : bids_) {
        Quantity total = 0;
        for (auto& o : lvl) total += o.remaining();
        std::cout << std::setw(10) << ""
                  << " | " << std::setw(8) << px
                  << " | " << total << "\n";
        if (++i >= depth) break;
    }
    std::cout << "==================\n";
}