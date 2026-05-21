#pragma once
#include <cstdint>
#include <string>

using OrderId   = uint64_t;
using Price     = int64_t;
using Quantity  = uint32_t;
using Timestamp = uint64_t;

enum class Side : uint8_t { BUY, SELL };
enum class OrderType : uint8_t { LIMIT, MARKET };