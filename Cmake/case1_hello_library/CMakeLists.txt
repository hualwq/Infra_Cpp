cmake_minimum_required(VERSION 3.10)
project(Case1_Hello)

# 创建一个共享库目标：libhello.so
add_library(hello SHARED hello.cpp)

# 指定 hello 库的头文件搜索路径
# PUBLIC：自己编译时需要，依赖者也（链接本库的人）需要
target_include_directories(hello PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 创建一个可执行文件目标
add_executable(case1_hello main.cpp)

# 把 main 和 hello 库链接起来
target_link_libraries(case1_hello PRIVATE hello)
