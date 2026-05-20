// matching_engine.cpp
#include "matching_engine.hpp"

void MatchingEngine::match_buy_order(Order& order) {
    while (order.remaining_qty>0 && !book_.get_asks().empty()){//查看卖单有没有
        auto ask_it = book_.get_asks_mut().begin();//begin是ask_的第一个，也就是最低价格卖单
        //此时卖单是被动方，成交价格以被动方为主
        Price best_ask = ask_it->first;//iterator指向<Price,PriceLevel>
        //提取最优卖价（卖单中最便宜的）
        
        if (order.price<best_ask) break;//订单报价低于卖单最优价格，无法成交停止撮合

        PriceLevel& level = ask_it->second;
        while (order.remaining_qty>0 && !level.orders.empty()){//循环交易完成同一个pricelevel的所有订单
            Order& passive = level.orders.front();
            Price trade_price = passive.price;
            Quantity fill_qty = std::min(order.remaining_qty,passive.remaining_qty);
            //fill_qty:本次撮合实际成交的数量（filled quantity，已成交量）
            on_trade_({order.id,passive.id,trade_price,fill_qty,Side::BUY});//记录一下交易的两笔订单状态
            order.remaining_qty -= fill_qty;
            passive.remaining_qty -= fill_qty;
            level.total_qty -= fill_qty;

            if(passive.remaining_qty ==0 ){//正常交易完，没有滑点
                OrderID passive_id = passive.id;
                book_.cancel_order(passive_id);
                break;
            }
        }
    }
    // 限价单有的剩余量则入簿，IOC订单剩余量直接丢弃
    if (order.remaining_qty > 0 && !order.is_ioc){
        book_.add_order(order);
    }
}

//处理卖单
void MatchingEngine::match_sell_order(Order& order) {
    while (order.remaining_qty>0 && !book_.get_bids().empty()){//查看买单有没有
        auto bid_it = book_.get_bids_mut().begin();//begin是bids_的第一个，也就是最高价买单
        //此时买单是被动方，成交价格以被动方为主
        Price best_bid = bid_it->first;//最高价的买单
        if (order.price>best_bid) break;

        PriceLevel& level = bid_it->second;
        while (order.remaining_qty>0 && !level.orders.empty()){//循环交易完成同一个pricelevel的所有订单
            Order& passive = level.orders.front();
            Price trade_price = passive.price;
            Quantity fill_qty = std::min(order.remaining_qty,passive.remaining_qty);

            on_trade_({order.id,passive.id,trade_price,fill_qty,Side::SELL});//记录一下交易的两笔订单状态
            order.remaining_qty -= fill_qty;
            passive.remaining_qty -= fill_qty;
            level.total_qty -= fill_qty;

            if(passive.remaining_qty ==0 ){//正常交易完，没有滑点
                OrderID passive_id = passive.id;
                book_.cancel_order(passive_id);
                break;
            }
        }
    }
    // 限价单有的剩余量则入簿，IOC订单剩余量直接丢弃
    if (order.remaining_qty > 0 && !order.is_ioc){
        book_.add_order(order);
    }
}
//因为 OrderBook book_;是private的，外部不能访问，所以cancel_order方法需要在matching_engine里面重写实现
bool MatchingEngine::cancel_order(OrderID id) {
    Order* o = book_.find_order(id);
    Quantity filled = o ? (o->quantity - o->remaining_qty) : 0;
    bool success = book_.cancel_order(id);   // 调用 OrderBook 的那个
    if (success) {
        on_order_({id, OrderStatus::CANCELLED,filled});  // 额外触发回调通知外部
        //on_order_({order.id, OrderStatus::ACTIVE});这个传入的参数是OrderEvent为什么没有quantity？
        //A：这里用的是 C++ 的聚合初始化（aggregate initialization），当你的初始化列表比结构体字段少时，剩余字段自动补零。
    }
    return success;
}


void MatchingEngine::modify_order(OrderID id, Price new_price,Quantity new_qty){
    const Order* orig = book_.find_order(id);
    if (!orig) return;

    Side orig_side = orig->side;
    cancel_order(id);// ⚠️ 这一行之后 orig 已经悬垂了
    // 不能再使用 orig->xxx
    Order modified(id, new_price,new_qty,new_qty,orig_side,OrderType::LIMIT,get_timestamp());
    submit_order(modified);
    //被修改的订单会失去原来的"价格-时间优先级"（time priority），重新排到该价格档位的队尾。


}



void MatchingEngine::submit_order(Order order){
    on_order_({order.id, OrderStatus::ACTIVE, 0});//filled_qty暂时为0，还没有成交
    //这里其实就是调用回调函数

    if(order.type == OrderType::MARKET){
        /**市价单:必须按照市场价格全部购买或卖出，
        所以可以把买的price调到最高
        卖的price当作0
        从而横扫所有可成交档位*/

        order.price = (order.side == Side::BUY)? INVALID_PRICE : 0;
        //这个价格已经不重要了，市价单都是以被动方的价格为准的
    }
    bool is_fok = order.is_fok;
    if(is_fok) {
        """fill or kill预先检查流动性"""
        Quantity avail = (order.side == Side::BUY)
                        ? book_.available_qty_for_buy(order.price)
                        : book_.available_qty_for_sell(order.price);
        if (avail < order.quantity){// kill
            on_order_({order.id, OrderStatus::CANCELLED,0});//回调函数调用，表示没有成交
            return;
        }
    }

    //以下都是ioc的Immediate_Or_Cancel
    if(order.side == Side::BUY){
        match_buy_order(order);
    } else {
        match_sell_order(order);
    }
    if (order.remaining_qty == 0) {
        on_order_({order.id, OrderStatus::FILLED,order.quantity - order.remaining_qty});//订单成功全部交割
    } else if (order.remaining_qty < order.quantity) {
        on_order_({order.id, OrderStatus::PARTIALLY_FILLED,order.quantity-order.remaining_qty});//部分交割
    }


}