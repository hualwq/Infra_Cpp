# Learn Cpp Infra Basics

这个仓库是学习 compiler infra 和现代 C++ 基础知识时整理的一组小 demo。内容主要围绕三块：编译器相关 C++ 设计模式、CMake 工程组织方式、常见 C++ 语言特性，以及 C++ 面试常见问题。

仓库里的代码不追求做成完整项目，重点是把每个知识点拆成可以单独编译、单独阅读的小例子，方便在学习 LLVM/MLIR、TVM 或其他 infra 项目源码时建立直觉。

## 目录概览

### `Cpp_prim/`

这一部分主要是面向 LLVM/MLIR 风格代码的 C++ 基础练习。内容包括非拥有视图、SmallVector、小对象优化、arena/bump allocator、LLVM 风格 RTTI、CRTP visitor、MLIR 风格 IR 结构，以及一些支撑 compiler infra 的常见工具模式。

这些例子适合在读 LLVM/MLIR 代码前先过一遍，重点理解 compiler 项目里为什么大量使用 `StringRef`、`ArrayRef`、`SmallVector`、`isa/cast/dyn_cast`、visitor 和 arena 生命周期管理。

**包含示例：**
- `01_string_ref_array_ref.cpp` - 非拥有视图（StringRef/ArrayRef）
- `02_small_vector.cpp` - 小对象优化（SmallVector）
- `03_bump_ptr_allocator.cpp` - Arena 分配器
- `04_isa_cast_dyn_cast.cpp` - LLVM 风格 RTTI
- `05_crtp_visitor.cpp` - CRTP 静态多态与 Visitor 模式
- `06_mlir_style_ir.cpp` - MLIR 风格 IR 结构
- `07_support_patterns.cpp` - LLVM Support 工具模式

详细学习笔记见：`Cpp_prim/ReadMe.md`

### `Fun_Cpp/`

这一部分整理的是更通用的 C++ 语言基础和常见技巧，包括 lambda、虚函数、多态、智能指针、模板、TVM object system 的简化理解、visitor 分发以及宏相关用法。

这些例子更偏语言特性本身，适合作为阅读 infra 源码前的补充。很多大型 C++ 项目会把模板、宏、多态、对象系统和 visitor 混合使用，先用小 demo 看清每个机制，会更容易理解真实项目里的抽象。

**包含示例：**
- `1_lambda.cpp` - Lambda 表达式
- `2_virtualFunc.cpp` - 虚函数与多态
- `3_smart_ptr.cpp` - 智能指针（unique_ptr/shared_ptr/weak_ptr）
- `4_templates.cpp` - 模板编程
- `5_tvm_object_system.cpp` - TVM Object System 简化理解
- `6_defile_visitor.cpp` - Visitor 模式详解
- `7_macros.cpp` - 宏相关用法
- `8_function.cpp` - 函数相关特性

### `Fun_compiler/`

一个迷你的 AI 编译器流水线 demo，展示从计算图定义到编译执行的基本过程。包含：

- **计算图 IR** - 定义算子（Mul/Add/Relu）和数据流
- **编译器 Pass** - 算子融合优化（fuse Mul+Add+Relu）
- **代码生成** -  lowering 到 kernel 函数
- **执行模式** - AOT（提前编译）和 JIT（即时编译）两种模式

这个 demo 适合理解 AI 编译器的核心概念：计算图表示、图优化、代码生成。

### `Cmake/`

这一部分是 CMake 工程组织的基础练习，按 case 拆分不同场景。内容覆盖最基础的可执行文件和库构建、头文件 include 作用域、库之间的链接依赖、interface library、static library、shared library，以及 `PUBLIC` / `PRIVATE` / `INTERFACE` 在真实 target 依赖中的传递规则。

这些 demo 主要帮助理解现代 CMake 的 target-based 写法：不要只把 CMake 当成编译脚本，而是把每个库、可执行文件和依赖关系都建模成 target。

**包含案例：**
- `case1_hello_library` - 基础库和可执行文件构建
- `case2_include_scope` - 头文件包含作用域
- `case3_link_chain` - 库链接依赖链
- `case4_interface_lib` - Interface 库
- `case5_static_library` - 静态库
- `case6_shared_library` - 动态库
- `case7_public_private_interface` - PUBLIC/PRIVATE/INTERFACE 传递规则

### `interview/`

C++ 面试常见问题整理，包含代码示例和详细讲解：

- **智能指针** - unique_ptr/shared_ptr/weak_ptr 的原理和使用场景
- **类型转换** - static_cast/dynamic_cast/const_cast/reinterpret_cast 对比
- **虚函数机制** - 虚函数表、多态实现原理
- **内存管理** - 堆与栈的区别、new/delete vs malloc/free
- **Move 语义** - 右值引用、std::move、完美转发

详细面试笔记见：`interview/interview.md`

## 推荐学习顺序

1. 先看 `Fun_Cpp/`，补齐 C++ 语言特性和常见写法
2. 再看 `Cmake/`，理解代码如何被组织、编译和链接
3. 接着看 `Cpp_prim/`，把 C++ 机制放到 compiler infra 的语境里理解
4. 最后看 `Fun_compiler/`，理解这些知识如何组合成一个简单的编译器
5. `interview/` 可以在任何阶段作为查漏补缺的参考

## 构建方式

### 根目录构建（构建所有子项目）

```bash
mkdir -p build && cd build
cmake ..
make
```

### 单独构建子项目

**Cpp_prim/：**
```bash
cd Cpp_prim
cmake -S . -B build
cmake --build build

# 运行示例
./build/01_string_ref_array_ref
./build/02_small_vector
# ...
```

**Fun_compiler/：**
```bash
cd Fun_compiler
cmake -S . -B build
cmake --build build
./build/fun_compiler
```

**Cmake/ 案例：**
```bash
cd Cmake/case1_hello_library
cmake -S . -B build
cmake --build build
```

**Fun_Cpp/ 和 interview/：**
这些目录中的文件更偏单文件示例，可以直接用 `g++` 或 `clang++` 编译运行：
```bash
g++ -std=c++17 -Wall -Wextra -pedantic 1_lambda.cpp -o lambda
./lambda
```

## 依赖环境

- C++ 编译器：支持 C++17 标准（g++ 7+ 或 clang++ 5+）
- CMake：3.10 及以上版本
- 构建工具：Make 或 Ninja

## 参考资料

- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual.html)
- [MLIR Tutorial](https://mlir.llvm.org/docs/Tutorials/)
- [CMake Documentation](https://cmake.org/documentation/)
- [C++ Reference](https://en.cppreference.com/)

## 贡献

这个仓库是个人学习笔记，欢迎提出建议和修正。如有问题，请提交 issue 或 pull request。

## License

MIT License
