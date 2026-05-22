#pragma once
#include "types.hpp"
#include "order.hpp"
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>


inline uint16_t bswap16(uint16_t v) { return (v >> 8) | (v << 8); }
inline uint32_t bswap32(uint32_t v) {
    return ((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8)  | ((v & 0x000000FF) << 24);
}
inline uint64_t bswap64(uint64_t v) {
    return ((v & 0xFF00000000000000ULL) >> 56) | ((v & 0x00FF000000000000ULL) >> 40) | ((v & 0x0000FF0000000000ULL) >> 24) | ((v & 0x000000FF00000000ULL) >>  8) |
           ((v & 0x00000000FF000000ULL) <<  8) |((v & 0x0000000000FF0000ULL) << 24) | ((v & 0x000000000000FF00ULL) << 40) | ((v & 0x00000000000000FFULL) << 56);
}

inline uint64_t read_ts6(const uint8_t* b) {
    return ((uint64_t)b[0] << 40) | ((uint64_t)b[1] << 32) | ((uint64_t)b[2] << 24) | ((uint64_t)b[3] << 16) | ((uint64_t)b[4] <<  8) |  (uint64_t)b[5];
}

#pragma pack(push, 1)

struct itch_add_order {
    char     msg_type;
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint8_t  timestamp[6];
    uint64_t order_ref_num;
    char     buy_sell;
    uint32_t shares;
    char     stock[8];
    uint32_t price;
};

struct itch_delete_order {
    char     msg_type;
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint8_t  timestamp[6];
    uint64_t order_ref_num;
};

struct itch_execute_order {
    char     msg_type;
    uint16_t stock_locate;
    uint16_t tracking_num;
    uint8_t  timestamp[6];
    uint64_t order_ref_num;
    uint32_t executed_shares;
    uint64_t match_num;
};

#pragma pack(pop)

struct AddOrderEvent {
    OrderId   order_id;
    Price     price;
    Quantity  quantity;
    Side      side;
    Timestamp timestamp;
    char      stock[9];
};

struct CancelOrderEvent {
    OrderId   order_id;
    Timestamp timestamp;
};

struct ExecuteOrderEvent {
    OrderId   order_id;
    Quantity  executed_qty;
    Timestamp timestamp;
};

struct ITCHCallbacks {
    std::function<void(const AddOrderEvent&)> on_add;
    std::function<void(const CancelOrderEvent&)> on_cancel;
    std::function<void(const ExecuteOrderEvent&)> on_execute;
};

uint64_t parse_itch_file(const std::string& path,
                         const ITCHCallbacks& cb,
                         const std::string& filter_stock = "");
