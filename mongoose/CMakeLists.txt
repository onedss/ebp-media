cmake_minimum_required(VERSION 3.23)
project(mongoose C)

set(CMAKE_C_STANDARD 99)

add_library(mongoose mongoose.h mongoose.c)