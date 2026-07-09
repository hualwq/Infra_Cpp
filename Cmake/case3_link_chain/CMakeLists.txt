cmake_minimum_required(VERSION 3.10)
project(Case3_LinkChain)

# === 底层核心库 ===
add_library(core_math SHARED core_math.cpp)
target_include_directories(core_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# === 上层库：依赖 core_math ===
add_library(advanced_math SHARED advanced_math.cpp)
target_include_directories(advanced_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接 core_math —— 用 PUBLIC！
# 因为 advanced_math 的接口返回值来自 core_math 的概念，
# 使用者虽然不直接 include core_math.h，但为了演示传递性：
target_link_libraries(advanced_math PUBLIC core_math)
#   ^^^^^^
#   PUBLIC 传递：任何链接 advanced_math 的目标，
#   也会自动链接 core_math（并继承其头文件搜索路径）
#
#   试试改成 PRIVATE 看看会发生什么 ↓

# === 可执行文件 ===
add_executable(case3_linkchain main.cpp)
target_link_libraries(case3_linkchain PRIVATE advanced_math)
# main 只链接了 advanced_math，
# 但因为 advanced_math 用 PUBLIC 链接 core_math，
# core_math 会自动传递过来 —— main 不用感知 core_math 的存在！
