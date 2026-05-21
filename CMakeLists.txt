cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

# Phase 1 — STL baseline
add_executable(nanomatch_v1
    src/main.cpp
    src/order_book_v1.cpp
)

# Phase 2 — PriceLevelQueue + pool allocator
add_executable(nanomatch_v2
    src/main_v2.cpp
    src/order_book_v2.cpp
)

# Phase 3 — ITCH parser + data ingestion
add_executable(nanomatch_v3
    src/main_v3.cpp
    src/order_book_v2.cpp
    src/itch_parser.cpp
)

# Synthetic data generator
add_executable(generate_itch
    src/generate_itch.cpp
)

# Phase 4 — SPSC ring buffer + trade logger
add_executable(nanomatch_v4
    src/main_v4.cpp
    src/order_book_v2.cpp
    src/itch_parser.cpp
)
target_link_libraries(nanomatch_v4 Threads::Threads)

# Phase 5 — latency histogram
add_executable(latency_hist
    bench/latency_histogram.cpp
    src/order_book_v2.cpp
)