cmake_minimum_required(VERSION 3.10)
project(Case6_SharedLib)

# ============================================
# 动态库链接 (SHARED)
# 1) 用 SHARED 构建 libgreet_shared.so
# 2) main.cpp 链接时只记录符号依赖
# 3) 运行时必须能找到 libgreet_shared.so
#    CMake 自动将 lib/ 路径写入 RUNPATH
#    如果缺了 RUNPATH，需设 LD_LIBRARY_PATH
# ============================================

# 1) 构建动态库
add_library(greet_shared SHARED greet.cpp)
target_include_directories(greet_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_custom_command(TARGET greet_shared POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:greet_shared> ${CMAKE_CURRENT_SOURCE_DIR}/lib/
  COMMENT "Deploying shared library → lib/"
)

# 2) 构建可执行文件并链接
add_executable(case6_shared main.cpp)
target_include_directories(case6_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(case6_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/libgreet_shared.so)
add_dependencies(case6_shared greet_shared)
