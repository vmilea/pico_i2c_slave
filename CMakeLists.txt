cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(i2c_examples C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(i2c_slave)
add_subdirectory(example_mem)
