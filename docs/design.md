# Memory Pool Allocator — Design Document

## Problem Statement

Standard malloc/free is unsuitable for embedded and real-time systems
for two reasons:

1. Non-deterministic execution time — malloc's speed depends on heap
   state. A task with a 5ms deadline cannot afford malloc taking 2ms
   on a bad run.

2. Fragmentation — repeated alloc/free of varying sizes fragments heap
   memory over time. On a device with 256KB RAM this is fatal.

## Design Goals

- O(1) allocation and deallocation — guaranteed, not average case
- Zero fragmentation — structurally impossible, not just unlikely
- Compile-time memory footprint — no surprises at runtime
- Thread-safe variant for multi-threaded embedded targets
- No external dependencies — deployable on bare-metal systems

## Core Data Structure: The Free List

Each free block stores a pointer to the next free block in the same
memory the block would use for data. This means bookkeeping costs
zero extra bytes — the metadata lives inside the free blocks themselves.

## Key Design Decisions

### 1. Fixed block size (template parameter)
Trade-off: one pool cannot serve multiple types. Payoff: zero
fragmentation is structurally guaranteed, not just hoped for.

### 2. Single mutex in ThreadSafePool
Per-block locking would reduce contention under heavy load but adds
significant complexity. Pool operations are microseconds — a single
mutex is correct, simple, and sufficient for typical embedded workloads.

### 3. Separate allocate() and construct()
allocate() returns raw void* — useful when construction must be
deferred. construct() combines allocation + placement new for
the common case. Mirrors how std::allocator works in the STL.

### 4. Non-copyable pools
A copied pool would share the same backing array. Two owners of
the same memory with independent free lists would corrupt each
other silently. Deleting copy/move makes this a compile error.

### 5. Static assertions on template parameters
Bad parameters (BlockSize < sizeof(T)) are caught at compile time
with a readable message. In embedded you want the earliest possible
failure — a compile error beats a runtime crash on a device.

## Benchmark Results



malloc/free   : X ns/op
MemoryPool    : X ns/op
Speedup       : Xx faster

## Limitations

- Fixed block size: one pool per type. A MultiPool layer would
  address this (future work).
- Pool size fixed at compile time: must know worst-case count upfront.
- No built-in overflow recovery: caller handles nullptr from allocate().

## Files

| File | Purpose |
|---|---|
| include/memory_pool.h | Core allocator — free list, O(1) ops |
| include/thread_safe_pool.h | Mutex wrapper for concurrent access |
| src/main.cpp | Realistic usage demo — sensor pipeline |
| tests/test_pool.cpp | Unit tests — correctness and edge cases |
| benchmarks/bench.cpp | Timing comparison vs malloc |