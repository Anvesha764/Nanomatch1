// include/types.hpp
#pragma once
#include <cstdint>
#include <string>

using OrderId   = uint64_t;
using Price     = int64_t;   // price in ticks (e.g., 10050 = $100.50)
using Quantity  = uint32_t;
using Timestamp = uint64_t;

enum class Side : uint8_t { BUY, SELL };
enum class OrderType : uint8_t { LIMIT, MARKET };