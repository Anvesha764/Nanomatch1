#pragma once
#include "price_level.hpp"
#include <array>
#include <cstddef>
#include <cassert>

// ---------------------------------------------------------------------------
// PriceLevelArray<Capacity, Descending>
//
// Replaces std::map<Price, PriceLevelQueue> on the hot path.
//
// Layout: flat sorted array of {price, queue} slots stored inline — no heap,
// no pointer chasing.  Best price is always slot [0].
//
//   Descending = true  →  bids  (highest price first)
//   Descending = false →  asks  (lowest  price first)
//
// Complexity:
//   find / best  : O(log N) binary search
//   insert/erase : O(N) shift  — N is number of *active price levels*, which
//                  in practice stays small (≤ 256 for a single-symbol book).
//                  The shift is a contiguous memmove — extremely cache-friendly
//                  vs. the pointer-chasing of std::map nodes.
// ---------------------------------------------------------------------------

template <std::size_t MaxLevels = 256, bool Descending = false>
class PriceLevelArray {
public:
    using Queue = PriceLevelQueue<64>;

    struct Slot {
        Price price = 0;
        Queue queue;
    };

    // ---- capacity / size ------------------------------------------------

    bool        empty() const noexcept { return size_ == 0; }
    std::size_t size()  const noexcept { return size_; }

    // ---- best-price access (index 0) ------------------------------------

    Slot&       front()       noexcept { assert(!empty()); return slots_[0]; }
    const Slot& front() const noexcept { assert(!empty()); return slots_[0]; }

    // ---- find (returns pointer, nullptr if not present) -----------------

    Queue* find(Price px) noexcept {
        int idx = lower_bound(px);
        if (idx < (int)size_ && slots_[idx].price == px)
            return &slots_[idx].queue;
        return nullptr;
    }

    const Queue* find(Price px) const noexcept {
        int idx = lower_bound(px);
        if (idx < (int)size_ && slots_[idx].price == px)
            return &slots_[idx].queue;
        return nullptr;
    }

    // ---- find_or_insert -------------------------------------------------
    // Returns reference to the queue at `px`, inserting an empty one if absent.

    Queue& operator[](Price px) {
        int idx = lower_bound(px);
        if (idx < (int)size_ && slots_[idx].price == px)
            return slots_[idx].queue;

        // insert at idx
        assert(size_ < MaxLevels && "PriceLevelArray overflow");
        // shift right
        for (int i = (int)size_; i > idx; --i)
            slots_[i] = std::move(slots_[i - 1]);
        slots_[idx].price = px;
        slots_[idx].queue = Queue{};
        ++size_;
        return slots_[idx].queue;
    }

    // ---- erase by price -------------------------------------------------

    bool erase(Price px) noexcept {
        int idx = lower_bound(px);
        if (idx >= (int)size_ || slots_[idx].price != px)
            return false;
        // shift left
        for (int i = idx; i < (int)size_ - 1; ++i)
            slots_[i] = std::move(slots_[i + 1]);
        --size_;
        return true;
    }

    // ---- iteration (forward only, index-based) -------------------------

    Slot*       begin()       noexcept { return slots_.data(); }
    Slot*       end()         noexcept { return slots_.data() + size_; }
    const Slot* begin() const noexcept { return slots_.data(); }
    const Slot* end()   const noexcept { return slots_.data() + size_; }

private:
    // Binary search: returns insertion index such that the slot array stays
    // sorted.  For Descending=true the array is kept high→low; for
    // Descending=false it is kept low→high.
    int lower_bound(Price px) const noexcept {
        int lo = 0, hi = (int)size_;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            bool before = Descending ? (slots_[mid].price > px)
                                     : (slots_[mid].price < px);
            if (before) lo = mid + 1;
            else        hi = mid;
        }
        return lo;
    }

    std::array<Slot, MaxLevels> slots_;
    std::size_t size_ = 0;
};
