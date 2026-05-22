#pragma once
#include "order.hpp"
#include "spsc_queue.hpp"
#include <fstream>
#include <thread>
#include <atomic>
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
    static constexpr std::size_t QUEUE_CAPACITY = 1 << 17; // 131072

    explicit TradeLogger(const std::string& log_path)
        : logged_(0), dropped_(0), running_(false)
    {
        log_file_.open(log_path, std::ios::out | std::ios::trunc);
        if (!log_file_.is_open())
            throw std::runtime_error("Cannot open log: " + log_path);
        log_file_ << "timestamp_ns,buy_order_id,sell_order_id,price,quantity\n";
    }

    ~TradeLogger() { stop(); }

    void start() {
        running_.store(true, std::memory_order_release);
        logger_thread_ = std::thread([this]() {
            while (running_.load(std::memory_order_acquire) || !queue_.empty()) {
                auto item = queue_.pop();
                if (item) {
                    write_trade(*item);
                }
            }
            log_file_.flush();
        });
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        if (logger_thread_.joinable())
            logger_thread_.join();
    }

    // Called from matching thread (producer)
    void log_trade(const TradeEvent& evt) noexcept {
        if (!queue_.push(evt))
            ++dropped_;
    }

    uint64_t logged_count()  const { return logged_;  }
    uint64_t dropped_count() const { return dropped_; }

private:
    void write_trade(const TradeEvent& evt) {
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

    SPSCQueue<TradeEvent, QUEUE_CAPACITY> queue_;
    std::ofstream  log_file_;
    std::thread    logger_thread_;
    std::atomic<bool> running_;
    uint64_t logged_;
    uint64_t dropped_;
};
