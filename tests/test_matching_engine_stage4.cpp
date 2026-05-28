// 阶段四验证标准（章节 6.3 / 6.4 / 6.5）
//
// 6.3 价格改进（Price Improvement）：
//     买方报高价、卖方报低价时，以被动方价格成交；
//     TradeEvent.price_improvement() 返回买方节省 / 卖方多得的 tick 数。
//
// 6.4 冰山订单（Iceberg）：
//     peak_qty 被吃完后自动从 hidden_qty 补充下一峰；
//     补充后的新峰被移到同价位队尾，丢失原先的时间优先权。
//
// 6.5 Post-only：
//     若提交时会立刻成 Taker（即会触发成交），系统直接拒单（CANCELLED）；
//     若不会触发成交，则正常入簿。
//
// 每个用例最后调用 verify_consistency()，应为 true。

#include <iostream>
#include <vector>
#include <cassert>
#include "../src/matching_engine.hpp"

// ── 全局收集器：方便用 assert 验证撮合细节 ────────────────────
static std::vector<TradeEvent> g_trades;
static std::vector<OrderEvent> g_orders;

static void reset_collectors() {
    g_trades.clear();
    g_orders.clear();
}

static void collect_trade(const TradeEvent& e) {
    g_trades.push_back(e);
    std::cout << "  [TRADE] aggr=" << e.aggressive_order_id
              << " passv=" << e.passive_order_id
              << " trade_price=" << e.trade_price
              << " limit_price=" << e.limit_price
              << " qty=" << e.trade_qty
              << " improv=" << e.price_improvement()
              << " side=" << (e.aggressive_side == Side::BUY ? "BUY" : "SELL")
              << "\n";
}

static void collect_order(const OrderEvent& e) {
    g_orders.push_back(e);
    const char* s = "?";
    switch (e.new_status) {
        case OrderStatus::ACTIVE:           s = "ACTIVE";           break;
        case OrderStatus::FILLED:           s = "FILLED";           break;
        case OrderStatus::PARTIALLY_FILLED: s = "PARTIALLY_FILLED"; break;
        case OrderStatus::CANCELLED:        s = "CANCELLED";        break;
    }
    std::cout << "  [ORDER] id=" << e.order_id
              << " status=" << s
              << " filled=" << e.filled_qty
              << "\n";
}

// 统计某个订单收到的特定状态事件次数
static int count_order_status(OrderID id, OrderStatus st) {
    int cnt = 0;
    for (const auto& e : g_orders)
        if (e.order_id == id && e.new_status == st) ++cnt;
    return cnt;
}

// ── Test 6.3：价格改进 ────────────────────────────────────────
// 卖方先挂 SELL@9900，买方主动 BUY@10000；
// 成交价应为 9900（被动方），price_improvement() 应为 100。
void test_price_improvement() {
    std::cout << "\n====== Test 6.3: 价格改进 ======\n";
    reset_collectors();
    MatchingEngine engine(collect_trade, collect_order);

    engine.submit_order(Order(1, 9900, 100, 100, Side::SELL, OrderType::LIMIT, 1));
    engine.submit_order(Order(2, 10000, 100, 100, Side::BUY,  OrderType::LIMIT, 2));

    assert(g_trades.size() == 1 && "应只产生 1 笔成交");
    const TradeEvent& t = g_trades[0];
    assert(t.aggressive_order_id == 2);
    assert(t.passive_order_id    == 1);
    assert(t.trade_price         == 9900);   // 取被动方价
    assert(t.limit_price         == 10000);  // 主动方报价
    assert(t.trade_qty           == 100);
    assert(t.aggressive_side     == Side::BUY);
    assert(t.price_improvement() == 100);    // 10000 - 9900

    assert(engine.get_book().verify_consistency());
    std::cout << "  PASS\n";
}

// 卖单主动吃买盘的对称测试：BUY@9900 挂在簿，SELL@9800 主动
// 成交价 9900，price_improvement = 9900 - 9800 = 100
void test_price_improvement_sell_side() {
    std::cout << "\n====== Test 6.3b: 价格改进（卖单主动）======\n";
    reset_collectors();
    MatchingEngine engine(collect_trade, collect_order);

    engine.submit_order(Order(11, 9900, 50, 50, Side::BUY,  OrderType::LIMIT, 1));
    engine.submit_order(Order(12, 9800, 50, 50, Side::SELL, OrderType::LIMIT, 2));

    assert(g_trades.size() == 1);
    const TradeEvent& t = g_trades[0];
    assert(t.trade_price         == 9900);
    assert(t.limit_price         == 9800);
    assert(t.price_improvement() == 100);
    assert(t.aggressive_side     == Side::SELL);

    assert(engine.get_book().verify_consistency());
    std::cout << "  PASS\n";
}

// ── Test 6.4：冰山订单 ────────────────────────────────────────
// 场景设计：
//   1) 先挂冰山 SELL@10000  total=100  peak=20  hidden=80   (id=21)
//   2) 再挂普通 SELL@10000  qty=15                          (id=22)
//   3) 提交 BUY@10000 qty=25                                (id=23)
//      → 先吃冰山可见峰 20 手 → 冰山补峰并移到队尾
//      → 此时队列：[normal(15), iceberg(peak=20, hidden=60)]
//      → 继续吃 normal 5 手 → 共 2 笔成交
//   4) 再提交 BUY@10000 qty=10                              (id=24)
//      → 应优先吃 normal 剩下的 10 手（normal 现在排在前面）
//      → 验证：成交对手方为 id=22，不是 id=21
//        ↑ 这就是冰山失去时间优先权的关键证据
void test_iceberg_replenish_and_priority_loss() {
    std::cout << "\n====== Test 6.4: 冰山补峰 + 失去时间优先权 ======\n";
    reset_collectors();
    MatchingEngine engine(collect_trade, collect_order);

    // 构造冰山卖单：可见 20，隐藏 80
    Order iceberg(21, 10000, 100, /*remaining=*/20, Side::SELL, OrderType::LIMIT, 1);
    iceberg.peak_qty   = 20;
    iceberg.hidden_qty = 80;
    engine.submit_order(iceberg);

    // 普通卖单，时间戳更晚，自然排在冰山后面
    engine.submit_order(Order(22, 10000, 15, 15, Side::SELL, OrderType::LIMIT, 2));

    std::cout << "  -- 挂单后盘口（10000 档显示 20+15=35 可见量）--\n";
    engine.get_book().print_depth(5);

    // 第一笔买单：消耗冰山的第一峰，触发补峰
    g_trades.clear();
    engine.submit_order(Order(23, 10000, 25, 25, Side::BUY, OrderType::LIMIT, 3));

    assert(g_trades.size() == 2 && "应产生 2 笔成交（冰山 20 + 普通 5）");
    assert(g_trades[0].passive_order_id == 21 && g_trades[0].trade_qty == 20);
    assert(g_trades[1].passive_order_id == 22 && g_trades[1].trade_qty == 5);

    std::cout << "  -- 补峰 + 重排队后盘口 --\n";
    engine.get_book().print_depth(5);
    assert(engine.get_book().verify_consistency());

    // 第二笔买单：验证冰山确实丢失了优先权
    g_trades.clear();
    engine.submit_order(Order(24, 10000, 10, 10, Side::BUY, OrderType::LIMIT, 4));

    assert(g_trades.size() == 1 && "应只吃 normal 剩下的 10 手");
    assert(g_trades[0].passive_order_id == 22
           && "新峰冰山应在队尾，普通卖单 22 应该被先吃");
    assert(g_trades[0].trade_qty == 10);

    assert(engine.get_book().verify_consistency());
    std::cout << "  PASS\n";
}

// ── Test 6.5：Post-only ───────────────────────────────────────
// 6.5a: 簿中已有对手盘，post-only 会立即成交 → 直接拒单
// 6.5b: 簿中对手盘价格不可达，post-only 正常入簿
void test_post_only_rejected_when_would_match() {
    std::cout << "\n====== Test 6.5a: post-only 会立即成交 → 拒单 ======\n";
    reset_collectors();
    MatchingEngine engine(collect_trade, collect_order);

    // 簿中先挂 SELL @ 9900
    engine.submit_order(Order(31, 9900, 50, 50, Side::SELL, OrderType::LIMIT, 1));
    g_trades.clear();
    g_orders.clear();

    // post-only BUY @ 10000，会立即匹配到 9900 的卖盘 → 应该被拒
    Order po(32, 10000, 50, 50, Side::BUY, OrderType::LIMIT, 2);
    po.is_post_only = true;
    engine.submit_order(po);

    assert(g_trades.empty() && "post-only 被拒时不应产生任何成交");
    assert(count_order_status(32, OrderStatus::CANCELLED) == 1
           && "post-only 拒单应触发 1 次 CANCELLED 事件");

    // 簿中应该只剩 SELL @ 9900，没有买盘
    assert(engine.get_book().best_bid() == 0
           && "post-only 被拒后不应入簿");
    assert(engine.get_book().best_ask() == 9900);
    assert(engine.get_book().verify_consistency());
    std::cout << "  PASS\n";
}

void test_post_only_accepted_when_no_match() {
    std::cout << "\n====== Test 6.5b: post-only 不会成交 → 正常入簿 ======\n";
    reset_collectors();
    MatchingEngine engine(collect_trade, collect_order);

    // 簿中先挂 SELL @ 9900
    engine.submit_order(Order(41, 9900, 50, 50, Side::SELL, OrderType::LIMIT, 1));
    g_trades.clear();
    g_orders.clear();

    // post-only BUY @ 9800，低于 best_ask=9900 → 不会成交 → 正常入簿
    Order po(42, 9800, 30, 30, Side::BUY, OrderType::LIMIT, 2);
    po.is_post_only = true;
    engine.submit_order(po);

    assert(g_trades.empty() && "无对手盘可吃，不应产生成交");
    assert(count_order_status(42, OrderStatus::CANCELLED) == 0
           && "post-only 在不会成交时不应被取消");

    assert(engine.get_book().best_bid() == 9800);
    assert(engine.get_book().best_ask() == 9900);
    assert(engine.get_book().verify_consistency());
    std::cout << "  PASS\n";
}

// 6.5c: 对称：post-only SELL 在 best_bid 之下/之上的边界情况
void test_post_only_sell_side() {
    std::cout << "\n====== Test 6.5c: post-only SELL 对称行为 ======\n";

    // case 1：会立即成交（SELL@9900 ≤ best_bid=10000）→ 拒单
    {
        reset_collectors();
        MatchingEngine engine(collect_trade, collect_order);
        engine.submit_order(Order(51, 10000, 50, 50, Side::BUY, OrderType::LIMIT, 1));
        g_trades.clear();
        g_orders.clear();

        Order po(52, 9900, 50, 50, Side::SELL, OrderType::LIMIT, 2);
        po.is_post_only = true;
        engine.submit_order(po);

        assert(g_trades.empty());
        assert(count_order_status(52, OrderStatus::CANCELLED) == 1);
        assert(engine.get_book().best_ask() != 9900);
    }

    // case 2：不会成交（SELL@10100 > best_bid=10000）→ 入簿
    {
        reset_collectors();
        MatchingEngine engine(collect_trade, collect_order);
        engine.submit_order(Order(61, 10000, 50, 50, Side::BUY, OrderType::LIMIT, 1));
        g_trades.clear();
        g_orders.clear();

        Order po(62, 10100, 30, 30, Side::SELL, OrderType::LIMIT, 2);
        po.is_post_only = true;
        engine.submit_order(po);

        assert(g_trades.empty());
        assert(count_order_status(62, OrderStatus::CANCELLED) == 0);
        assert(engine.get_book().best_ask() == 10100);
    }

    std::cout << "  PASS\n";
}

int main() {
    test_price_improvement();
    test_price_improvement_sell_side();
    test_iceberg_replenish_and_priority_loss();
    test_post_only_rejected_when_would_match();
    test_post_only_accepted_when_no_match();
    test_post_only_sell_side();

    std::cout << "\n========================================\n";
    std::cout << "  所有阶段四测试通过 ✓\n";
    std::cout << "========================================\n";
    return 0;
}
