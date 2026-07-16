# Learn Cpp Infra Basics

[中文版](ReadMe_zh.md)

This repository is a collection of small demos organized while learning compiler infrastructure and modern C++ fundamentals. The content focuses on three main areas: compiler-related C++ design patterns, CMake project organization, common C++ language features, and frequently asked C++ interview questions.

The code in this repository is not intended to be a complete project. The emphasis is on breaking down each concept into small, independently compilable and readable examples, making it easier to build intuition when reading source code from LLVM/MLIR, TVM, or other infrastructure projects.

## Directory Overview

### `Cpp_prim/`

This section focuses on C++ basics oriented toward LLVM/MLIR-style code. It covers non-owning views, SmallVector, small object optimization, arena/bump allocators, LLVM-style RTTI, CRTP visitors, MLIR-style IR structures, and common utility patterns that support compiler infrastructure.

These examples are best reviewed before diving into LLVM/MLIR code, with a focus on understanding why compiler projects make heavy use of `StringRef`, `ArrayRef`, `SmallVector`, `isa/cast/dyn_cast`, visitors, and arena lifetime management.

**Included examples:**
- `01_string_ref_array_ref.cpp` - Non-owning views (StringRef/ArrayRef)
- `02_small_vector.cpp` - Small object optimization (SmallVector)
- `03_bump_ptr_allocator.cpp` - Arena allocator
- `04_isa_cast_dyn_cast.cpp` - LLVM-style RTTI
- `05_crtp_visitor.cpp` - CRTP static polymorphism and Visitor pattern
- `06_mlir_style_ir.cpp` - MLIR-style IR structure
- `07_support_patterns.cpp` - LLVM Support utility patterns

Detailed study notes: `Cpp_prim/ReadMe.md`

### `Fun_Cpp/`

This section covers more general C++ language fundamentals and common techniques, including lambdas, virtual functions, polymorphism, smart pointers, templates, a simplified understanding of the TVM object system, visitor dispatch, and macro-related usage.

These examples focus more on the language features themselves and serve as a supplement before reading infrastructure source code. Many large C++ projects mix templates, macros, polymorphism, object systems, and visitors; understanding each mechanism through small demos first makes it easier to grasp the abstractions in real projects.

**Included examples:**
- `1_lambda.cpp` - Lambda expressions
- `2_virtualFunc.cpp` - Virtual functions and polymorphism
- `3_smart_ptr.cpp` - Smart pointers (unique_ptr/shared_ptr/weak_ptr)
- `4_templates.cpp` - Template programming
- `5_tvm_object_system.cpp` - Simplified TVM Object System
- `6_defile_visitor.cpp` - Visitor pattern in detail
- `7_macros.cpp` - Macro-related usage
- `8_function.cpp` - Function-related features

### `Fun_compiler/`

A mini AI compiler pipeline demo, showing the basic process from computation graph definition to compiled execution. Includes:

- **Computation Graph IR** - Defining operators (Mul/Add/Relu) and data flow
- **Compiler Passes** - Operator fusion optimization (fuse Mul+Add+Relu)
- **Code Generation** - Lowering to kernel functions
- **Execution Modes** - Both AOT (ahead-of-time) and JIT (just-in-time) modes

This demo is suitable for understanding core AI compiler concepts: computation graph representation, graph optimization, and code generation.

### `Cmake/`

This section covers basic CMake project organization, split into cases for different scenarios. It covers the most basic executable and library builds, header file include scope, inter-library link dependencies, interface libraries, static libraries, shared libraries, and the propagation rules of `PUBLIC` / `PRIVATE` / `INTERFACE` in real target dependencies.

These demos mainly help understand modern CMake's target-based approach: don't just treat CMake as a build script, but model each library, executable, and dependency as a target.

**Included cases:**
- `case1_hello_library` - Basic library and executable build
- `case2_include_scope` - Header file include scope
- `case3_link_chain` - Library link dependency chain
- `case4_interface_lib` - Interface library
- `case5_static_library` - Static library
- `case6_shared_library` - Shared library
- `case7_public_private_interface` - PUBLIC/PRIVATE/INTERFACE propagation rules

### `interview/`

A collection of common C++ interview questions, with code examples and detailed explanations:

- **Smart Pointers** - Principles and usage scenarios of unique_ptr/shared_ptr/weak_ptr
- **Type Casting** - Comparison of static_cast/dynamic_cast/const_cast/reinterpret_cast
- **Virtual Function Mechanism** - Virtual function table, polymorphism implementation principles
- **Memory Management** - Heap vs. stack, new/delete vs. malloc/free
- **Move Semantics** - Rvalue references, std::move, perfect forwarding

Detailed interview notes: `interview/interview.md`

## Recommended Learning Order

1. Start with `Fun_Cpp/` to fill in C++ language features and common patterns
2. Then review `Cmake/` to understand how code is organized, compiled, and linked
3. Next, go through `Cpp_prim/` to understand C++ mechanisms in the context of compiler infrastructure
4. Finally, review `Fun_compiler/` to see how these concepts combine into a simple compiler
5. `interview/` can be used as a reference for filling gaps at any stage

## Build Instructions

### Build from Root Directory (build all subprojects)

```bash
mkdir -p build && cd build
cmake ..
make
```

### Build Subprojects Individually

**Cpp_prim/:**
```bash
cd Cpp_prim
cmake -S . -B build
cmake --build build

# Run examples
./build/01_string_ref_array_ref
./build/02_small_vector
# ...
```

**Fun_compiler/:**
```bash
cd Fun_compiler
cmake -S . -B build
cmake --build build
./build/fun_compiler
```

**Cmake/ cases:**
```bash
cd Cmake/case1_hello_library
cmake -S . -B build
cmake --build build
```

**Fun_Cpp/ and interview/:**
These directories contain single-file examples that can be compiled and run directly with `g++` or `clang++`:
```bash
g++ -std=c++17 -Wall -Wextra -pedantic 1_lambda.cpp -o lambda
./lambda
```

## Dependencies

- C++ Compiler: C++17 support required (g++ 7+ or clang++ 5+)
- CMake: Version 3.10 or above
- Build Tool: Make or Ninja

## References

- [LLVM Programmer's Manual](https://llvm.org/docs/ProgrammersManual.html)
- [MLIR Tutorial](https://mlir.llvm.org/docs/Tutorials/)
- [CMake Documentation](https://cmake.org/documentation/)
- [C++ Reference](https://en.cppreference.com/)

## Contributing

This repository is a personal study notebook. Suggestions and corrections are welcome. If you have any questions, please submit an issue or pull request.

## License

MIT License
