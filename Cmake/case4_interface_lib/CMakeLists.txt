cmake_minimum_required(VERSION 3.10)
project(Case4_Interface)

# ============================================
# INTERFACE 库：纯头文件库，没有 .cpp 文件
#   类似于 C++ 中的"抽象接口"概念
#   自身不编译，只为传递依赖信息给使用者
# ============================================
add_library(printable INTERFACE)
# 没有源文件！INTERFACE 库不需要编译

target_include_directories(printable INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
#        ^^^^^^^^^^
#        INTERFACE：本目标不需要此路径（因为不编译），
#        但所有依赖本目标的人都必须要有这个头文件目录

# 对比记忆：
#   PRIVATE    = 我自己用，传给别人
#   PUBLIC     = 我自己用，也传给别人
#   INTERFACE  = 我自己不用，只传给别人（纯头文件库）

add_executable(case4_interface main.cpp)
target_link_libraries(case4_interface PRIVATE printable)
# main 链接 printable 后，自动获得 printable 的头文件搜索路径
