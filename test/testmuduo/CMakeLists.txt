cmake_minimum_required(VERSION 3.0)
project(main)

#编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#头文件搜索路径
#include_directories()

#库文件搜索路径
#link_directories()

#源文件列表
set(SRC_LIST ./muduo_server.cpp)

#指定可执行文件目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

#把指定路径下的所有源文件名字放入变量SRC_LIST
#aux_source_directory(. SRC_LIST)

add_executable(server ${SRC_LIST})

#链接的库文件
target_link_libraries(server muduo_net muduo_base pthread)