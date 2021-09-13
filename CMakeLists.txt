cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(i2c_slave_example C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(i2c_slave)

add_executable(i2c_slave_example i2c_slave_example.c)

pico_enable_stdio_uart(i2c_slave_example 1)
pico_enable_stdio_usb(i2c_slave_example 1)

pico_add_extra_outputs(i2c_slave_example)

target_compile_options(i2c_slave_example PRIVATE -Wall)

target_link_libraries(i2c_slave_example hardware_i2c i2c_slave pico_multicore pico_stdlib)
