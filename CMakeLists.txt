cmake_minimum_required(VERSION 3.15)
project(MemoryPoolAllocator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_compile_options(-Wall -Wextra -Wpedantic)


add_executable(pool_demo src/main.cpp)
target_include_directories(pool_demo PRIVATE include)


add_executable(bench benchmarks/bench.cpp)
target_include_directories(bench PRIVATE include)


add_executable(tests tests/test_pool.cpp)
target_include_directories(tests PRIVATE include)