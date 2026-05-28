//event_handler.hpp
//事件系统设计
# pragma once
# include "types.hpp"
# include <functional>

struct TradeEvent {
    OrderID aggressive_order_id; //主动方：新进来触发撮合的订单
    OrderID passive_order_id;    //被动方：已在订单簿中等待的订单
    Price trade_price; //成交价取被动方的报价
    Price limit_price;//成交时主动方的报价
    Quantity trade_qty;
    Side aggressive_side;
    //价格改进可以反应做市策略质量
    Quantity price_improvement() const {
        if(aggressive_side==Side::BUY){//主动方是买方
            return limit_price - trade_price;
        }else{
            return trade_price - limit_price;
        }
    }
};
struct OrderEvent {
    OrderID order_id;
    OrderStatus new_status;
    Quantity filled_qty;//已经成交了的数量
};
using TradeCallback = std::function<void(const TradeEvent&)>;
using OrderCallback = std::function<void(const OrderEvent&)>;
//用了functional就相当于TradeCallback就是一个函数类型，和int一样，可以定义TradeCallback t这种
//std::function<void(const TradeEvent&)>
//void: 表示这个函数没有返回值。
//(const TradeEvent&): 表示这个函数必须接收一个参数，且参数类型是 TradeEvent 的常量引用（为了效率，避免拷贝）。
//std::function<返回类型(参数类型)>
/**
 * TradeCallback 就是一个插槽，你可以往里面塞任何形状匹配的函数。
 * 订单簿内部不需要关心外面谁在监听、怎么处理——它只管在成交时"
 * 按一下这个按钮"，按钮背后接什么逻辑完全由外部决定。
 * 这种设计模式叫回调（Callback）。
 */