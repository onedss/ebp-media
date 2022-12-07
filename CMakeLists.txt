cmake_minimum_required(VERSION 3.23)
project(ebp-media)

set(CMAKE_CXX_STANDARD 11)

# 添加头文件
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/mongoose/include
)

link_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/mongoose/lib/x64
)

add_executable(ebp-media main.cpp)

TARGET_LINK_LIBRARIES(
        ebp-media
        ws2_32
        wsock32
        mongoose.dll
)
