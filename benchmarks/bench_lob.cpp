// benchmarks/bench_lob.cpp
// ============================================================
//  LOB 撮合引擎 baseline benchmark（未做内存池/数组化优化前的本底数据）
//
//  Build : cmake -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build
//  Run   : ./build/lob_bench --benchmark_format=console
//
//  指标说明：
//    - BM_AddOrder    : 纯挂单延迟（不触发撮合）
//    - BM_CancelOrder : O(1) 撤单延迟（含一次补单维持簿规模）
//    - BM_Execute    : 单笔被动单撮合延迟
// ============================================================

#include <benchmark/benchmark.h>
#include "matching_engine.hpp"

namespace {
// 空回调：避免 std::cout 进入热路径，只测撮合内核本身。
auto noop_trade = [](const TradeEvent&) {};
auto noop_order = [](const OrderEvent&) {};
}  // namespace

// ---------------------------------------------------------------
// BM_AddOrder: 纯挂单延迟（不会触发撮合）
//   全部为 BUY，订单簿里始终没有卖盘，match 循环会因
//   get_asks().empty() == true 立即退出，相当于纯插入。
//   价格在 9900~10099 之间循环分布，模拟多档挂单。
// ---------------------------------------------------------------
static void BM_AddOrder(benchmark::State& state) {
    MatchingEngine engine(noop_trade, noop_order);
    OrderID id = 0;
    for (auto _ : state) {
        Price p = 9900 + static_cast<Price>(id % 200);
        Order o(id, p, 100, 100, Side::BUY, OrderType::LIMIT, id);
        engine.submit_order(o);
        ++id;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_AddOrder);

// ---------------------------------------------------------------
// BM_CancelOrder: 撤单延迟
//   预填 N 笔订单后，每次迭代：撤掉一笔 -> 立即用同样 id 补一笔，
//   维持订单簿规模恒定。实测值约为 (cancel + add)，
//   做减法即可得到纯 cancel 成本（约 ns/op_cancel = ns/op - ns/op_add）。
// ---------------------------------------------------------------
static void BM_CancelOrder(benchmark::State& state) {
    MatchingEngine engine(noop_trade, noop_order);
    const OrderID N = static_cast<OrderID>(state.range(0));
    for (OrderID i = 0; i < N; ++i) {
        Price p = 9900 + static_cast<Price>(i % 200);
        Order o(i, p, 100, 100, Side::BUY, OrderType::LIMIT, i);
        engine.submit_order(o);
    }

    OrderID cur = 0;
    for (auto _ : state) {
        OrderID target = cur % N;
        engine.cancel_order(target);
        Price p = 9900 + static_cast<Price>(target % 200);
        Order o(target, p, 100, 100, Side::BUY, OrderType::LIMIT, target);
        engine.submit_order(o);
        ++cur;
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CancelOrder)->Arg(1000)->Arg(10000)->Arg(100000);

// ---------------------------------------------------------------
// BM_Execute: 单笔被动单撮合延迟
//   每次迭代：构造新引擎 + 挂一笔 SELL 被动单（计时暂停），
//   然后提交一笔同价 BUY 主动单触发立即成交（计时恢复）。
//   PauseTiming/ResumeTiming 把建簿成本排除在外。
// ---------------------------------------------------------------
static void BM_Execute(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        MatchingEngine engine(noop_trade, noop_order);
        engine.submit_order(
            Order(1, 10000, 100, 100, Side::SELL, OrderType::LIMIT, 1));
        state.ResumeTiming();

        engine.submit_order(
            Order(2, 10000, 100, 100, Side::BUY, OrderType::LIMIT, 2));
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Execute);

BENCHMARK_MAIN();
