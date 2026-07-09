cmake_minimum_required(VERSION 3.16)

project(AICompilerCppBasics LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(add_lesson target source)
  add_executable(${target} ${source})
  target_compile_options(${target} PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
  )
endfunction()

add_lesson(01_string_ref_array_ref src/01_string_ref_array_ref.cpp)
add_lesson(02_small_vector src/02_small_vector.cpp)
add_lesson(03_bump_ptr_allocator src/03_bump_ptr_allocator.cpp)
add_lesson(04_isa_cast_dyn_cast src/04_isa_cast_dyn_cast.cpp)
add_lesson(05_crtp_visitor src/05_crtp_visitor.cpp)
add_lesson(06_mlir_style_ir src/06_mlir_style_ir.cpp)
add_lesson(07_support_patterns src/07_support_patterns.cpp)
