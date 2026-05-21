#pragma once
#include "order.hpp"
#include <array>
#include <cstddef>
#include <cassert>

template <std::size_t Capacity = 64>
class PriceLevelQueue {
public:
    bool        empty() const { return size_ == 0; }
    bool        full()  const { return size_ == Capacity; }
    std::size_t size()  const { return size_; }

    void push_back(const Order& o) {
        assert(!full() && "PriceLevelQueue overflow");
        data_[(head_ + size_) % Capacity] = o;
        ++size_;
    }

    Order& front() {
        assert(!empty());
        return data_[head_];
    }

    void pop_front() {
        assert(!empty());
        head_ = (head_ + 1) % Capacity;
        --size_;
    }

    bool remove(OrderId id) {
        for (std::size_t i = 0; i < size_; ++i) {
            std::size_t idx = (head_ + i) % Capacity;
            if (data_[idx].id == id) {
                for (std::size_t j = i; j < size_ - 1; ++j)
                    data_[(head_ + j) % Capacity] =
                        data_[(head_ + j + 1) % Capacity];
                --size_;
                return true;
            }
        }
        return false;
    }

private:
    std::array<Order, Capacity> data_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};