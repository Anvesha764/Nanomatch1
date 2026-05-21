#include "order_book_v2.hpp"
#include "itch_parser.hpp"
#include <iostream>
#include <chrono>
#include <unordered_map>

int main(int argc, char* argv[]) {
    std::string itch_path = (argc >= 2) ? argv[1] : "test_data.itch";
    std::string filter = (argc >= 3) ? argv[2] : "AAPL";

    OrderBook_v2 book;
    std::unordered_map<OrderId, bool> active_orders;

    uint64_t add_count = 0;
    uint64_t cancel_count  = 0;
    uint64_t execute_count = 0;
    uint64_t trade_count = 0;

    ITCHCallbacks callbacks {
        .on_add = [&](const AddOrderEvent& e) {
            Order o(e.order_id, e.price, e.quantity, e.side, e.timestamp);
            auto trades = book.add_order(o);
            active_orders[e.order_id] = true;
            trade_count += trades.size();
            ++add_count;
        },
        .on_cancel = [&](const CancelOrderEvent& e) {
            if (active_orders.count(e.order_id)) { 
                book.cancel_order(e.order_id);
                active_orders.erase(e.order_id);
                ++cancel_count;
            }
        },
        .on_execute = [&](const ExecuteOrderEvent&) {
            ++execute_count;
        }
    };

    std::cout << "Parsing: " << itch_path << "\n";
    std::cout << "Filter:  " << filter << "\n\n";

    auto t0 = std::chrono::high_resolution_clock::now();
    uint64_t total = parse_itch_file(itch_path, callbacks, filter);
    auto t1 = std::chrono::high_resolution_clock::now();

    double sec = std::chrono::duration<double>(t1 - t0).count();

    std::cout << "=== INGESTION COMPLETE ===\n"
              << "Total messages : " << total         << "\n"
              << "Add orders     : " << add_count     << "\n"
              << "Cancels        : " << cancel_count  << "\n"
              << "Executions     : " << execute_count << "\n"
              << "Trades matched : " << trade_count   << "\n"
              << "Time           : " << sec           << "s\n"
              << "Throughput     : "
              << (uint64_t)(total / sec) << " msg/sec\n";

    book.print_book(10);
    return 0;
}