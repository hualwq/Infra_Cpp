# Fun Compiler

这是一个可运行的最小 AI 编译器 demo。它不依赖 CUDA、LLVM、TVM 或 PyTorch；目标是把 AI 编译器的主干流程缩小到一个可以完整读完的 C++ 文件中。

Demo 编译并运行下面的张量表达式：

```text
y = relu(x * weight + bias)
```

## 构建和运行

从仓库根目录构建：

```bash
cmake -S . -B build
cmake --build build --target fun_compiler_demo
./build/Fun_compiler/fun_compiler_demo
```

也可以单独构建本目录：

```bash
cmake -S Fun_compiler -B Fun_compiler/build
cmake --build Fun_compiler/build
./Fun_compiler/build/fun_compiler_demo
```

## 运行时会看到什么

程序顺次打印：

1. 前端构造的 Graph IR：`input`、`constant`、`mul`、`add`、`relu` 节点，以及节点间的输入边。
2. 解释器逐节点执行的结果。它代表优化前最直接的执行方式，会产生 `scaled` 与 `shifted` 两个中间张量。
3. 算子融合 pass 后的 Graph IR：模式 `relu(add(mul(x, weight), bias))` 变成一个 `fused.mul_add_relu` 节点。
4. AOT 执行：启动前就完成图优化和 lowering，之后每次调用直接运行已有 kernel。
5. JIT 执行：第一次遇到某个 shape 时编译并缓存，第二次看到相同 shape 时命中缓存。

## 关键概念

| 概念 | 这个 demo 中的对应物 | 真正框架中的含义 |
| --- | --- | --- |
| Graph IR | `Graph`、`Node`、`OpCode` | 用节点表示算子、用边表示张量依赖的数据流程序；常见于 TensorFlow Graph、Torch FX、TVM Relay 等。 |
| 算子融合 | `fuseMulAddRelu()` | 把多个小算子合成一个 kernel，减少中间张量读写、kernel launch 和调度开销。 |
| Lowering | `lowerToKernel()` | 将较高层的图算子降到更接近目标的表示；这里简化为生成一个 C++ 循环 kernel。真实系统会继续降到 LLVM IR、CUDA、Triton、C 或设备指令。 |
| AOT | `AotCompiler` | 提前编译，部署时启动快、延迟稳定；通常要求输入 shape、硬件和优化选项较固定。 |
| JIT | `JitCompiler` | 运行时根据实际 shape 或硬件特征编译/选择 kernel；首次有编译开销，但对动态输入更灵活。 |

## AI 编译器和传统编译器的区别

传统编译器主要把 C/C++/Rust 等通用程序降到机器代码，重点处理控制流、寄存器分配、指令选择和 ABI。

AI 编译器也需要 lowering 和代码生成，但优化对象往往是张量计算图。它额外掌握 shape、dtype、layout、算子语义、并行维度与目标加速器的信息，因此能做矩阵乘分块、算子融合、内存规划、layout 转换消除、自动调优和特定硬件 kernel 选择。

两者不是彼此替代的关系。AI 编译器通常会在最后借助传统编译器基础设施，例如 LLVM、NVCC 或设备驱动，把低层 kernel 变成可执行代码。

## 建议的阅读顺序

1. 阅读 `Node` 和 `Graph`，确认一条边只是一个节点 id，表示 SSA/dataflow 依赖。
2. 运行 `interpret()`，理解未优化的逐算子执行为何要保存中间张量。
3. 阅读 `fuseMulAddRelu()`，把它当作一个最小 rewrite pass：匹配图模式后生成更高效的节点。
4. 阅读 `lowerToKernel()`，理解 Graph IR 不直接等同于 CPU/GPU 指令，需要经过 lowering。
5. 对比 `AotCompiler` 与 `JitCompiler`：两者都复用同一个优化和 lowering，只是编译发生的时刻、缓存策略不同。

## 有意省略的真实系统复杂度

此 demo 只支持一维、等长的 `float` 张量，并假设图按拓扑顺序构建。真实 AI 编译器还会有类型与 shape 推导、动态 shape、layout、别名分析、buffer 复用、控制流、子图划分、设备间通信、自动调优、数值精度和错误处理等模块。

理解这个 demo 后，可以再阅读 `Cpp_prim/src/06_mlir_style_ir.cpp`，把这里的简化 Graph IR 与 MLIR 的 `Operation` / `Value` / `Block` / `Context` 对应起来。
