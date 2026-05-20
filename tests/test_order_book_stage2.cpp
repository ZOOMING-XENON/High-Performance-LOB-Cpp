/**
 * 阶段二验证标准： 插入 10 笔买单和 10 笔卖单，
 * 打印 5 档深度数量正确；随机撤销其中 5 笔后，verify_consistency() 
 * 返回 true；
 * 尝试撤销不存在的 order_id 返回 false；
 * 重复撤销同一订单返回 false。
 * 
 */
#include<iostream>
#include "../src/order_book.hpp"
#include "../src/order.hpp"
#include<vector>
int main(){
    OrderBook book;//定义一个orderbook对象，其中有完整的BidMap,AskMap
    //插入10笔卖单和买单
    Order o1 = Order(1,9090,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o2 = Order(2,9080,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o3 = Order(3,9085,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o4 = Order(4,9070,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o5 = Order(5,9060,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o6 = Order(6,9070,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o7 = Order(7,9030,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o8 = Order(8,9040,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o9 = Order(9,9050,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o10 = Order(10,9060,100,100,Side::BUY,OrderType::LIMIT,202651);
    Order o11 = Order(11,10010,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o12 = Order(12,10015,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o13 = Order(13,10020,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o14 = Order(14,10030,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o15 = Order(15,10040,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o16 = Order(16,10050,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o17 = Order(17,10060,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o18 = Order(18,10070,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o19 = Order(19,10075,100,100,Side::SELL,OrderType::LIMIT,202651);
    Order o20 = Order(20,10030,100,100,Side::SELL,OrderType::LIMIT,202651);
    std::vector<Order> orders = {o1, o2, o3, o4, o5,o6,o7,o8,o9,o10,o11,o12,o13,o14,o15,o16,o17,o18,o19,o20};
    for (const auto& order : orders) {
        book.add_order(order);
    }
    book.print_depth(5);  // 打印 5 档
    //随机撤销其中 5 笔后，verify_consistency() 
    std::vector<int> cancel_list = {1,3,5,10,11};
    for(auto& cancel_num: cancel_list){
        book.cancel_order(cancel_num);
    }
    std::cout<<"check consistency:"<<book.verify_consistency()<<std::endl;
    //尝试撤销不存在的 order_id 返回 false；
    std::cout<<"check consistency if drop illegal orders:"<<book.cancel_order(21)<<std::endl;
    //重复撤销同一订单返回 false。
    std::cout<<"check consistency if drop deleted orders:"<<book.cancel_order(1)<<std::endl;
    
    
    

}