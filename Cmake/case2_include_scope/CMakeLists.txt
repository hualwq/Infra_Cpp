cmake_minimum_required(VERSION 3.10)
project(Case2_Scope)

# ============================================
# math_ops 库：internal.h 是内部实现细节
#   用 PRIVATE 正确隔离，外部无需知道 internal.h
# ============================================
add_library(math_ops SHARED math_ops.cpp)
target_include_directories(math_ops PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
#        ^^^^^^^
#        仅 math_ops 自己编译时能看到此目录下的头文件
#        依赖 math_ops 的 main 看不到 internal.h

# ============================================
# printer 库：printer.h 是公开接口
#   用 PUBLIC，使用者也必须知道 printer.h 在哪
# ============================================
add_library(printer SHARED printer.cpp)
target_include_directories(printer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#        ^^^^^^
#        既给自己用，也传给链接本库的人

# ============================================
# main 可执行文件：链接两个库
# ============================================
add_executable(case2_scope main.cpp)
target_link_libraries(case2_scope PRIVATE math_ops printer)
# 此时 main 能看到 printer.h 但看不到 internal.h
# 这正是 PRIVATE 的用意 —— 隐藏实现细节
