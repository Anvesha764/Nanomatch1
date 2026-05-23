cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)
enable_testing()
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

# Phase 1 - STL baseline
add_executable(nanomatch_v1
    src/main.cpp
    src/order_book.cpp
)

# Phase 2 - PriceLevelQueue + pool allocator
add_executable(nanomatch_v2
    src/main_v2.cpp
    src/order_book_v2.cpp
)

# Phase 3 - ITCH parser + data ingestion
add_executable(nanomatch_v3
    src/main_v3.cpp
    src/order_book_v2.cpp
    src/itch_parser.cpp
)

# Synthetic data generator
add_executable(generate_itch
    src/generate_itch.cpp
)

# Phase 4 - SPSC ring buffer + trade logger
add_executable(nanomatch_v4
    src/main_v4.cpp
    src/order_book_v2.cpp
    src/itch_parser.cpp
)
target_link_libraries(nanomatch_v4 Threads::Threads)

# Latency histogram — v2
add_executable(latency_hist
    bench/latency_histogram.cpp
    src/order_book_v2.cpp
)

# Latency histogram — v1 STL baseline
add_executable(latency_hist_v1
    bench/latency_histogram_v1.cpp
    src/order_book.cpp
)

# Side-by-side v1 vs v2 latency comparison
# bench_v1.cpp and bench_v2.cpp are separate TUs so each only sees one book header
add_executable(latency_compare
    bench/latency_compare.cpp
    bench/bench_v1.cpp
    bench/bench_v2.cpp
    src/order_book.cpp
    src/order_book_v2.cpp
)

# Tests
add_executable(run_tests
    tests/test_matching.cpp
    src/order_book_v2.cpp
)
add_test(NAME matching_tests COMMAND run_tests)
set_tests_properties(matching_tests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)