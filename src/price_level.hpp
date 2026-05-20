//price_lecel.hpp
#pragma once
#include "order.hpp"
#include <list>

struct PriceLevel {
    Price price;
    Quantity total_qty = 0;
    std::list<Order> orders;

    explicit PriceLevel(Price p): price(p){}
    //迭代器不是次序，也不是副本，而是“指向容器内部某个元素的对象（类似指针）”。
    std::list<Order>::iterator add_order(const Order& order){
        orders.push_back(order);
        total_qty += order.remaining_qty;
        return std::prev(orders.end());
    }
    // ::作用域限定符（scope resolution operator）
    //std::list<Order> 是一个类型（模板实例）
    //在这个类型的作用域里有定义 iterator 类型
    //返回的是 iterator，不是 Order 对象
    //orders.end() → 指向 尾后位置（不是最后一个元素）
    //std::prev(orders.end()) → 返回尾后位置的前一个 → 就是最后一个元素的迭代器
    //因为迭代器本质是指针，所以可以快速方便访问auto it = add_order(order);
    //std::cout << it->price;
    void remove_order(std::list<Order>::iterator it){
        total_qty -= it->remaining_qty;
        orders.erase(it);
        
    }
    bool empty() const {return orders.empty();}

};
//C++ 中 struct / class 的 } 后面必须加分号：