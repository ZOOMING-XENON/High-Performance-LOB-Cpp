// order_book.hpp
#pragma once
#include "price_level.hpp"
#include <map>
#include <unordered_map>
#include <functional>

class OrderBook{
public:
    using BidMap = std::map<Price,PriceLevel,std::greater<Price>>;
    //添加了比较器
    /**
     * 默认情况下，std::map 使用 std::less<Key>，这意味着它会按照
     *  Key 进行升序排列（从小到大，例如 100, 200, 300）。
        但在你的代码中使用了 std::greater<Price>，这告诉 map：
        “请按照 Price 的大小进行比较，大的排在前面”。
     */
    using AskMap = std::map<Price,PriceLevel>;//默认小的排在前面
    //买单大的放前面，卖单小的放前面
    struct OrderLocation {
        Price price;
        Side side;
        std::list<Order>::iterator iter;
    };
    void add_order(const Order& order);//这里的const表示函数返回的值（或引用）是一个常量，调用者不能修改这个返回值。
    bool cancel_order(OrderID id);
    void print_depth(int level = 5) const;
    //这里的 const 表示 print_depth 是一个常量成员函数 (const member function)。
    //这个函数向编译器和代码阅读者保证，它不会修改调用它的对象的任何非静态成员变量
   
    Price best_bid() const;
    Price best_ask() const;
    //在函数声明尾部加 const：表示函数内部不会修改类成员的变量：e.g.quantity，side等

    Quantity bid_qty_at(Price p) const;
    Quantity ask_qty_at(Price p) const;

    const BidMap& get_bids() const {return bids_;}//但是撮合引擎中需要修改每笔订单的信息，所以在下面新增了2个非const的函数
    const AskMap& get_asks() const {return asks_;}
    // 供撮合引擎内部使用的可变版本（需要直接修改PriceLevel内容）
    BidMap& get_bids_mut() {return bids_;}//mut 是 mutable（可变的）
    AskMap& get_asks_mut() {return asks_;}

    Order* find_order(OrderID id);
    Quantity available_qty_for_buy(Price limit_price) const;
    Quantity available_qty_for_sell(Price limit_price) const;

    bool verify_consistency() const;


private:
    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderID,OrderLocation> order_map_;//order_map_ 的作用： 用 OrderID 作 key，直接记录这个订单在"哪个价格、哪个方向、链表里的哪个位置"，实现 O(1) 查找。
    //private 限制的是外部其他类/代码对这个成员的访问，
    //而 .cpp 里写的 OrderBook::add_order
    // 本身就是 OrderBook 类的成员函数，
    //成员函数天然可以访问自己类的所有 private 成员。
};