#include <iostream>
#include "../src/order.hpp"
#include "../src/price_level.hpp"
std::string side_to_str(Side s) {
    switch (s) {
        case Side::BUY:  return "BUY";
        case Side::SELL: return "SELL";
    }
    return "UNKNOWN";
}
std::string type_to_str(OrderType t) {
    switch (t) {
        case OrderType::LIMIT:  return "LIMIT";
        case OrderType::MARKET: return "MARKET";
    }
    return "UNKNOWN";
}
int main(){
    Order o = Order(1,10,400,100,Side::BUY,OrderType::LIMIT,2026429);
    //枚举类必须带作用域
    PriceLevel p(10);
    auto it = p.add_order(o);
    //& 在类型声明中（const Order&）表示"引用"
    //& 在表达式中（&o）表示"取地址，得到指针"
    std::cout<<"iterator for Order shows price:"<<it->price<<std::endl;
    std::cout<<"iterator for Order show quantity:"<<it->quantity<<std::endl;
    std::cout << "side: " << side_to_str(it->side) << std::endl;
    std::cout << "type: " << type_to_str(it->type) << std::endl;
    std::cout<<"iterator for Order shows timestamp:"<<it->timestamp<<std::endl;
}