#pragma once

#include <cstddef>
#include <cassert>
#include <stdexcept>

/**
 * MemoryPool<T, BlockSize, PoolSize>
 *
 * A fixed-size block memory allocator designed for embedded/real-time systems
 * where malloc() is unacceptable due to:
 *   1. Non-deterministic allocation time (heap fragmentation-dependent)
 *   2. Fragmentation on devices with limited RAM (e.g. 256KB)
 *
 * This pool solves both: O(1) allocation/deallocation, zero fragmentation.
 *
 * Template parameters:
 *   T         - Type of objects this pool manages
 *   BlockSize - Size of each memory block in bytes (must be >= sizeof(T))
 *   PoolSize  - Total number of blocks pre-allocated at startup
 */
template <typename T, std::size_t BlockSize, std::size_t PoolSize>
class MemoryPool {
public:
    // --- Static assertions: catch bad template args at compile time ---
    static_assert(BlockSize >= sizeof(T),
        "BlockSize must be at least sizeof(T)");
    static_assert(BlockSize >= sizeof(void*),
        "BlockSize must be large enough to store a free-list pointer");
    static_assert(PoolSize > 0,
        "PoolSize must be greater than 0");

    MemoryPool();
    ~MemoryPool() = default;

    // Non-copyable, non-movable — pool owns its memory
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;
    MemoryPool(MemoryPool&&) = delete;
    MemoryPool& operator=(MemoryPool&&) = delete;

    /**
     * Allocate one block from the pool.
     * Returns a pointer to raw memory (not constructed).
     * Caller must use placement new to construct the object.
     * O(1) — just pops the head of the free list.
     * Returns nullptr if pool is exhausted.
     */
    void* allocate();

    /**
     * Return a block to the pool.
     * Caller must have already called the destructor manually.
     * O(1) — just pushes to the head of the free list.
     */
    void deallocate(void* ptr);

    // --- Convenience wrappers: construct/destroy in one call ---
    template <typename... Args>
    T* construct(Args&&... args);

    void destroy(T* obj);

    // --- Diagnostics ---
    std::size_t available() const { return free_count_; }
    std::size_t capacity()  const { return PoolSize; }
    bool        full()      const { return free_count_ == 0; }
    bool        empty()     const { return free_count_ == PoolSize; }

private:
    /**
     * The free list trick: each free block stores a pointer to the next
     * free block IN THE SAME MEMORY. No extra allocation needed.
     * When a block is allocated, that space is used for the object instead.
     */
    union Block {
        Block* next;                   // used when block is FREE
        alignas(T) char data[BlockSize]; // used when block is ALLOCATED
    };

    Block pool_[PoolSize];  // the entire pool — stack allocated at startup
    Block* free_list_;      // points to first free block
    std::size_t free_count_;
};


// ---- Implementation ----
// (In a header-only template, implementation goes here)

template <typename T, std::size_t BlockSize, std::size_t PoolSize>
MemoryPool<T, BlockSize, PoolSize>::MemoryPool()
    : free_list_(nullptr), free_count_(PoolSize)
{
    // Wire all blocks together into a linked list at startup.
    // pool_[0] -> pool_[1] -> pool_[2] -> ... -> pool_[N-1] -> nullptr
    for (std::size_t i = 0; i < PoolSize - 1; ++i) {
        pool_[i].next = &pool_[i + 1];
    }
    pool_[PoolSize - 1].next = nullptr;
    free_list_ = &pool_[0];
}

template <typename T, std::size_t BlockSize, std::size_t PoolSize>
void* MemoryPool<T, BlockSize, PoolSize>::allocate()
{
    if (free_list_ == nullptr) {
        return nullptr;  // pool exhausted — caller decides what to do
    }

    // Pop head of free list
    Block* block = free_list_;
    free_list_ = block->next;
    --free_count_;

    return static_cast<void*>(block->data);
}

template <typename T, std::size_t BlockSize, std::size_t PoolSize>
void MemoryPool<T, BlockSize, PoolSize>::deallocate(void* ptr)
{
    if (ptr == nullptr) return;

    // Push back onto free list — reinterpret the memory as a Block
    Block* block = reinterpret_cast<Block*>(ptr);
    block->next = free_list_;
    free_list_ = block;
    ++free_count_;
}

template <typename T, std::size_t BlockSize, std::size_t PoolSize>
template <typename... Args>
T* MemoryPool<T, BlockSize, PoolSize>::construct(Args&&... args)
{
    void* mem = allocate();
    if (mem == nullptr) {
        throw std::bad_alloc();
    }
    // Placement new: construct T in the pre-allocated memory
    return new(mem) T(std::forward<Args>(args)...);
}

template <typename T, std::size_t BlockSize, std::size_t PoolSize>
void MemoryPool<T, BlockSize, PoolSize>::destroy(T* obj)
{
    if (obj == nullptr) return;
    obj->~T();           // explicitly call destructor
    deallocate(obj);     // return memory to pool
}