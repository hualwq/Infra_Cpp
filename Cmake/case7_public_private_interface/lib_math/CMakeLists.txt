add_library(math STATIC math.cpp)
target_include_directories(math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# PUBLIC: core 的头文件路径会透传给 app
# 改成 PRIVATE 后 app 将无法 #include "core.h"
target_link_libraries(math PUBLIC core)
