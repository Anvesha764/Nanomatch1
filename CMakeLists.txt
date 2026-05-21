cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)

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