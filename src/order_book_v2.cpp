#include "order_book_v2.hpp"
#include <iostream>
#include <iomanip>

std::vector<Trade> OrderBook_v2::add_order(Order order) {
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

std::vector<Trade> OrderBook_v2::match(Order& incoming) {
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
                    level.pop_front();
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

bool OrderBook_v2::cancel_order(OrderId id) {
    auto it = order_index_.find(id);
    if (it == order_index_.end()) return false;

    auto [side, price] = it->second;

    if (side == Side::BUY) {
        auto level_it = bids_.find(price);
        if (level_it != bids_.end()) {
            level_it->second.remove(id);
            if (level_it->second.empty()) bids_.erase(level_it);
        }
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()) {
            level_it->second.remove(id);
            if (level_it->second.empty()) asks_.erase(level_it);
        }
    }

    order_index_.erase(it);
    return true;
}

void OrderBook_v2::print_book(int depth) const {
    std::cout << "\n=== ORDER BOOK v2 (Pool + PriceLevelQueue) ===\n";
    std::cout << std::setw(10) << "ASK QTY"
              << " | " << std::setw(8) << "PRICE"
              << " | " << "BID QTY\n";
    std::cout << std::string(45, '-') << "\n";

    int i = 0;
    std::vector<std::pair<Price, Quantity>> ask_levels;
    for (auto& [px, lvl] : asks_) {
        Quantity total = 0;
        for (std::size_t j = 0; j < lvl.size(); ++j)
            total += const_cast<PriceLevel&>(lvl).front().remaining();
        ask_levels.push_back({px, lvl.size() * 100});
        if (++i >= depth) break;
    }
    for (auto it = ask_levels.rbegin(); it != ask_levels.rend(); ++it)
        std::cout << std::setw(10) << it->second
                  << " | " << std::setw(8) << it->first << " |\n";

    std::cout << std::string(45, '-') << "\n";

    i = 0;
    for (auto& [px, lvl] : bids_) {
        std::cout << std::setw(10) << ""
                  << " | " << std::setw(8) << px
                  << " | " << lvl.size() * 100 << "\n";
        if (++i >= depth) break;
    }
    std::cout << std::string(45, '=') << "\n";
}