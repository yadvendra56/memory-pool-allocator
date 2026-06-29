# Memory Pool Allocator

A fixed-size block memory allocator written in C++ for embedded and real-time systems, where `malloc` is unsuitable due to non-deterministic latency and heap fragmentation.

## The Problem

`malloc` and `free` have two critical issues in embedded systems:

- **Non-deterministic time** — execution time varies depending on heap state. A task with a 5ms deadline cannot afford malloc taking 2ms on a bad run.
- **Fragmentation** — repeated alloc/free of different sizes fragments heap memory over time. On a device with 256KB RAM, this is fatal.

## The Solution

A memory pool pre-allocates a fixed chunk of memory at startup and manages it internally with a free list. Every allocation and deallocation is O(1) — always, not on average.

## Features

- O(1) allocation and deallocation via free list
- Zero fragmentation — structurally impossible with fixed-size blocks
- Compile-time memory footprint — no runtime surprises
- Thread-safe wrapper using RAII mutex
- Full test suite — 9/9 unit tests
- Benchmarked against malloc

## Project Structure

## Build Instructions

**Requirements:** CMake 3.15+, C++17 compiler

```bash
git clone https://github.com/yourusername/memory-pool-allocator
cd memory-pool-allocator
mkdir build && cd build
cmake ..
make
```

## Run

```bash
# Demo — sensor pipeline simulation
./pool_demo

# Tests — 9/9 unit tests
./tests

# Benchmarks — pool vs malloc timing
./bench
```

## How It Works

Each free block stores a pointer to the next free block in the same memory the block would use for data. This means bookkeeping costs zero extra bytes.

## Usage

```cpp
// Create a pool of 8 SensorPackets
MemoryPool<SensorPacket, sizeof(SensorPacket), 8> mem_pool;

// Allocate and construct
SensorPacket* p = mem_pool.construct(sensor_id, timestamp);

// Use it
p->readings[0] = 23.5f;

// Destroy and return to pool
mem_pool.destroy(p);

// Thread-safe version
ThreadSafePool<SensorPacket, sizeof(SensorPacket), 8> safe_pool;
SensorPacket* p2 = safe_pool.construct(sensor_id, timestamp);
safe_pool.destroy(p2);
```

## Benchmark Results

| Operation | malloc/free | MemoryPool | Speedup |
|---|---|---|---|
| Alloc/dealloc (ns/op) | 46 ns/op | 20 ns/op | 2.3x faster |
| Fragmentation test (ns total) | 76375 ns total | 33042 ns total | 2.3x faster |

*Benchmarked on Apple M-series, 100,000 iterations*

## Design Trade-offs

- **Fixed block size** — one pool per type. Eliminates fragmentation structurally.
- **Single mutex** in ThreadSafePool — simpler and correct over per-block locking.
- **Non-copyable pools** — prevents two owners corrupting the same free list.
- **Static assertions** — bad template parameters caught at compile time.

## Relevance

Directly applicable to embedded firmware development where:
- `malloc` is banned or restricted (MISRA C guidelines)
- Real-time deadlines require deterministic memory operations
- RAM is constrained (256KB–2MB typical in embedded targets)