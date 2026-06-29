#include <iostream>
#include <cassert>
#include <cstdint>
#include "memory_pool.h"
#include "thread_safe_pool.h"


static int tests_run    = 0;
static int tests_passed = 0;

#define TEST(name) void name()
#define RUN(name)  do { \
    ++tests_run; \
    std::cout << "  running " #name " ... "; \
    name(); \
    ++tests_passed; \
    std::cout << "PASS\n"; \
} while(0)



struct Small {
    int x;
    Small(int v) : x(v) {}
    ~Small() { x = -1; }   
};

struct Large {
    uint8_t data[128];
    int     id;
    Large(int i) : id(i) { data[0] = 0xAB; data[127] = 0xCD; }
};



TEST(test_basic_alloc_dealloc) {
    MemoryPool<Small, 8, 4> mp;

    assert(mp.available() == 4);
    assert(mp.capacity()  == 4);
    assert(mp.empty()     == true);
    assert(mp.full()      == false);

    Small* a = mp.construct(10);
    assert(a->x == 10);
    assert(mp.available() == 3);
    assert(mp.empty()     == false);

    mp.destroy(a);
    assert(mp.available() == 4);
    assert(mp.empty()     == true);
}

TEST(test_fill_to_capacity) {
    MemoryPool<Small, 8, 4> mp;

    Small* ptrs[4];
    for (int i = 0; i < 4; ++i) {
        ptrs[i] = mp.construct(i);
        assert(ptrs[i]->x == i);
    }

    assert(mp.full()      == true);
    assert(mp.available() == 0);

    // Free all
    for (int i = 0; i < 4; ++i) {
        mp.destroy(ptrs[i]);
    }
    assert(mp.empty() == true);
}

TEST(test_exhaustion_returns_nullptr) {
    MemoryPool<Small, 8, 2> mp;

    mp.construct(1);
    mp.construct(2);

    
    void* overflow = mp.allocate();
    assert(overflow == nullptr);
}

TEST(test_reuse_after_free) {
    
    MemoryPool<Small, 8, 2> mp;

    for (int cycle = 0; cycle < 100; ++cycle) {
        Small* a = mp.construct(cycle);
        Small* b = mp.construct(cycle + 1);
        assert(mp.full());
        mp.destroy(a);
        mp.destroy(b);
        assert(mp.empty());
    }
    
}



TEST(test_large_type) {
    MemoryPool<Large, sizeof(Large), 8> mp;

    Large* l = mp.construct(99);
    assert(l->id       == 99);
    assert(l->data[0]  == 0xAB);
    assert(l->data[127]== 0xCD);
    assert(mp.available() == 7);

    mp.destroy(l);
    assert(mp.available() == 8);
}

TEST(test_multiple_types_independent) {
    
    MemoryPool<Small, 8, 4> small_pool;
    MemoryPool<Large, sizeof(Large), 4> large_pool;

    Small* s = small_pool.construct(7);
    Large* l = large_pool.construct(99);

    assert(s->x  == 7);
    assert(l->id == 99);
    assert(small_pool.available() == 3);
    assert(large_pool.available() == 3);

    small_pool.destroy(s);
    large_pool.destroy(l);

    assert(small_pool.empty());
    assert(large_pool.empty());
}

TEST(test_lifo_order) {
    
    MemoryPool<Small, 8, 3> mp;

    Small* a = mp.construct(1);
    Small* b = mp.construct(2);
    Small* c = mp.construct(3);

    mp.destroy(c);
    mp.destroy(b);
    mp.destroy(a);

    
    Small* first = mp.construct(99);
    assert(first == a);   
    mp.destroy(first);
}

TEST(test_thread_safe_pool_basic) {
    ThreadSafePool<Small, 8, 4> tp;

    assert(tp.available() == 4);
    assert(tp.capacity()  == 4);

    Small* s = tp.construct(55);
    assert(s->x == 55);
    assert(tp.available() == 3);

    tp.destroy(s);
    assert(tp.available() == 4);
}



int main() {
    std::cout << "=== Memory Pool Test Suite ===\n\n";

    RUN(test_basic_alloc_dealloc);
    RUN(test_fill_to_capacity);
    RUN(test_exhaustion_returns_nullptr);
    RUN(test_reuse_after_free);
    
    RUN(test_large_type);
    RUN(test_multiple_types_independent);
    RUN(test_lifo_order);
    RUN(test_thread_safe_pool_basic);

    std::cout << "\n" << tests_passed << "/" << tests_run << " tests passed\n";

    return (tests_passed == tests_run) ? 0 : 1;
}