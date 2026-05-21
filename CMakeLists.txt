cmake_minimum_required(VERSION 3.20)
project(nanomatch CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Optimization flags matching HFT builds
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")

include_directories(include)

add_executable(nanomatch
    src/main.cpp
    src/order_book.cpp
)