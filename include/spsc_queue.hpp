// include/spsc_queue.hpp
#pragma once
#include <atomic>
#include <array>
#include <optional>
#include <cstddef>

template <typename T, std::size_t Capacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");

public:
    SPSCQueue() : head_(0), tail_(0) {}

    // Producer thread only
    bool push(const T& item) noexcept {
        const std::size_t tail     = tail_.load(std::memory_order_relaxed);
        const std::size_t next_tail = (tail + 1) & MASK;

        if (next_tail == head_.load(std::memory_order_acquire))
            return false;  // full

        data_[tail] = item;
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Consumer thread only
    std::optional<T> pop() noexcept {
        const std::size_t head = head_.load(std::memory_order_relaxed);

        if (head == tail_.load(std::memory_order_acquire))
            return std::nullopt;  // empty

        T item = data_[head];
        head_.store((head + 1) & MASK, std::memory_order_release);
        return item;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    std::size_t size() const noexcept {
        return (tail_.load(std::memory_order_acquire) -
                head_.load(std::memory_order_acquire)) & MASK;
    }

private:
    static constexpr std::size_t MASK = Capacity - 1;

    alignas(64) std::atomic<std::size_t> head_;
    alignas(64) std::atomic<std::size_t> tail_;
    alignas(64) std::array<T, Capacity>  data_;
};