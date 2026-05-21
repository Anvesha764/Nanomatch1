#include "itch_parser.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>

uint64_t parse_itch_file(const std::string& path,
                         const ITCHCallbacks& cb,
                         const std::string& filter_stock) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) {
        std::cerr << "Cannot open: " << path << "\n";
        return 0;
    }

    uint64_t msg_count = 0;
    uint8_t  len_buf[2];
    uint8_t  msg_buf[1024];

    while (fread(len_buf, 1, 2, f) == 2) {
        uint16_t msg_len = bswap16(
            *reinterpret_cast<uint16_t*>(len_buf));

        if (msg_len == 0 || msg_len > sizeof(msg_buf)) break;
        if (fread(msg_buf, 1, msg_len, f) != msg_len) break;

        char msg_type = static_cast<char>(msg_buf[0]);

        switch (msg_type) {

        case 'A': {
            if (msg_len < sizeof(itch_add_order)) break;
            auto* m = reinterpret_cast<const itch_add_order*>(msg_buf);

            if (!filter_stock.empty()) {
                char stock[9] = {};
                std::memcpy(stock, m->stock, 8);
                for (int i = 7; i >= 0 && stock[i] == ' '; --i)
                    stock[i] = '\0';
                if (filter_stock != stock) break;
            }

            AddOrderEvent evt;
            evt.order_id  = bswap64(m->order_ref_num);
            evt.price     = static_cast<Price>(bswap32(m->price));
            evt.quantity  = static_cast<Quantity>(bswap32(m->shares));
            evt.side      = (m->buy_sell == 'B') ? Side::BUY : Side::SELL;
            evt.timestamp = read_ts6(m->timestamp);
            std::memcpy(evt.stock, m->stock, 8);
            evt.stock[8]  = '\0';

            if (cb.on_add) cb.on_add(evt);
            break;
        }

        case 'D': {
            if (msg_len < sizeof(itch_delete_order)) break;
            auto* m = reinterpret_cast<const itch_delete_order*>(msg_buf);

            CancelOrderEvent evt;
            evt.order_id  = bswap64(m->order_ref_num);
            evt.timestamp = read_ts6(m->timestamp);

            if (cb.on_cancel) cb.on_cancel(evt);
            break;
        }

        case 'E': {
            if (msg_len < sizeof(itch_execute_order)) break;
            auto* m = reinterpret_cast<const itch_execute_order*>(msg_buf);

            ExecuteOrderEvent evt;
            evt.order_id     = bswap64(m->order_ref_num);
            evt.executed_qty = bswap32(m->executed_shares);
            evt.timestamp    = read_ts6(m->timestamp);

            if (cb.on_execute) cb.on_execute(evt);
            break;
        }

        default: break;
        }

        ++msg_count;
    }

    fclose(f);
    return msg_count;
}