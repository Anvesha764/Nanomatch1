// src/generate_itch.cpp
#include "itch_parser.hpp"
#include <fstream>
#include <random>
#include <iostream>
#include <cstring>

// Write big-endian values
void write_be16(std::ofstream& f, uint16_t v) {
    v = bswap16(v);
    f.write(reinterpret_cast<char*>(&v), 2);
}
void write_be32(std::ofstream& f, uint32_t v) {
    v = bswap32(v);
    f.write(reinterpret_cast<char*>(&v), 4);
}
void write_be64(std::ofstream& f, uint64_t v) {
    v = bswap64(v);
    f.write(reinterpret_cast<char*>(&v), 8);
}

int main() {
    std::ofstream out("test_data.itch", std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Cannot create test_data.itch\n";
        return 1;
    }

    std::mt19937 rng(42);
    uint32_t base_price = 1000000; // $100.00 (4 decimal places)
    int N = 1000000;

    std::cout << "Generating " << N << " ITCH messages...\n";

    for (int i = 0; i < N; ++i) {
        itch_add_order msg{};
        msg.msg_type = 'A';

        // timestamp (6 bytes) — just use counter
        uint64_t ts = i * 1000;
        msg.timestamp[0] = (ts >> 40) & 0xFF;
        msg.timestamp[1] = (ts >> 32) & 0xFF;
        msg.timestamp[2] = (ts >> 24) & 0xFF;
        msg.timestamp[3] = (ts >> 16) & 0xFF;
        msg.timestamp[4] = (ts >>  8) & 0xFF;
        msg.timestamp[5] = (ts >>  0) & 0xFF;

        msg.order_ref_num = bswap64((uint64_t)(i + 1));
        msg.buy_sell      = (i % 2 == 0) ? 'B' : 'S';
        msg.shares        = bswap32(100);

        // price spread: ±50 ticks around base
        int32_t offset = (rng() % 101) - 50;
        msg.price = bswap32(base_price + offset * 100);

        std::memcpy(msg.stock, "AAPL    ", 8);

        // write length prefix then message
        uint16_t len = sizeof(itch_add_order);
        write_be16(out, len);
        out.write(reinterpret_cast<char*>(&msg), len);
    }

    out.close();
    std::cout << "Done — test_data.itch created\n";
    return 0;
}