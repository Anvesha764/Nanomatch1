// include/trade_logger.hpp
#pragma once
#include "order.hpp"
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <string>

struct TradeEvent {
    OrderId   buy_order_id;
    OrderId   sell_order_id;
    Price     price;
    Quantity  quantity;
    Timestamp timestamp;
};

class TradeLogger {
public:
    explicit TradeLogger(const std::string& log_path) : logged_(0), dropped_(0) {
        log_file_.open(log_path, std::ios::out | std::ios::trunc);
        if (!log_file_.is_open())
            throw std::runtime_error("Cannot open log: " + log_path);
        log_file_ << "timestamp_ns,buy_order_id,sell_order_id,price,quantity\n";
    }

    void start() {}
    void stop()  { log_file_.flush(); }

    void log_trade(const TradeEvent& evt) noexcept {
        char buf[256];
        int len = std::snprintf(buf, sizeof(buf),
            "%llu,%llu,%llu,%lld,%u\n",
            (unsigned long long)evt.timestamp,
            (unsigned long long)evt.buy_order_id,
            (unsigned long long)evt.sell_order_id,
            (long long)evt.price,
            (unsigned)evt.quantity);
        log_file_.write(buf, len);
        ++logged_;
        if (logged_ % 10000 == 0) log_file_.flush();
    }

    uint64_t logged_count()  const { return logged_;  }
    uint64_t dropped_count() const { return dropped_; }

private:
    std::ofstream log_file_;
    uint64_t      logged_;
    uint64_t      dropped_;
};