# Learn Cpp Infra Basics

这个仓库是学习 compiler infra 和现代 C++ 基础知识时整理的一组小 demo。内容主要围绕三块：AI compiler 相关 C++ 设计模式、CMake 工程组织方式，以及常见 C++ 语言特性。

仓库里的代码不追求做成完整项目，重点是把每个知识点拆成可以单独编译、单独阅读的小例子，方便在学习 LLVM/MLIR、TVM 或其他 infra 项目源码时建立直觉。

## 目录概览

### `AIcompiler/`

这一部分主要是面向 LLVM/MLIR 风格代码的 C++ 基础练习。内容包括非拥有视图、SmallVector、小对象优化、arena/bump allocator、LLVM 风格 RTTI、CRTP visitor、MLIR 风格 IR 结构，以及一些支撑 compiler infra 的常见工具模式。

这些例子适合在读 LLVM/MLIR 代码前先过一遍，重点理解 compiler 项目里为什么大量使用 `StringRef`、`ArrayRef`、`SmallVector`、`isa/cast/dyn_cast`、visitor 和 arena 生命周期管理。

### `Cmake/`

这一部分是 CMake 工程组织的基础练习，按 case 拆分不同场景。内容覆盖最基础的可执行文件和库构建、头文件 include 作用域、库之间的链接依赖、interface library、static library、shared library，以及 `PUBLIC` / `PRIVATE` / `INTERFACE` 在真实 target 依赖中的传递规则。

这些 demo 主要帮助理解现代 CMake 的 target-based 写法：不要只把 CMake 当成编译脚本，而是把每个库、可执行文件和依赖关系都建模成 target。

### `Fun_Cpp/`

这一部分整理的是更通用的 C++ 语言基础和常见技巧，包括 lambda、虚函数、多态、智能指针、模板、TVM object system 的简化理解、visitor 分发以及宏相关用法。

这些例子更偏语言特性本身，适合作为阅读 infra 源码前的补充。很多大型 C++ 项目会把模板、宏、多态、对象系统和 visitor 混合使用，先用小 demo 看清每个机制，会更容易理解真实项目里的抽象。

## 推荐学习顺序

1. 先看 `Fun_Cpp/`，补齐 C++ 语言特性和常见写法。
2. 再看 `Cmake/`，理解代码如何被组织、编译和链接。
3. 最后看 `AIcompiler/`，把 C++ 机制放到 compiler infra 的语境里理解。

## 构建方式

部分目录可以单独进入后使用 CMake 构建，例如：

```bash
cd AIcompiler
cmake -S . -B build
cmake --build build
```

`Cmake/` 目录下的 case 也可以按子目录分别阅读和构建。`Fun_Cpp/` 中的文件更偏单文件示例，可以直接用 `g++` 或 `clang++` 编译运行。
