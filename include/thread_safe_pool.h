#pragma once

#include <mutex>
#include "memory_pool.h"

/**
 * ThreadSafePool<T, BlockSize, PoolSize>
 *
 * Wraps MemoryPool with a std::mutex to make all operations thread-safe.
 *
 * Design decision: we use a single mutex over the entire pool rather than
 * per-block locking. This is simpler and correct. Per-block locking would
 * be faster under high contention but adds significant complexity — not
 * worth it for typical embedded use where pool operations are microseconds.
 *
 * This is a deliberate trade-off: correctness and simplicity over
 * maximum throughput.
 */
template <typename T, std::size_t BlockSize, std::size_t PoolSize>
class ThreadSafePool {
public:
    ThreadSafePool() = default;
    ~ThreadSafePool() = default;

    // Non-copyable for same reason as MemoryPool
    ThreadSafePool(const ThreadSafePool&) = delete;
    ThreadSafePool& operator=(const ThreadSafePool&) = delete;

    /**
     * Thread-safe allocate.
     * lock_guard acquires the mutex on construction,
     * releases it automatically when it goes out of scope.
     * This is RAII — no manual lock/unlock, no risk of forgetting to unlock.
     */
    void* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.allocate();
    }

    void deallocate(void* ptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.deallocate(ptr);
    }

    template <typename... Args>
    T* construct(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.construct(std::forward<Args>(args)...);
    }

    void destroy(T* obj) {
        std::lock_guard<std::mutex> lock(mutex_);
        pool_.destroy(obj);
    }

    // Diagnostics — note: value may be stale by the time caller reads it
    // in a multithreaded context. Fine for logging, not for decisions.
    std::size_t available() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return pool_.available();
    }

    std::size_t capacity() const { return pool_.capacity(); }

private:
    MemoryPool<T, BlockSize, PoolSize> pool_;
    mutable std::mutex mutex_;
    // mutable = allows locking even in const methods like available()
};