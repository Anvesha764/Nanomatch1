#include "order_book.hpp"
#include <benchmark/benchmark.h>

static void BM_V1_AddOrder_NoMatch(benchmark::State& state) {
    OrderBook book;
    uint64_t id = 1;
    for (auto _ : state) {
        // Alternating BUY/SELL far apart so no matching occurs
        Side side = (id % 2 == 0) ? Side::BUY : Side::SELL;
        Price px  = (side == Side::BUY) ? 990000 : 1010000;
        book.add_order({id++, px, 100, side, id});
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_V1_AddOrder_WithMatch(benchmark::State& state) {
    uint64_t id = 1;
    for (auto _ : state) {
        OrderBook book;
        book.add_order({id++, 1000000, 100, Side::BUY,  id});
        book.add_order({id++,  999000, 100, Side::SELL, id});
    }
    state.SetItemsProcessed(state.iterations());
}

static void BM_V1_CancelOrder(benchmark::State& state) {
    OrderBook book;
    // Pre-fill with 1000 resting orders
    for (uint64_t i = 1; i <= 1000; ++i)
        book.add_order({i, 1000000 - (Price)(i * 100), 100, Side::BUY, i});

    uint64_t id = 1;
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.cancel_order(id));
        id = (id % 1000) + 1;
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_V1_AddOrder_NoMatch)->Threads(1)->UseRealTime();
BENCHMARK(BM_V1_AddOrder_WithMatch)->Threads(1)->UseRealTime();
BENCHMARK(BM_V1_CancelOrder)->Threads(1)->UseRealTime();

BENCHMARK_MAIN();
