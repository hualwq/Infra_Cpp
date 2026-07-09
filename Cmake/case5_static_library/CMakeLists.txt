cmake_minimum_required(VERSION 3.10)
project(Case5_StaticLib)

# ============================================
# 静态库链接 (STATIC)
# libgreet.a 的代码在链接时完整复制进可执行文件
# 运行时不再需要任何 .a 文件，程序独立运行
# ============================================
add_executable(case5_static main.cpp)
target_include_directories(case5_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(case5_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libgreet.a)
