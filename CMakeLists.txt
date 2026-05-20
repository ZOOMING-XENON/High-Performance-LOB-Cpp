cmake_minimum_required(VERSION 3.15)
project(LOB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 把 src/ 下所有的 .cpp 编译成一个公共库
file(GLOB SRC_FILES src/*.cpp)
add_library(lob_lib ${SRC_FILES})
target_include_directories(lob_lib PUBLIC src/)

# 把 tests/ 下每个 .cpp 自动变成一个可执行文件
file(GLOB TEST_FILES tests/*.cpp)
foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} lob_lib)
endforeach()

# benchmarks/ 下每个 .cpp 自动变成一个 Google Benchmark 可执行文件
# 仅当系统装有 google-benchmark（brew install google-benchmark）时才启用，
# 避免没装 benchmark 的环境直接报错。
find_package(benchmark QUIET)
if(benchmark_FOUND)
    file(GLOB BENCH_FILES benchmarks/*.cpp)
    foreach(BENCH_FILE ${BENCH_FILES})
        get_filename_component(BENCH_NAME ${BENCH_FILE} NAME_WE)
        add_executable(${BENCH_NAME} ${BENCH_FILE})
        target_link_libraries(${BENCH_NAME} lob_lib benchmark::benchmark)
    endforeach()
    message(STATUS "google-benchmark found: ${benchmark_DIR}")
else()
    message(STATUS "google-benchmark NOT found, skipping benchmarks/ targets. "
                   "Install with: brew install google-benchmark")
endif()