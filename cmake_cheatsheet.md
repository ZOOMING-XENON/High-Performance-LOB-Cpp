# CMake 日常命令速查

## 项目结构

```
LOB/
├── CMakeLists.txt       # 构建配置（根目录）
├── build/               # 所有编译产物，不要手动改这里面的东西
├── src/
│   ├── order_book.cpp
│   └── *.hpp
└── tests/
    └── *.cpp            # 每个 .cpp 自动生成一个可执行文件
```

---

## 第一次使用 / CMakeLists.txt 改动后

```bash
# 在项目根目录执行
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

| 参数 | 含义 |
|---|---|
| `-S .` | 源码目录是当前目录（有 CMakeLists.txt 的地方） |
| `-B build` | 把所有编译产物放到 `build/` 目录 |
| `-DCMAKE_BUILD_TYPE=Debug` | Debug 模式，保留调试信息 |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | 生成 `compile_commands.json`，让 VSCode IntelliSense 消除假红线 |

---

## 日常编译

```bash
# 编译全部目标（最常用）
cmake --build build

# 加速：多核并行编译
cmake --build build --parallel

# 只编译某一个目标
cmake --build build --target test_order_book_stage2
```

---

## 运行可执行文件

编译产物都在 `build/` 下：

```bash
./build/test_order_book
./build/test_order_book_stage2
```

---

## 切换 Debug / Release 模式

```bash
# Debug（默认，有调试符号，适合开发）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Release（开启优化，适合性能测试）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## 清理重新编译

```bash
# 只清理编译产物（不删 CMake 配置）
cmake --build build --target clean

# 彻底清理（删掉整个 build 目录，等同于"完全重置"）
rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build --parallel
```

---

## 新增 .cpp 文件后需要做什么？

本项目的 `CMakeLists.txt` 使用了 `file(GLOB ...)` 自动扫描，
所以新增文件后**只需要重新 Configure 一次**，不需要手动改 `CMakeLists.txt`：

```bash
cmake -S . -B build   # 重新扫描文件
cmake --build build --parallel
```

---

## 常见错误速查

| 错误信息 | 原因 | 解决 |
|---|---|---|
| `Undefined symbols` | 链接时找不到函数实现，通常是 .cpp 没参与编译 | 用 CMake 编译，别用 `g++ 单个文件` |
| `invalid digit '8' in octal constant` | OrderID 写成了 `000008`（八进制） | 去掉前导零，写 `8` |
| `No such file or directory` | 头文件路径找不到 | 检查 `CMakeLists.txt` 里 `target_include_directories` |
| `cmake: command not found` | CMake 没安装 | `brew install cmake` |

---

## VSCode 快捷键对应

| 操作 | 快捷键 |
|---|---|
| 编译（CMake Build task） | `Cmd + Shift + B` |
| 查看所有 task | `Cmd + Shift + P` → `Tasks: Run Task` |
