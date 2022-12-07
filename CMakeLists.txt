cmake_minimum_required(VERSION 3.23)
project(ebp-media)

set(CMAKE_CXX_STANDARD 11)

# 添加头文件
include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/mongoose/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/BasicUsageEnvironment/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/darwin/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/groupsock/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/liveMedia/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/UsageEnvironment/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libevent-2.1.10/include
)

link_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/mongoose/lib/x64
        ${CMAKE_CURRENT_SOURCE_DIR}/libevent-2.1.10/lib
)

set(SOURCE_FILES ./main.cpp)
AUX_SOURCE_DIRECTORY(./src/BasicUsageEnvironment SOURCE_FILES)
AUX_SOURCE_DIRECTORY(./src/darwin SOURCE_FILES)
AUX_SOURCE_DIRECTORY(./src/groupsock SOURCE_FILES)
AUX_SOURCE_DIRECTORY(./src/liveMedia SOURCE_FILES)
AUX_SOURCE_DIRECTORY(./src/UsageEnvironment SOURCE_FILES)

ADD_DEFINITIONS("-g -O0")
ADD_DEFINITIONS("-std=c++11")

add_executable(
        ebp-media
        ${SOURCE_FILES}
)

TARGET_LINK_LIBRARIES(
        ebp-media
        ws2_32
        wsock32
        pthread
        event
        mongoose.dll
)
