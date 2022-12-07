cmake_minimum_required(VERSION 3.23)
project(mongoose C)

set(CMAKE_C_STANDARD 99)

add_library(
        mongoose
        SHARED
        mongoose.h
        mongoose.c
)

TARGET_LINK_LIBRARIES(
        mongoose
        ws2_32
        wsock32
)