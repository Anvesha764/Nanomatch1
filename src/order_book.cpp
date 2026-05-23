#include "order_book_v1.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

// ---------------------------------------------------------------------------
// match  —  price-time priority
//
// BUY  incoming: sweeps asks_ from lowest ask upward  while ask  <= buy price
// SELL incoming: sweeps bids_ from highest bid down   while bid  >= sell price
//
// std::map guarantees begin() is always the best price on each side,
// so no sorting is needed — just keep looking at begin() until the
// crossing condition breaks or the book empties.
// ---------------------------------------------------------------------------
std::vector<Trade> OrderBook::match(Order& incoming) {
    std::vector<Trade> trades;

    auto do_match = [&](auto& book) {
        while (incoming.remaining() > 0 && !book.empty()) {
            auto  it          = book.begin();
            Price level_price = it->first;

            // Check whether the incoming order crosses this level
            bool crosses = (incoming.side == Side::BUY)
                ? incoming.price >= level_price   // buy price must reach the ask
                : level_price    >= incoming.price; // bid must reach the sell price
            if (!crosses) break;

            PriceLevel& level = it->second;

            // Fill as many resting orders at this level as possible (FIFO)
            while (incoming.remaining() > 0 && !level.empty()) {
                Order&   resting  = level.front();
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
                    level.erase(level.begin()); // pop the fully-filled resting order
                }
            }

            if (level.empty()) book.erase(it); // remove exhausted price level
        }
    };

    if (incoming.side == Side::BUY) do_match(asks_);
    else                            do_match(bids_);

    return trades;
}

// ---------------------------------------------------------------------------
// add_order
// ---------------------------------------------------------------------------
std::vector<Trade> OrderBook::add_order(Order order) {
    auto trades = match(order);

    // Whatever quantity wasn't matched rests in the book (LIMIT only)
    if (!order.is_filled()) {
        if (order.type == OrderType::LIMIT) {
            if (order.side == Side::BUY) bids_[order.price].push_back(order);
            else                         asks_[order.price].push_back(order);
            order_index_[order.id] = {order.side, order.price};
        }
        // MARKET orders that couldn't fully fill are simply cancelled (no resting)
    }

    return trades;
}

// ---------------------------------------------------------------------------
// cancel_order
// ---------------------------------------------------------------------------
bool OrderBook::cancel_order(OrderId id) {
    auto it = order_index_.find(id);
    if (it == order_index_.end()) return false; // not found

    auto [side, price] = it->second;

    auto remove_from = [&](auto& book) {
        auto lvl_it = book.find(price);
        if (lvl_it == book.end()) return;
        auto& level = lvl_it->second;
        level.erase(
            std::remove_if(level.begin(), level.end(),
                [id](const Order& o){ return o.id == id; }),
            level.end());
        if (level.empty()) book.erase(lvl_it);
    };

    if (side == Side::BUY) remove_from(bids_);
    else                   remove_from(asks_);

    order_index_.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// print_book
// ---------------------------------------------------------------------------
void OrderBook::print_book(int depth) const {
    std::cout << "\n=== ORDER BOOK (v1 STL) ===\n";
    std::cout << std::setw(10) << "ASK QTY"
              << " | " << std::setw(8) << "PRICE"
              << " | BID QTY\n";
    std::cout << std::string(35, '-') << "\n";

    // Asks: ascending in the map, print highest-first so the spread is visible
    int i = 0;
    std::vector<std::pair<Price, Quantity>> ask_snap;
    for (auto& [px, lvl] : asks_) {
        Quantity total = 0;
        for (auto& o : lvl) total += o.remaining();
        ask_snap.push_back({px, total});
        if (++i >= depth) break;
    }
    for (auto it = ask_snap.rbegin(); it != ask_snap.rend(); ++it)
        std::cout << std::setw(10) << it->second
                  << " | " << std::setw(8) << it->first << " |\n";

    std::cout << std::string(35, '-') << "\n";

    // Bids: map is already descending (std::greater), so begin() = best bid
    i = 0;
    for (auto& [px, lvl] : bids_) {
        Quantity total = 0;
        for (auto& o : lvl) total += o.remaining();
        std::cout << std::setw(10) << ""
                  << " | " << std::setw(8) << px
                  << " | " << total << "\n";
        if (++i >= depth) break;
    }
    std::cout << std::string(35, '=') << "\n";
}