cmake_minimum_required(VERSION 3.10)
project(ReactorWebServer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 强制要求编译器支持c++17

# 包含目录
include_directories(${CMAKE_SOURCE_DIR})

# 递归搜索源文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

# 生成可执行文件
add_executable(server ${SOURCES})
