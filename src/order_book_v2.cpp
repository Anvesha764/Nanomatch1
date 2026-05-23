#include "order_book_v2.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

// ---------------------------------------------------------------------------
// add_order
// ---------------------------------------------------------------------------

std::vector<Trade> OrderBook_v2::add_order(Order order) {
    auto trades = match(order);

    if (!order.is_filled()) {
        if (order.side == Side::BUY) {
            bids_[order.price].push_back(order);
        } else {
            asks_[order.price].push_back(order);
        }
        order_index_.insert(order.id, {order.side, order.price});
    }
    return trades;
}

// ---------------------------------------------------------------------------
// match  (price-time priority)
//
// For a BUY  incoming order: sweep asks_ from slot 0 (lowest ask) upward
//            while ask_price <= incoming_price.
// For a SELL incoming order: sweep bids_ from slot 0 (highest bid) upward
//            while bid_price >= incoming_price.
//
// "slot 0 is always best price" is guaranteed by PriceLevelArray's sorted
// layout, so we never need an iterator — just keep looking at [0] until it
// no longer crosses or the book empties.
// ---------------------------------------------------------------------------

std::vector<Trade> OrderBook_v2::match(Order& incoming) {
    std::vector<Trade> trades;

    // Generic lambda works for both BidBook and AskBook since they expose
    // the same interface.
    auto do_match = [&](auto& book, bool buy_side) {
        while (incoming.remaining() > 0 && !book.empty()) {
            auto& slot        = book.front();         // best price level
            Price level_price = slot.price;

            // Check crossing condition
            bool crosses = buy_side ? (incoming.price >= level_price)
                                    : (level_price    >= incoming.price);
            if (!crosses) break;

            PriceLevel& level = slot.queue;

            while (incoming.remaining() > 0 && !level.empty()) {
                Order& resting   = level.front();
                Quantity fill_qty = std::min(incoming.remaining(),
                                             resting.remaining());

                trades.push_back({
                    buy_side ? incoming.id : resting.id,
                    buy_side ? resting.id  : incoming.id,
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

            if (level.empty())
                book.erase(level_price);
        }
    };

    if (incoming.side == Side::BUY)
        do_match(asks_, /*buy_side=*/true);
    else
        do_match(bids_, /*buy_side=*/false);

    return trades;
}

// ---------------------------------------------------------------------------
// cancel_order
// ---------------------------------------------------------------------------

bool OrderBook_v2::cancel_order(OrderId id) {
    IndexValue* entry = order_index_.find(id);
    if (!entry) return false;

    auto [side, price] = *entry;

    if (side == Side::BUY) {
        PriceLevel* lvl = bids_.find(price);
        if (lvl) {
            lvl->remove(id);
            if (lvl->empty()) bids_.erase(price);
        }
    } else {
        PriceLevel* lvl = asks_.find(price);
        if (lvl) {
            lvl->remove(id);
            if (lvl->empty()) asks_.erase(price);
        }
    }

    order_index_.erase(id);
    return true;
}

// ---------------------------------------------------------------------------
// print_book
// ---------------------------------------------------------------------------

void OrderBook_v2::print_book(int depth) const {
    std::cout << "\n=== ORDER BOOK v2 (FlatArray + FlatHash) ===\n";
    std::cout << std::setw(10) << "ASK QTY"
              << " | " << std::setw(8) << "PRICE"
              << " | " << "BID QTY\n";
    std::cout << std::string(45, '-') << "\n";

    // Collect ask levels (ascending), then print them top-down (reversed)
    // Build a small stack so we can print highest ask first
    struct LevelSnap { Price px; Quantity qty; };
    LevelSnap ask_snap[256];
    int snap_sz = 0;
    for (auto& slot : asks_) {
        if (snap_sz >= depth) break;
        Quantity total = 0;
        PriceLevel tmp = slot.queue;          // copy to iterate
        while (!tmp.empty()) {
            total += tmp.front().remaining();
            tmp.pop_front();
        }
        ask_snap[snap_sz++] = {slot.price, total};
    }
    for (int i = snap_sz - 1; i >= 0; --i)
        std::cout << std::setw(10) << ask_snap[i].qty
                  << " | " << std::setw(8) << ask_snap[i].px << " |\n";

    std::cout << std::string(45, '-') << "\n";

    int n = 0;
    for (auto& slot : bids_) {
        if (n++ >= depth) break;
        Quantity total = 0;
        PriceLevel tmp = slot.queue;
        while (!tmp.empty()) {
            total += tmp.front().remaining();
            tmp.pop_front();
        }
        std::cout << std::setw(10) << ""
                  << " | " << std::setw(8) << slot.price
                  << " | " << total << "\n";
    }
    std::cout << std::string(45, '=') << "\n";
}
