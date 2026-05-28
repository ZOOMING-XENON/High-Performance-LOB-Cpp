#include "order_book.hpp"
#include<iostream>
Price OrderBook::best_bid()const{
    if (bids_.empty()) return 0;
    return bids_.begin()->first;
}
Price OrderBook::best_ask()const{
    if (asks_.empty()) return INVALID_PRICE;
    return asks_.begin()->first;
}
void OrderBook::add_order(const Order& order) {
    // tutorial 4.2 的代码写在这里
    //重要复习点
    if(order.side == Side::BUY){
        //把这一单order的数据附加在订单簿的bids_里面
        //bids_的数据结构std::map<Price,PriceLevel,std::greater<Price>>;
        //order里面的元素有 id; price;quantity;remaining_qty;side;type;timestamp;
        auto [level_it, _] = bids_.emplace(order.price, PriceLevel(order.price));
        //map的emplace方法比insert好，因为insert会在栈上面临时创建副本，然后再拷贝，但是emplace是直接往里面写，c++11引入的“就地构造”
        //emplace 返回一个 std::pair<iterator, bool>。
        //其中，iterator是一个指向插入对象的指针(插入对象是是一个pair<price,PriceLevel>)当然PriceLevel就是level_it.second，当插入成功的时候bool为true，iterator是插入对象的指针，当插入失败
        //即已经存在对象的时候，bool为false，iterator是已有对象的指针
        auto order_it = level_it->second.add_order(order);
        //level_it本质是指向map bids_的一行<price,PriceLevel>的一个指针
       //现在level_it->second就是一个PriceLevel对象，PriceLevel对象维护着某一个price下所有的订单信息，且有add_order方法
        order_map_[order.id] = {order.price, Side::BUY,order_it};
        //order_map_ 的作用： 用 OrderID 作 key，直接记录这个订单在"哪个价格、哪个方向、链表里的哪个位置"，实现 O(1) 查找。
    }else{//SELL
        //把这一单的order附加在订单簿的asks_里面
        auto [level_it,_] = asks_.emplace(order.price, PriceLevel(order.price));
        auto order_it = level_it->second.add_order(order);
        //order_it是一个指向某一具体order的iterator，返回性质是add_order函数决定的
        order_map_[order.id] = {order.price, Side::SELL, order_it};

    }

}
Order* OrderBook::find_order(OrderID id){
    //直接通过维护的order_map_寻找
    auto it = order_map_.find(id);
    if (it == order_map_.end()) return nullptr;
    return &(*(it->second.iter));//要转换成Order*类型
}

bool OrderBook::cancel_order(OrderID id) {
    // tutorial 4.3 的代码写在这里
    // 一个常被忽视的细节：当某个价格档位的最后一笔订单被撤销后，必须立即从std::map中删除该空档位，否则best_bid(),best_ask()会出问题
    auto map_it = order_map_.find(id);
    if (map_it == order_map_.end()) return false;

    auto& loc = map_it->second;//order_map_的第二个参数是一个OrderLocation对象
    if(loc.side == Side::BUY){
        auto level_it = bids_.find(loc.price);
        level_it->second.remove_order(loc.iter);//order_map_里面存有指向order的iterator
        //level_it->second是一个PriveLevel对象
        if(level_it->second.empty()) bids_.erase(level_it);//直接把这个价格的<price,pricelevel>删除

    } else {
        auto level_it = asks_.find(loc.price);
        level_it->second.remove_order(loc.iter);
        if(level_it->second.empty()) asks_.erase(level_it);
    }
    order_map_.erase(map_it);//这个id的订单信息可以完全删除
    return true;
}

bool OrderBook::verify_consistency() const {
    // tutorial 4.4 的代码写在这里
    if (!bids_.empty() && !asks_.empty()){
        // 订单簿非空时，最高买价必须严格小于最低卖价
        if(bids_.begin()->first >= asks_.begin()->first) return false;//检验订单簿价格排序是否出错
        // order_map_ 中的每个条目必须能正确找到对应的价格档位
        for(auto & [id,loc] : order_map_){
            if(loc.side == Side::BUY){
                if(bids_.find(loc.price) == bids_.end()) return false;
                //end() 是一个指向容器中最后一个有效元素之后位置的迭代器
            } else {
                if(asks_.find(loc.price)==asks_.end()) return false;
            }
            // 验证迭代器指向的订单 id 与 map key 一致，即检查order_map_内部结构是否正常
            if (loc.iter->id != id) return false;
        }
    }
    return true;
}
//OrderBook:: 前缀，这是告诉编译器"这个函数属于 OrderBook 这个类"。
/**
 * 
 * 
 * 在 C++ 里，类（class/struct）本身就是一个作用域，
 * 和命名空间的作用在这一点上是一样的：
 * 类里面定义的所有东西（成员变量、成员函数）
 * 都属于这个类的作用域。
 * :: 叫做作用域解析运算符，
 * 它的意思是"在某个作用域里找"，
 */
void OrderBook::print_depth(int level) const {
    std::cout << "===== ASKS =====\n";
    int cnt = 0;
    for (const auto& [price, lv] : asks_) {
        if (cnt++ >= level) break;
        std::cout << "  " << price << "  qty=" << lv.total_qty << "\n";
    }
    std::cout << "===== BIDS =====\n";
    cnt = 0;
    for (const auto& [price, lv] : bids_) {
        if (cnt++ >= level) break;
        std::cout << "  " << price << "  qty=" << lv.total_qty << "\n";
    }
}

Quantity OrderBook::available_qty_for_buy(Price limit_price) const {
    
    //辅助函数：用于检查买的时候fok的流动性，提前知晓某单是否有足够的对手盘来卖
    
    Quantity total = 0;
    for(auto& [price,level]:asks_){
        if(price > limit_price) break;
        total+=level.total_qty;
    }
    return total;
}

Quantity OrderBook::available_qty_for_sell(Price limit_price) const {
    //辅助函数：用于检查卖的时候fok的流动性，提前知晓某单是否有足够的对手盘来买
    Quantity total = 0;
    for(auto& [price,level]:bids_){
        if(price < limit_price) break;
        total+=level.total_qty;
    }
    return total;
}