// 阶段三验证标准
// 买价 100，卖价 99：立即撮合，成交价 99（卖单为被动方）
// 买价 100 量 10，卖价 100 量 6：部分成交，剩余 4 留买盘
// 一笔大买单横扫三档卖盘：产生三笔 TradeEvent，价格依次递增
// 每次撮合后 verify_consistency() 返回 true
#include <iostream>
#include "../src/matching_engine.hpp"

// ── 打印辅助函数 ──────────────────────────────────────────
void print_trade(const TradeEvent& e) {
    std::cout << "  [TRADE] 主动方id=" << e.aggressive_order_id
              << " 被动方id=" << e.passive_order_id
              << " 成交价=" << e.trade_price
              << " 成交量=" << e.trade_qty
              << " 方向=" << (e.aggressive_side == Side::BUY ? "BUY" : "SELL")
              << "\n";
}

void print_order(const OrderEvent& e) {
    std::string s;
    switch (e.new_status) {
        case OrderStatus::ACTIVE:           s = "ACTIVE";           break;
        case OrderStatus::FILLED:           s = "FILLED";           break;
        case OrderStatus::PARTIALLY_FILLED: s = "PARTIALLY_FILLED"; break;
        case OrderStatus::CANCELLED:        s = "CANCELLED";        break;
    }
    std::cout << "  [ORDER] id=" << e.order_id
              << " 状态=" << s
              << " 本次成交量=" << e.filled_qty
              << "\n";
}

// ── 测试1：买价高于卖价，立即撮合 ───────────────────────
void test1() {
    std::cout << "\n====== Test1: 买@10000 vs 卖@9900，应以9900成交 ======\n";
    MatchingEngine engine(print_trade, print_order);

    // 先挂卖单，再提交买单
    engine.submit_order(Order(1, 9900, 100, 100, Side::SELL, OrderType::LIMIT, 1));
    engine.submit_order(Order(2, 10000, 100, 100, Side::BUY, OrderType::LIMIT, 2));

    std::cout << "  verify_consistency=" << engine.get_book().verify_consistency() << "\n";
    engine.get_book().print_depth(5);
}

// ── 测试2：买10手@10000，卖6手@10000，部分成交，剩余4留盘 ──
void test2() {
    std::cout << "\n====== Test2: 买10手@10000 vs 卖6手@10000，剩4手留买盘 ======\n";
    MatchingEngine engine(print_trade, print_order);

    engine.submit_order(Order(3, 10000, 10, 10, Side::BUY,  OrderType::LIMIT, 1));
    engine.submit_order(Order(4, 10000,  6,  6, Side::SELL, OrderType::LIMIT, 2));

    std::cout << "  verify_consistency=" << engine.get_book().verify_consistency() << "\n";
    engine.get_book().print_depth(5);
}

// ── 测试3：大买单横扫三档卖盘，产生3笔成交 ────────────────
void test3() {
    std::cout << "\n====== Test3: 大买单横扫3档卖盘，应产生3笔TradeEvent ======\n";
    MatchingEngine engine(print_trade, print_order);//巧妙地传入两个打印函数
    /*
    MatchingEngine engine(print_trade, print_order)
       │
       └── 构造时：on_trade_ = print_trade
                  on_order_ = print_order（存起来备用）

    engine.submit_order(买单)
        │
        └── 触发撮合 → on_trade_({...}) → 实际执行 print_trade({...}) → 打印
                    → on_order_({...}) → 实际执行 print_order({...}) → 打印
    */

    engine.submit_order(Order(5, 10010, 30, 30, Side::SELL, OrderType::LIMIT, 1));
    engine.submit_order(Order(6, 10020, 30, 30, Side::SELL, OrderType::LIMIT, 2));
    engine.submit_order(Order(7, 10030, 30, 30, Side::SELL, OrderType::LIMIT, 3));

    std::cout << "  -- 挂单后盘口 --\n";
    engine.get_book().print_depth(5);

    // 大买单：报价10050，量100，横扫全部3档（共90手），剩10手留买盘
    engine.submit_order(Order(8, 10050, 100, 100, Side::BUY, OrderType::LIMIT, 4));

    std::cout << "  verify_consistency=" << engine.get_book().verify_consistency() << "\n";
    std::cout << "  -- 撮合后盘口 --\n";
    engine.get_book().print_depth(5);
}

int main() {
    test1();
    test2();
    test3();
    return 0;
}
