// types.hpp
#pragma once //作用：防止头文件被重复包含（include 多次）
//等价于：#ifndef TYPES_HPP
//       #define TYPES_HPP
#include <cstdint>//提供精确位宽的整数类型，例如uint64_t
#include <limits>//提供各种类型的最大值 / 最小值：std::numeric_limits<int>::max();std::numeric_limits<double>::min();
#include <chrono>       
using OrderID = uint64_t;//定义类型别名
using Price = int64_t;//以tick为单位，10050表示100.50元（tick = 0.01）
using Quantity = uint32_t;
using Timestamp = uint64_t;
/* 三种定义方式：
typedef 原类型 别名;
#define 别名 原类型
using 别名 = 原类型;
 */

constexpr Price INVALID_PRICE = std::numeric_limits<Price>::max();//返回 int64_t 的最大值
//std（名字空间）::numeric_limits（类）<Price>（模板参数）::max()（静态函数）
enum class Side : uint8_t{//enum class（强类型枚举）
    BUY = 0 ,
    SELL = 1
};
enum class OrderType:uint8_t{
    LIMIT = 0,
    MARKET = 1
};
enum class OrderStatus:uint8_t{
    ACTIVE,FILLED,PARTIALLY_FILLED,CANCELLED
};//cpp自动分配0，1，2，3，4
//名称有命名空间，Side::BUY
//Side::SELL

inline Timestamp get_timestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}