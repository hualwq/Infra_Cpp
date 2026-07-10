cmake_minimum_required(VERSION 3.16)

add_executable(fun_compiler_demo src/main.cpp)
target_compile_features(fun_compiler_demo PRIVATE cxx_std_17)
target_compile_options(fun_compiler_demo PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)
