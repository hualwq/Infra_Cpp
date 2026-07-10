cmake_minimum_required(VERSION 3.10)
project(Interview)

# 自动收集当前目录下所有的 .cpp 文件
file(GLOB CPP_FILES "*.cpp")

# 为每个 .cpp 文件创建一个可执行文件
foreach(CPP_FILE ${CPP_FILES})
    # 获取文件名（不含扩展名）
    get_filename_component(EXE_NAME ${CPP_FILE} NAME_WE)
    # 创建可执行文件
    add_executable(interview_${EXE_NAME} ${CPP_FILE})
endforeach()
