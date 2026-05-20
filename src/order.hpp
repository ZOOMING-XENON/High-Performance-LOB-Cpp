// order.hpp
#pragma once
#include "types.hpp"

struct Order {
    OrderID id;
    Price price;
    Quantity quantity;
    Quantity remaining_qty;
    Side side;
    OrderType type;
    Timestamp timestamp;
    bool is_ioc = false;
    bool is_fok = false;
    /**IOC 的全称是 Immediate-Or-Cancel（立即成交或取消）。
    这是金融交易系统中一种常见的订单执行策略（Order Time-in-Force）：
    Immediate（立即）：订单提交后，系统立刻尝试以指定价格撮合成交
    Or-Cancel（或取消）：撮合完成后，若订单还有剩余未成交数量，立即丢弃，不进入订单簿排队等待
    */

    Order(OrderID id, Price price , Quantity qty,  Quantity remaining_qty, Side side,OrderType type, Timestamp ts):
        id(id),
        price(price),
        quantity(qty),
        remaining_qty(remaining_qty),
        side(side),
        type(type),
        timestamp(ts)
        {}
    //构造函数：一般格式
    /**
     * ClassName(parameters) 
            : member1(value1), member2(value2), ...
        {
            // 构造函数体
        }
     */
    bool is_fully_filled() const {return remaining_qty==0;}
    //在函数声明尾部加 const：表示函数内部不会修改类成员的变量：e.g.quantity，side等
    // 因为一般order是一个const对象，操作const对象编译器想保证操作步骤中不会修改const对象内容，所以需要使用带有const的函数（编译器才不会报错）
};