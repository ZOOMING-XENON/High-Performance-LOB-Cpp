//matching_engine.hpp
# pragma once
#include "order_book.hpp"
#include "event_handler.hpp"

class MatchingEngine {

public:
    MatchingEngine(TradeCallback on_trade , OrderCallback on_order):on_trade_(std::move(on_trade)),on_order_(std::move(on_order)){}
    // 等价于：调用 std::function 的"移动构造函数"
    // on_trade_ 直接接管 on_trade 的内部资源
    // 之后 on_trade 变成一个"空的" std::function（不能再调用）
    // 构造函数

    //对外暴露的接口就是submit还有cancel
    void submit_order(Order order);
    bool cancel_order(OrderID id);
    void modify_order(OrderID id, Price new_price,Quantity new_qty);

    const OrderBook& get_book() const {return book_;}
    //左边的 const 是为了保护返回值（防篡改），右边的 const 是为了保护调用者（守规矩）。
private: 
    OrderBook book_;
    TradeCallback on_trade_;
    OrderCallback on_order_;

    void match_buy_order(Order& order);
    void match_sell_order(Order& order);
};