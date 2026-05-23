#pragma once
#include "types.hpp"
#include <array>
#include <cstddef>
#include <cassert>
#include <optional>

// ---------------------------------------------------------------------------
// FlatHashMap<V, Capacity>
//
// Replaces std::unordered_map<OrderId, V> on the hot path.
//
// Design:
//   - Open addressing with linear probing.
//   - Capacity must be a power of 2.
//   - Keys are OrderId (uint64_t).  Tombstones are used for erase.
//   - Zero heap allocation — all storage is inline in the struct.
//   - Load factor kept ≤ 0.75 by the caller choosing Capacity appropriately.
//     For 500k orders, Capacity=1<<20 (1M slots, 24 MB) is appropriate.
//     For tests / small workloads, 1<<16 (64k slots, ~1.5 MB) is fine.
// ---------------------------------------------------------------------------

template <typename V, std::size_t Capacity = (1 << 16)>
class FlatHashMap {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    static constexpr OrderId EMPTY     = 0;
    static constexpr OrderId TOMBSTONE = ~OrderId(0);

    struct Slot {
        OrderId key   = EMPTY;
        V       value = {};
    };

public:
    FlatHashMap() { slots_.fill(Slot{}); }

    // ---- insert / upsert ------------------------------------------------

    void insert(OrderId key, const V& val) {
        assert(key != EMPTY && key != TOMBSTONE);
        std::size_t idx = probe_start(key);
        for (;;) {
            auto& s = slots_[idx];
            if (s.key == EMPTY || s.key == TOMBSTONE || s.key == key) {
                s.key   = key;
                s.value = val;
                ++size_;
                return;
            }
            idx = (idx + 1) & MASK;
        }
    }

    // ---- find (returns pointer, nullptr if absent) ----------------------

    V* find(OrderId key) noexcept {
        std::size_t idx = probe_start(key);
        for (;;) {
            auto& s = slots_[idx];
            if (s.key == EMPTY)     return nullptr;
            if (s.key == key)       return &s.value;
            idx = (idx + 1) & MASK;
        }
    }

    const V* find(OrderId key) const noexcept {
        std::size_t idx = probe_start(key);
        for (;;) {
            const auto& s = slots_[idx];
            if (s.key == EMPTY)  return nullptr;
            if (s.key == key)    return &s.value;
            idx = (idx + 1) & MASK;
        }
    }

    // ---- erase ----------------------------------------------------------

    bool erase(OrderId key) noexcept {
        std::size_t idx = probe_start(key);
        for (;;) {
            auto& s = slots_[idx];
            if (s.key == EMPTY)     return false;
            if (s.key == key) {
                s.key = TOMBSTONE;
                --size_;
                return true;
            }
            idx = (idx + 1) & MASK;
        }
    }

    std::size_t size()  const noexcept { return size_; }
    bool        empty() const noexcept { return size_ == 0; }

private:
    static constexpr std::size_t MASK = Capacity - 1;

    // Fibonacci hashing for good distribution of sequential OrderIds
    std::size_t probe_start(OrderId key) const noexcept {
        return (key * 11400714819323198485ULL) >> (64 - __builtin_ctzll(Capacity));
    }

    std::array<Slot, Capacity> slots_;
    std::size_t size_ = 0;
};
