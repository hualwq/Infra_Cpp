# AI Compiler C++ Basics

这个目录用于学习 LLVM/MLIR 高频 C++ 基础。目标不是复刻 LLVM 源码，而是用足够小的 demo 先掌握它们背后的设计取舍：非拥有视图、小对象优化、arena 分配、LLVM 风格 RTTI、CRTP 静态多态，以及 MLIR 风格 IR 结构。

## 怎么运行

```bash
cmake -S . -B build
cmake --build build

./build/01_string_ref_array_ref
./build/02_small_vector
./build/03_bump_ptr_allocator
./build/04_isa_cast_dyn_cast
./build/05_crtp_visitor
./build/06_mlir_style_ir
./build/07_support_patterns
```

也可以单文件编译：

```bash
g++ -std=c++17 -Wall -Wextra -pedantic src/01_string_ref_array_ref.cpp -o /tmp/lesson01
/tmp/lesson01
```

## 学习顺序

| 顺序 | 文件 | LLVM/MLIR 中对应概念 | 你要真正掌握的点 |
| --- | --- | --- | --- |
| 1 | `src/01_string_ref_array_ref.cpp` | `llvm::StringRef`, `llvm::ArrayRef`, `llvm::MutableArrayRef` | 非拥有视图、生命周期、避免不必要拷贝 |
| 2 | `src/02_small_vector.cpp` | `llvm::SmallVector<T, N>` | 小对象优化、inline storage、什么时候避免堆分配 |
| 3 | `src/03_bump_ptr_allocator.cpp` | `llvm::BumpPtrAllocator`, `StringSaver`, arena allocation | 批量释放、placement new、IR 对象生命周期 |
| 4 | `src/04_isa_cast_dyn_cast.cpp` | `llvm::isa`, `cast`, `dyn_cast`, `classof` | LLVM 风格 RTTI、断言式向下转型、空指针处理 |
| 5 | `src/05_crtp_visitor.cpp` | CRTP visitor/mutator, pass 基类 | 静态多态、复用默认行为、减少虚函数开销 |
| 6 | `src/06_mlir_style_ir.cpp` | MLIR `Type/Attribute/Value/Operation/Block/Region` | SSA 值、operation 拥有关系、attribute/type uniquing 的直觉 |
| 7 | `src/07_support_patterns.cpp` | `llvm::function_ref`, `llvm::Twine`, `llvm::Expected` | 非拥有回调、临时字符串拼接、显式错误处理 |

## 为什么先学这些

LLVM/MLIR 代码不是普通业务 C++ 风格。它大量使用非拥有引用和 arena 生命周期管理，让 IR 构造、遍历和重写足够快；同时用 `isa/cast/dyn_cast` 和 `classof` 代替频繁的 `dynamic_cast`；再通过 CRTP、visitor、pattern rewrite 等模板技巧把通用遍历逻辑下沉到基类。

先把这些基础摸清，之后读 MLIR 里的 `Operation`、`Value`、`Type`、`Attribute`、`OpBuilder`、`PatternRewriter`、`Pass` 会轻松很多。

## 逐文件学习讲义

### 1. `src/01_string_ref_array_ref.cpp`

要解决的问题：

在 compiler 里，字符串、shape、operand list、attribute list 会被频繁传来传去。如果每次函数调用都复制 `std::string` 或 `std::vector`，IR 构建、解析、打印、pass 遍历都会产生大量无意义的内存分配和拷贝。

设计思路：

`StringRef` 和 `ArrayRef` 被设计成“非拥有 view”。它们只保存 `data pointer + size`，不负责分配、不负责释放，也不延长底层数据生命周期。这样函数参数可以统一写成 `StringRef` / `ArrayRef<T>`，既能接收 `std::string`、C 字符串、`std::vector`、C array，又避免复制。

跟读代码时看这几处：

1. `StringRef` 只有 `const char *data_` 和 `size_`，所以 `str()` 才会真的复制成 `std::string`。
2. `owned[4] = 'a'` 后，`opName` 的输出也变了，说明 `StringRef` 只是看同一块 buffer。
3. `dangerousReturn()` 是反例：返回指向局部 `std::string` 的 view，会悬空。
4. `dumpArray(ArrayRef<int> values, StringRef title)` 展示了 compiler API 常见写法：参数统一是 view，而不是具体容器类型。

学完应该形成的直觉：

看到 LLVM/MLIR API 接收 `StringRef`、`ArrayRef`，要立刻想到“它不拥有数据”。重点不是语法，而是生命周期：底层对象必须比 view 活得更久。

### 2. `src/02_small_vector.cpp`

要解决的问题：

IR 节点经常有“小数量列表”：一个 op 的 operands、results、successors、regions，很多时候只有 0 到 4 个元素。如果全部用 `std::vector`，哪怕只有 1 个元素也可能触发堆分配。compiler 内部这种小列表数量巨大，堆分配成本会被放大。

设计思路：

`SmallVector<T, N>` 把前 `N` 个元素直接放在对象内部的 inline storage 中。只有超过 `N` 时，才把已有元素 move 到堆上并扩容。它用更大的对象体积换取常见小规模场景下的零堆分配。

跟读代码时看这几处：

1. `Storage inline_[InlineN]` 是对象内部预留空间，最开始 `data_` 指向它。
2. `emplace_back` 用 placement new 在已有内存上构造元素。
3. 第 4 个元素仍然 `inline=1`，第 5 个元素触发 `grow()`，已有元素被 move 到 heap。
4. `sizeof(SmallVector<Operand, 4>)` 明显大于 `std::vector`，说明 `N` 不能随便设大。

学完应该形成的直觉：

`SmallVector<T, N>` 适合“多数情况下元素很少，偶尔变多”的 compiler 数据结构。`N` 是性能和对象大小之间的折中，不是越大越好。

### 3. `src/03_bump_ptr_allocator.cpp`

要解决的问题：

AST/IR 中会创建大量小对象和字符串。它们通常生命周期一致：整个 module、function、context 销毁时一起释放。如果每个对象都单独 `new/delete`，分配慢、碎片多、释放管理复杂。

设计思路：

`BumpPtrAllocator` 把内存分成大块 block，每次分配只做对齐并向前移动 `used` 指针。它不支持单独释放某个对象，最后通过 `reset()` 或 allocator 析构整批释放。`StringSaver` 则把字符串复制进 arena，确保字符串和 IR 对象有相同生命周期。

跟读代码时看这几处：

1. `allocate()` 中的 `alignTo()` 处理对齐，然后只更新 `block.used`。
2. `make<T>()` 用 placement new 把对象构造在 arena 返回的内存上。
3. `StringSaver::save()` 把字符串内容复制进 arena，而不是引用外部 `std::string`。
4. `arena.reset()` 后旧指针全部失效，这就是 arena 生命周期边界。

#### 完整分配过程示例

下面用一个具体 case 走完 `allocate()` 的完整逻辑。假设 `BlockSize = 4096`，连续做三次分配：

```cpp
BumpPtrAllocator arena;

void *p1 = arena.allocate(4000);   // ① 第一次分配
void *p2 = arena.allocate(200);    // ② 第二次分配
void *p3 = arena.allocate(500);    // ③ 第三次分配
```

**① 第一次分配 4000 字节：**

`blocks_` 为空 → 进入 `if` 分支，新建 `Block0`（`emplace_back(4096)`）：

```
blocks_[0] (Block0):
    bytes: [4096 字节的缓冲区，unique_ptr 管理]
    used:  4000     ← allocate 里 offset=0, used 更新为 0+4000
返回: block.bytes.get() + 0  → 指向 Block0 开头
```

**② 第二次分配 200 字节：**

`Block0` 剩余 `4096 - 4000 = 96` 字节，不够 200 → 进入 `if` 分支，新建 `Block1`：

```
blocks_[0] (Block0):
    bytes: [4096 字节]
    used:  4000     ← 已用满，不再动

blocks_[1] (Block1):  ← 新块
    bytes: [4096 字节]
    used:  200      ← offset=0, used 更新为 0+200
返回: Block1.bytes.get() + 0
```

**③ 第三次分配 500 字节：**

`Block1` 剩余 `4096 - 200 = 3896`，够用 → 不走 `if`，直接在 `Block1` 里 bump：

```
blocks_[1] (Block1):
    bytes: [4096 字节]
    used:  700      ← offset=200（对齐后仍是200，因为 align=1）, used 更新为 200+500
返回: Block1.bytes.get() + 200
```

**最终 `blocks_` 状态：**

```
blocks_ = [ Block0 , Block1 ]
            ↑           ↑
          用满了      用了 700/4096
```

**如果把第二次分配改成 5000 字节（超过 BlockSize）：**

```cpp
void *p2 = arena.allocate(5000);   // bytes=5000 > BlockSize
```

`newBlockSize = bytes > BlockSize ? bytes : BlockSize` → `newBlockSize = 5000`，新建的块大小为 5000 字节，而不是默认的 4096。这保证大对象不会被拒绝，也不会被硬塞进不够大的块里。

**`reset()` 时发生了什么：**

```cpp
arena.reset();   // → blocks_.clear()
```

`blocks_.clear()` 销毁每个 `Block` → 每个 `Block` 的 `unique_ptr<char[]>` 析构 → 每块缓冲区被释放。整批释放，没有逐个 free。

学完应该形成的直觉：

Arena allocation 的核心不是“自动内存管理”，而是“生命周期建模”。当一批对象天然一起生、一起死时，arena 非常高效；如果对象需要单独析构或单独释放，就不适合直接塞进 bump allocator。

### 4. `src/04_isa_cast_dyn_cast.cpp`

要解决的问题：

Compiler IR 是大量继承层级：`Expr` 下面有 `ConstantExpr`、`AddExpr`、`MulExpr`，MLIR/LLVM 里也到处是基类 handle 指向具体子类。频繁使用 C++ `dynamic_cast` 成本和风格都不理想，也不方便表达“这里必须是某类型”和“这里可能是某类型”的区别。

设计思路：

LLVM 风格 RTTI 给每个基类对象一个轻量 `Kind`，每个子类提供 `static bool classof(const Base *)`。模板函数 `isa<T>` 调用 `T::classof` 做判断；`dyn_cast<T>` 判断成功返回指针，失败返回 `nullptr`；`cast<T>` 判断失败直接 assert。

跟读代码时看这几处：

1. `Expr::Kind` 是运行时类型标签。
2. `ConstantExpr::classof` 只接受 `Kind::Constant`。
3. `BinaryExpr::classof` 同时接受 `Add` 和 `Mul`，说明 `classof` 可以表达类型范围。
4. `inspect()` 里先 `dyn_cast<ConstantExpr>`，否则判断 `isa<BinaryExpr>`。
5. `cast<ConstantExpr>(first)` 表示调用者确信 `first` 一定是 constant。

学完应该形成的直觉：

`cast<T>` 是带断言的承诺，适合不变量已经保证的路径；`dyn_cast<T>` 是分支判断工具，适合处理多态输入。读 LLVM/MLIR 源码时，这两个选择本身就透露了作者对类型不变量的判断。

### 5. `src/05_crtp_visitor.cpp`

要解决的问题：

IR 遍历代码很容易重复：每种 visitor 都需要按节点类型分发，再递归访问子节点。如果全部写虚函数，样板代码多；如果每个 visitor 自己写 switch，又容易遗漏节点和递归逻辑。

设计思路：

CRTP visitor 把通用分发和默认递归放进 `ExprVisitor<Derived>`。基类通过 `static_cast<Derived *>(this)` 调用派生类实现。派生类只覆盖自己关心的 `visitConstant`、`visitAdd`、`visitMul`，没覆盖的就使用基类默认行为。

跟读代码时看这几处：

1. `ExprVisitor<Derived>::visit()` 是统一入口，按 `Kind` 分发。
2. `derived()` 是 CRTP 的核心，把基类 `this` 转成派生类引用。
3. `walkBinary()` 里调用 `derived().visit(...)`，所以递归时仍然走派生类逻辑。
4. `PrintVisitor` 关心打印缩进，`CostVisitor` 关心代价统计，但都复用了同一套遍历框架。

学完应该形成的直觉：

CRTP 的价值是”在编译期复用通用算法，同时把定制点留给派生类”。MLIR/LLVM 中很多 visitor、pass、mixin 都是在用这个思路减少样板和虚调用。

#### CRTP 详解：为什么会有 `ExprVisitor<PrintVisitor>` 这种写法

**CRTP（Curiously Recurring Template Pattern，奇异递归模板模式）** 的核心语法是：**派生类把自己作为模板参数传给基类**。

```cpp
class PrintVisitor : public ExprVisitor<PrintVisitor>
//                              ^^^^^^^^^^^^^^^^^^^^
//                              基类模板参数 = 派生类自身
```

##### 为什么需要这种写法？

传统虚函数多态在**运行时**通过查虚函数表（vtable）分发，有间接调用开销：

```cpp
class Base     { virtual void visit() = 0; };
class Derived : Base { void visit() override { /* ... */ } };

Base* p = new Derived;
p->visit();   // 运行时查 vtable，有开销
```

CRTP 用**模板**，让基类在**编译期**就知道派生类的类型，通过 `static_cast<Derived*>(this)` 直接调用，零运行时开销：

```cpp
template <typename Derived>
class ExprVisitor {
protected:
    Derived& derived() {
        return *static_cast<Derived*>(this);  // 编译期转型，0 开销
    }
public:
    void visit(Expr* expr) {
        switch (expr->kind()) {
        case Constant:
            derived().visitConstant(...);  // 编译期直接调用 PrintVisitor::visitConstant，可内联
            break;
        case Add:
            derived().visitAdd(...);
            break;
        }
    }
};
```

##### 模板参数 `To` / `From` 的类比理解

CRTP 里的 `Derived` 和 `isa` 里的 `To` / `From` 是同一个思路——**模板参数代表类型转换的两端**：

```cpp
// isa 里：From 由编译器自动推导，To 由你显式指定
template <typename To, typename From>
bool isa(const From *value);

isa<ConstantExpr>(e);
//   ↑To          ↑From 由 e 的类型自动推导

// CRTP 里：Derived 由你显式指定，基类在编译期拿到它
template <typename Derived>
class ExprVisitor { ... };

class PrintVisitor : public ExprVisitor<PrintVisitor>;
//                                    ↑Derived = PrintVisitor，编译期已知
```

##### CRTP vs 虚函数多态对比

| | 虚函数 | CRTP |
|---|---|---|
| 分发时机 | 运行时查 vtable | 编译期确定，可内联 |
| 性能 | 有间接调用开销 | 零开销（static_cast 编译期决议） |
| 灵活性 | 可以放入容器中统一管理（通过基类指针） | 每个派生类是独立类型，不能放同一个容器 |
| 语法 | 自然直观 | 初看怪异（`class A : Base<A>`） |
| 适用场景 | 需要运行时多态、对象需要放入容器 | 性能敏感路径、框架骨架代码 |

##### 为什么 LLVM/MLIR 大量使用 CRTP？

LLVM/MLIR 的 IR 遍历（visitor、pass、pattern rewrite）是**性能敏感路径**，而且 visitor 的种类在编译期就是确定的（不会在运行时动态增减 visitor 类型）。CRTP 正好满足这两个需求：

1. **编译期多态 + 零开销抽象** — visitor 的 `visitXxx` 调用可以被内联，没有 vtable 间接跳转
2. **代码复用** — 通用分发逻辑（`visit` 按 Kind 分发、`walkBinary` 递归）写在基类模板里，所有 visitor 复用，派生类只覆盖关心的节点

这就是 `PrintVisitor` 和 `CostVisitor` 能共享同一套 `ExprVisitor` 遍历框架，又各自定制行为的原因。

### 6. `src/06_mlir_style_ir.cpp`

要解决的问题：

学习 MLIR 前，必须先建立 IR 结构的基本模型：一个 op 有名字、operands、results、attributes；`Value` 是 SSA 值；`Block` 持有 operation 序列；`Type/Attribute` 往往由 `Context` 去重管理。如果没有这个模型，直接读 MLIR 的 `Operation`、`Value`、`OpBuilder` 会很抽象。

设计思路：

这个 demo 构建了一个极简 MLIR 骨架：`Context` 负责字符串 intern，模拟 type/attribute uniquing；`Type` 和 `Attribute` 是轻量 handle；`Operation` 拥有 operands/results/attrs；`Value` 只是指向 defining op 和 result index；`Block` 拥有 operations；`OpBuilder` 统一创建类型、属性和 op。

跟读代码时看这几处：

1. `Context::intern()` 保证相同字符串返回同一个底层指针，模拟 uniquing。
2. `Type` 只保存 `const std::string *`，所以比较可以直接比较指针。
3. `Operation` 构造时根据 `resultTypes` 创建 `Value{this, i, type}`。
4. `Value::name()` 通过 owner operation 和 result index 生成 SSA 名字。
5. `Block::addOperation()` 用 `unique_ptr<Operation>` 表示 block 拥有 op。
6. `builder.create("toy.matmul", {lhs, rhs}, {tensor}, attrs)` 模拟 MLIR `OpBuilder` 的使用方式。

学完应该形成的直觉：

MLIR 里很多对象是 handle，不一定拥有底层数据。读 IR 相关源码时要不断追问：这个对象是谁创建的？谁拥有它？它只是引用一个 operation/result/type，还是负责释放资源？

### 7. `src/07_support_patterns.cpp`

要解决的问题：

LLVM Support 里有很多“小工具类型”，它们不是 IR 核心，但源码里出现频率很高。比如短生命周期回调用 `function_ref`，临时字符串拼接用 `Twine`，可能失败的函数返回 `Expected<T>`。如果不了解它们的设计动机，读源码时会误以为它们只是普通 `std::function`、`std::string` 或异常替代品。

设计思路：

`FunctionRefInt` 只保存 callable 的地址和一个调用 trampoline，不拥有 lambda；`TwineLike` 保存两个 `string_view`，只适合立即转成 `std::string`；`Expected<T>` 用“要么有值、要么有错误字符串”的方式强迫调用者显式处理失败。

跟读代码时看这几处：

1. `FunctionRefInt(Callable &callable)` 接收的是引用，说明它不复制、不拥有 callable。
2. `walkNumbers()` 适合立刻调用回调，不适合把 `FunctionRefInt` 存到成员变量里。
3. `TwineLike` 内部是 `string_view`，所以不能绑定到马上销毁的临时字符串。
4. `parsePositiveInt()` 失败时返回错误，成功时返回值；调用处必须 `if (parsed)` 判断。

学完应该形成的直觉：

LLVM 很多工具类型都在刻意避免隐藏成本：不轻易分配、不轻易复制、不用异常隐式跳转。它们要求调用者理解生命周期和错误处理边界，这也是读 compiler C++ 必须适应的风格。

## 重点提醒

1. `StringRef` / `ArrayRef` 不拥有数据。不要返回指向局部变量的 view。
2. `SmallVector<T, N>` 的 `N` 不是越大越好。它会增加对象自身大小，适合“多数时候元素很少”的场景。
3. `BumpPtrAllocator` 适合“一批对象一起死”的 IR、AST、字符串驻留场景，不适合需要单独析构/释放的业务对象。
4. `cast<T>` 表示“我确信就是 T”，错了应当直接暴露 bug；`dyn_cast<T>` 表示“可能是 T”，需要检查返回值。
5. CRTP 的核心是 `static_cast<Derived *>(this)`，让基类在编译期调用派生类能力。
6. MLIR 的很多对象是非拥有 handle。看源码时要持续问：谁拥有内存？谁只是引用？

## 后续可以继续补的主题

- `llvm::DenseMap` / `DenseSet`：LLVM 高频哈希容器。
- MLIR `PatternRewriter`：局部 IR rewrite 的入口。
- MLIR TableGen / ODS：定义 dialect 和 op 的核心工具链。

## 官方资料入口

- LLVM Programmer's Manual: https://llvm.org/docs/ProgrammersManual.html
- LLVM `BumpPtrAllocator` 源码文档: https://llvm.org/doxygen/classllvm_1_1BumpPtrAllocatorImpl.html
- MLIR Tutorial: https://mlir.llvm.org/docs/Tutorials/
