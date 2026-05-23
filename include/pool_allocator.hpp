#pragma once
#include <cstddef>
#include <cassert>
#include <new>

template <typename T, std::size_t PoolSize = 4096>
class PoolAllocator {
public:
    using value_type = T;

    template <typename U>
    struct rebind { using other = PoolAllocator<U, PoolSize>; };

    PoolAllocator() {
        for (std::size_t i = 0; i < PoolSize - 1; ++i)
            blocks_[i].next = &blocks_[i + 1];
        blocks_[PoolSize - 1].next = nullptr;
        free_head_ = &blocks_[0];
    }

    PoolAllocator(const PoolAllocator&) = delete;
    PoolAllocator& operator=(const PoolAllocator&) = delete;

    T* allocate(std::size_t n) {
        if (n != 1 || free_head_ == nullptr)
            return static_cast<T*>(::operator new(n * sizeof(T)));
        Block* block = free_head_;
        free_head_ = block->next;
        ++alloc_count_;
        return reinterpret_cast<T*>(block->storage);
    }

    void deallocate(T* ptr, std::size_t n) noexcept {
    if (n != 1) { ::operator delete(ptr); return; }
    // Check if ptr is inside our pool slab before returning to free list
    Block* block = reinterpret_cast<Block*>(ptr);
    if (block >= &blocks_[0] && block < &blocks_[PoolSize]) {
        block->next = free_head_;
        free_head_ = block;
        --alloc_count_;
    } else {
        ::operator delete(ptr);  // was an overflow allocation
    }
}

    std::size_t allocated_count() const { return alloc_count_; }

private:
    union Block {
        alignas(T) std::byte storage[sizeof(T)];
        Block* next;
    };

    Block  blocks_[PoolSize];
    Block* free_head_ = nullptr;
    std::size_t alloc_count_ = 0;
};

template <typename T, typename U, std::size_t N>
bool operator==(const PoolAllocator<T,N>&, const PoolAllocator<U,N>&) { return true; }
template <typename T, typename U, std::size_t N>
bool operator!=(const PoolAllocator<T,N>&, const PoolAllocator<U,N>&) { return false; }
