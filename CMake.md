# CMake 入门：从 g++ 到现代化的 C++ 构建工具

---

## 1. 前置知识

这篇文章假设你已经掌握了以下内容：

| 知识点 | 说明 |
|--------|------|
| **头文件与多文件编译** | 一个项目拆成多个 `.cpp` 和 `.h`，用 `#include` 组合 |
| **g++ 基本用法** | 至少见过 `g++ main.cpp -o app` |

---

## 2. C++ 程序运行前的全过程

很多人以为 `g++ main.cpp -o app` 是一个"魔法命令"，其实它在背后干了四件事。我们可以一层层拆开看。

假设有一个最简单的程序：

```cpp
// hello.h
int greet();

// hello.cpp
#include <iostream>
int greet() {
    return 42;
}

// main.cpp
#include "hello.h"
int main() {
    return greet();
}
```

### ① 预处理（Preprocessing）

预处理器处理所有 `#` 开头的指令：`#include` 把整个头文件的内容粘贴进来，`#define` 做宏替换。

```bash
g++ -E main.cpp -o main.i
```

打开 `main.i` 你会发现它变成了几百行——`<iostream>` 的内容全部展开了。

### ② 编译（Compilation）

把预处理后的代码翻译成**汇编语言**（人类可读的机器指令助记符）。

```bash
g++ -S main.i -o main.s
```

`main.s` 里是类似 `movl $42, %eax` 的汇编代码。

### ③ 汇编（Assembly）

把汇编代码转成**机器码**——也就是 **.o 文件**（目标文件）。

```bash
g++ -c main.s -o main.o
```

此时 `main.o` 是二进制文件，用 `xxd` 能看见一堆十六进制。

### ④ 链接（Linking）

把多个 `.o` 文件、以及 C++ 标准库的代码拼在一起，生成最终的可执行文件。

```bash
g++ main.o hello.o -o app
```

这一步解决"符号解析"问题：`main.o` 里调用了 `greet()`，链接器去 `hello.o` 里找到它的定义，填入正确的地址。

**总结一条命令 = 四个阶段：**

```
main.cpp ──预处理──▶ main.i ──编译──▶ main.s ──汇编──▶ main.o ──┐
hello.cpp─预处理──▶hello.i──编译──▶hello.s──汇编──▶hello.o─┤
                                                               ├──链接──▶ app
                                                    libstdc++ ──┘
```

---

## 3. 静态库 vs 动态库

链接阶段我们可以选择"打包方式"。

### 静态库（`.a`，Linux / `.lib`，Windows）

把一组 `.o` 文件打包成一个归档文件。链接时，链接器会把**用到的部分直接拷贝**进最终的可执行文件。

```bash
# 制作静态库
ar rcs libhello.a hello.o

# 链接静态库
g++ main.o -L. -lhello -o app_static
```

### 动态库（`.so`，Linux / `.dll`，Windows）

链接时只记录"这个函数在哪个动态库里"，不拷贝代码。程序运行时才加载。

```bash
# 制作动态库
g++ -fPIC -shared hello.cpp -o libhello.so

# 链接动态库（链接阶段只做符号检查，不拷贝）
g++ main.o -L. -lhello -o app_dynamic

# 运行前需要告诉系统去哪找 .so
export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
./app_dynamic
```

### 对比

| 特性 | 静态库 | 动态库 |
|------|--------|--------|
| 体积 | exe 变大 | exe 小，但需要带 .so |
| 部署 | 一个文件搞定 | 要保证 .so 也在 |
| 更新 | 重新编译整个 exe | 替换 .so 即可（接口不变） |
| 内存 | 每个进程一份 | 多进程共享一份代码段 |

### 验证方法

```bash
# 查看可执行文件依赖了哪些动态库
ldd app_dynamic

# 查看文件类型和段信息
readelf -h app_static
readelf -d app_dynamic
```

---

## 4. 为什么需要 CMake？

### 阶段一：手敲 g++

项目只有 2 个文件时，敲 `g++ main.cpp hello.cpp -o app` 还凑合。

### 阶段二：写 Makefile

项目变成 20 个文件后，每次编译不想重敲所有文件：

```makefile
# Makefile
CXX = g++
CXXFLAGS = -std=c++17 -Wall

app: main.o hello.o world.o utils.o
	$(CXX) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o app
```

痛点开始暴露：

- **跨平台**：Linux 上用 `g++`，Windows 上用 `cl.exe`，Mac 上可能是 `clang++`——Makefile 不通用。
- **找第三方库**：`-I` 包含路径、`-L` 库路径、`-l` 库名全要手写，换台机器路径就变了。
- **条件编译**：Debug/Release 不同参数、要不要开启某特性——Makefile 的语法晦涩难懂。

### 阶段三：CMake 登场

CMake 说：**你只告诉我"项目有哪些源文件、要链接哪些库、目标是什么"，其余的（编译器、路径、命令行参数）我来帮你搞定。**

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MyApp)

add_executable(app main.cpp hello.cpp world.cpp utils.cpp)
target_compile_features(app PRIVATE cxx_std_17)
```

简单、清晰、跨平台。

---

## 5. CMake 是生成器：配置与构建两阶段

CMake 最容易被误解的一点：**CMake 本身不编译代码**。

一个厨房的类比：

| 角色 | 比喻 |
|------|------|
| 你写 `CMakeLists.txt` | 写菜谱 |
| `cmake -S . -B build`（配置） | 根据菜谱列采购清单、选工具 |
| `make` / `ninja`（构建） | 按清单切菜、炒菜 |
| 最终的可执行文件 | 端上桌的菜 |

CMake 是**元构建系统（meta-build system）**——它根据你的 `CMakeLists.txt` 和当前平台，生成对应的构建文件（Makefile 或 Ninja.build 或 Visual Studio 的 `.sln`）。

### 实际动手

```bash
# 第一步：配置阶段（生成 Makefile）
cmake -S . -B build

# 第二步：构建阶段（执行编译）
cmake --build build

# 也可以直接 make
cd build && make
```

配置阶段会发生：

1. 检测编译器（g++? clang++? MSVC?）
2. 检查依赖库是否存在
3. 生成 `build/Makefile`
4. 缓存所有检测结果到 `CMakeCache.txt`

构建阶段就是 `make` 按规则执行 g++ 命令。

### 一次配置，多次构建

```bash
# 改源码后只需要重新构建
cmake --build build         # 增量编译，只重新编译改动的文件

# 要切换 Debug 模式？
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build_debug
```

---

## 6. 常见的 Property

CMake 通过"目标属性（target properties）"来控制编译行为。以下是五个最常用的：

### **target_include_directories** — 指定头文件搜索路径

```cmake
target_include_directories(app PRIVATE include/)
```

等价于 `g++ -Iinclude/`，告诉编译器去哪找 `#include "xxx.h"`。

### **target_link_libraries** — 指定要链接的库

```cmake
target_link_libraries(app PRIVATE fmt pthread)
```

等价于 `g++ ... -lfmt -lpthread`。

### **target_compile_definitions** — 定义预处理器宏

```cmake
target_compile_definitions(app PRIVATE USE_OPENSSL=1)
```

等价于 `g++ -DUSE_OPENSSL=1`，可以在代码中用 `#ifdef USE_OPENSSL` 做条件编译。

### **target_compile_features** — 控制 C++ 标准

```cmake
target_compile_features(app PRIVATE cxx_std_17)
```

等价于 `g++ -std=c++17`。用 property 的好处是：CMake 会自动检查编译器是否支持该特性，不支持则报错。

### **CMAKE_CXX_STANDARD** — 全局设定 C++ 标准

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

全局设置所有目标默认用 C++17。与 `target_compile_features` 二选一即可，推荐后者——更精确。

---

## 7. PUBLIC / PRIVATE / INTERFACE

这三个关键字控制"属性传播范围"，是 CMake 里最核心也最容易混淆的概念。

### 用一句话记

- **PRIVATE**：我自己的实现需要，别人不需要知道
- **PUBLIC**：我需要的，别人也需要
- **INTERFACE**：我不需要，但用我的人需要

### 三层依赖的例子

假设有三个库：

```
        app
         │
         ▼
      lib_math       ← 提供 add()，内部用了 fmt 打印日志
         │
         ▼
      lib_core       ← 提供 Vector 类型
```

#### lib_core：最底层，不需要依赖别人

```cmake
add_library(lib_core STATIC core.cpp)
target_include_directories(lib_core INTERFACE include_core/)
```

用 `INTERFACE` 表示：lib_core 自己编译**不需要**这个头文件路径（它自己的源文件可能不需要），但**链接 lib_core 的人**需要。

#### lib_math：依赖 lib_core，内部用了 fmt

```cmake
add_library(lib_math STATIC math.cpp)
target_link_libraries(lib_math
    PUBLIC    lib_core      # Vector 类型暴露在 math.h 里，所以用我的人也要有
    PRIVATE   fmt           # fmt 只用在 math.cpp 内部，外部不需要知道
)
```

推理逻辑：

1. `lib_math` 的 `math.h` 中 `#include "core.h"`（用了 `Vector`） → **PUBLIC**
2. `lib_math` 的 `math.cpp` 中用 `fmt::print()` 打日志 → **PRIVATE**

#### app：只用 lib_math

```cmake
add_executable(app main.cpp)
target_link_libraries(app PRIVATE lib_math)
```

CMake 会自动传递：

- `app` 能 `#include "math.h"` ✅（lib_math 的 PUBLIC 头文件路径）
- `app` 能 `#include "core.h"` ✅（因为 lib_core 通过 PUBLIC 传递过来了）
- `app` 不需要知道 fmt ❌（PRIVATE 不传递）

### 传播规则一览

| 当前目标设置的属性 | 自己可见？ | 链接我的人可见？ | 再下游可见？ |
|-------------------|-----------|----------------|------------|
| **PRIVATE** | ✅ | ❌ | ❌ |
| **PUBLIC** | ✅ | ✅ | ❌（仅一层） |
| **INTERFACE** | ❌ | ✅ | ❌（仅一层） |

> INTERFACE 是一层传递，不是无限传递。PUBLIC 传递到直接使用者就停了，不会再往上传。

---

## 总结与思维导图

```
CMake 入门知识体系
│
├── 1. 前置知识：函数 / 头文件 / 多文件编译 / 命令行
│
├── 2. 编译四阶段
│    预处理 (-E) → 编译 (-S) → 汇编 (-c) → 链接
│
├── 3. 静态库 vs 动态库
│    .a → 打包进 exe，体积大，部署简单
│    .so → 运行时加载，体积小，更新灵活
│
├── 4. 为什么 CMake
│    g++ → Makefile（可维护差、跨平台难）→ CMake
│
├── 5. CMake = 生成器
│    配置 (cmake -S . -B build) → 构建 (cmake --build build)
│
├── 6. 常用属性
│    target_include_directories
│    target_link_libraries
│    target_compile_definitions
│    target_compile_features
│    CMAKE_CXX_STANDARD
│
└── 7. PUBLIC / PRIVATE / INTERFACE
     PRIVATE:  我自己用
     PUBLIC:   我和别人都用
     INTERFACE: 只用别人用
```

### 下一步推荐

- 读一遍官方文档 `cmake --help-command-list`
- 实战：把你的一个小项目从 g++ 改写成 CMake
- 学会用 `find_package` 找第三方库（OpenCV、Boost 等）
- 了解 Modern CMake 的"目标导向"设计哲学
