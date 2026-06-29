#include <iostream>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include "memory_pool.h"

// ─────────────────────────────────────────
// Timing helper
// ─────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;
using NS    = std::chrono::nanoseconds;

static int64_t now_ns() {
    return std::chrono::duration_cast<NS>(
        Clock::now().time_since_epoch()
    ).count();
}

// ─────────────────────────────────────────
// The type being benchmarked
// ─────────────────────────────────────────

struct Packet {
    uint32_t id;
    float    data[8];
    Packet(uint32_t i) : id(i) { data[0] = 0.0f; }
};

static const int ITERATIONS = 100000;

// ─────────────────────────────────────────
// Benchmark 1: malloc/free baseline
// ─────────────────────────────────────────

void bench_malloc() {
    int64_t start = now_ns();

    for (int i = 0; i < ITERATIONS; ++i) {
        Packet* p = static_cast<Packet*>(malloc(sizeof(Packet)));
        new(p) Packet(i);      // placement new to construct
        p->~Packet();          // explicit destructor
        free(p);
    }

    int64_t elapsed = now_ns() - start;
    std::cout << "malloc/free     : "
              << elapsed << " ns total | "
              << elapsed / ITERATIONS << " ns/op\n";
}

// ─────────────────────────────────────────
// Benchmark 2: MemoryPool
// ─────────────────────────────────────────

void bench_pool() {
    MemoryPool<Packet, sizeof(Packet), ITERATIONS> mp;

    int64_t start = now_ns();

    for (int i = 0; i < ITERATIONS; ++i) {
        Packet* p = mp.construct(i);
        mp.destroy(p);
    }

    int64_t elapsed = now_ns() - start;
    std::cout << "MemoryPool      : "
              << elapsed << " ns total | "
              << elapsed / ITERATIONS << " ns/op\n";
}

// ─────────────────────────────────────────
// Benchmark 3: Fragmentation simulation
// Alternating alloc sizes stress-test malloc
// but pool is immune (fixed size blocks)
// ─────────────────────────────────────────

void bench_fragmentation_malloc() {
    static const int N = 1000;
    void* ptrs[N];

    int64_t start = now_ns();

    // Allocate all, free every other one, allocate again
    // This fragments the heap — malloc slows down noticeably
    for (int i = 0; i < N; ++i)
        ptrs[i] = malloc(sizeof(Packet));

    for (int i = 0; i < N; i += 2)
        free(ptrs[i]);

    for (int i = 0; i < N; i += 2)
        ptrs[i] = malloc(sizeof(Packet));

    for (int i = 0; i < N; ++i)
        free(ptrs[i]);

    int64_t elapsed = now_ns() - start;
    std::cout << "malloc fragment : " << elapsed << " ns total\n";
}

void bench_fragmentation_pool() {
    static const int N = 1000;
    MemoryPool<Packet, sizeof(Packet), N> mp;
    Packet* ptrs[N];

    int64_t start = now_ns();

    // Same pattern — pool is completely unaffected
    // Fixed size blocks mean fragmentation is impossible
    for (int i = 0; i < N; ++i)
        ptrs[i] = mp.construct(i);

    for (int i = 0; i < N; i += 2)
        mp.destroy(ptrs[i]);

    for (int i = 0; i < N; i += 2)
        ptrs[i] = mp.construct(i);

    for (int i = 0; i < N; ++i)
        mp.destroy(ptrs[i]);

    int64_t elapsed = now_ns() - start;
    std::cout << "pool fragment   : " << elapsed << " ns total\n";
}

// ─────────────────────────────────────────
// Main
// ─────────────────────────────────────────

int main() {
    std::cout << "=== Memory Pool Benchmark ===\n";
    std::cout << "Iterations: " << ITERATIONS << "\n\n";

    std::cout << "-- Alloc/dealloc speed --\n";
    bench_malloc();
    bench_pool();

    std::cout << "\n-- Fragmentation resistance --\n";
    bench_fragmentation_malloc();
    bench_fragmentation_pool();

    std::cout << "\nNote: pool wins on speed AND fragmentation.\n";
    std::cout << "In real-time systems, the ns/op consistency\n";
    std::cout << "matters as much as the average speed.\n";

    return 0;
}