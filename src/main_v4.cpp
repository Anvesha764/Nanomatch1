#include "order_book_v2.hpp"
#include "itch_parser.hpp"
#include "trade_logger.hpp"
#include <iostream>
#include <chrono>
#include <unordered_map>

int main(int argc, char* argv[]) {
    std::string itch_path = (argc >= 2) ? argv[1] : "test_data.itch";
    std::string filter = (argc >= 3) ? argv[2] : "AAPL";

    std::cout << "Starting...\n"; std::cout.flush();

    TradeLogger logger("trades.csv");
    logger.start();

    std::cout << "Logger started\n"; std::cout.flush();

    OrderBook_v2 book;
    std::unordered_map<OrderId, bool> active_orders;
    uint64_t add_count = 0, trade_count = 0;

    ITCHCallbacks callbacks {
        .on_add = [&](const AddOrderEvent& e) {
            Order o(e.order_id, e.price, e.quantity, e.side, e.timestamp);
            auto trades = book.add_order(o);
            for (auto& t : trades) {
                logger.log_trade({t.buy_order_id, t.sell_order_id, t.price, t.quantity, e.timestamp});
            }
            active_orders[e.order_id] = true;
            trade_count += trades.size();
            ++add_count;
        },
        .on_cancel = [&](const CancelOrderEvent& e) {
            if (active_orders.count(e.order_id)) {
                book.cancel_order(e.order_id);
                active_orders.erase(e.order_id);
            }
        },
        .on_execute = [](const ExecuteOrderEvent&) {}
    };

    std::cout << "Parsing: " << itch_path << "\n";
    std::cout << "Filter:  " << filter << "\n\n";
    std::cout.flush();

    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t total = parse_itch_file(itch_path, callbacks, filter);
    auto t1 = std::chrono::high_resolution_clock::now();

    logger.stop();

    double sec = std::chrono::duration<double>(t1 - t0).count();

    std::cout << "=== PHASE 4 RESULTS ===\n"
              << "Total messages : " << total<< "\n"
              << "Add orders     : " << add_count<< "\n"
              << "Trades matched : " << trade_count<< "\n"
              << "Trades logged  : " << logger.logged_count()<< "\n"
              << "Trades dropped : " << logger.dropped_count()<< "\n"
              << "Time           : " << sec<< "s\n"
              << "Throughput     : "
              << (uint64_t)(total / sec) << " msg/sec\n";

    book.print_book(5);
    return 0;
}
