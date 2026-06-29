#include <iostream>
#include <cstring>
#include "memory_pool.h"

/**
 * Simulates a sensor packet in an embedded system.
 * In real Qualcomm firmware this might be a DSP audio frame,
 * a modem data packet, or an IMU reading buffer.
 */
struct SensorPacket {
    uint32_t sensor_id;
    uint32_t timestamp_ms;
    float    readings[4];      // e.g. accelerometer x,y,z + temperature
    bool     processed;

    SensorPacket(uint32_t id, uint32_t ts)
        : sensor_id(id), timestamp_ms(ts), processed(false)
    {
        readings[0] = readings[1] = readings[2] = readings[3] = 0.0f;
    }

    void print() const {
        std::cout << "[Packet] id=" << sensor_id
                  << " ts=" << timestamp_ms << "ms"
                  << " processed=" << (processed ? "yes" : "no") << "\n";
    }
};

/**
 * Simulates a real-time processing pipeline:
 * packets arrive, get processed, get released — in a loop.
 * This is the pattern memory pools are built for.
 */
void run_pipeline(MemoryPool<SensorPacket, sizeof(SensorPacket), 4>& mem_pool) {
    std::cout << "\n--- Pipeline Start ---\n";
    std::cout << "Pool slots available: " << mem_pool.available() << "/" << mem_pool.capacity() << "\n\n";

    // Simulate 4 packets arriving (fills the pool completely)
    SensorPacket* packets[4];
    for (int i = 0; i < 4; ++i) {
        packets[i] = mem_pool.construct(i + 1, i * 10);
        packets[i]->readings[0] = i * 1.5f;
        std::cout << "Allocated: ";
        packets[i]->print();
    }

    std::cout << "\nPool after filling: " << mem_pool.available() << " slots left\n";

    // Try to allocate when full — should return nullptr gracefully
    void* overflow = mem_pool.allocate();
    if (overflow == nullptr) {
        std::cout << "Pool exhausted — allocation correctly returned nullptr\n";
    }

    // Process and release packets one by one (simulate real-time handling)
    std::cout << "\n--- Processing and releasing ---\n";
    for (int i = 0; i < 4; ++i) {
        packets[i]->processed = true;
        packets[i]->print();
        mem_pool.destroy(packets[i]);
        packets[i] = nullptr;
        std::cout << "Released. Pool slots available: " << mem_pool.available() << "\n";
    }

    std::cout << "\nPool fully restored: " << mem_pool.empty() << " (1=yes)\n";
}

int main() {
    std::cout << "=== Memory Pool Allocator Demo ===\n";
    std::cout << "Block size: " << sizeof(SensorPacket) << " bytes\n";
    std::cout << "Pool size:  4 blocks\n";
    std::cout << "Total memory pre-allocated: " << sizeof(SensorPacket) * 4 << " bytes\n";

    MemoryPool<SensorPacket, sizeof(SensorPacket), 4> mem_pool;

    run_pipeline(mem_pool);

    // Show pool is reusable — critical property for embedded systems
    std::cout << "\n--- Second pipeline run (pool reuse) ---\n";
    run_pipeline(mem_pool);

    return 0;
}