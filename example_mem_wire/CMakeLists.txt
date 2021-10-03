add_executable(example_mem_wire Wire.cpp example_mem_wire.cpp)

pico_enable_stdio_uart(example_mem_wire 1)
pico_enable_stdio_usb(example_mem_wire 1)

pico_add_extra_outputs(example_mem_wire)

target_compile_options(example_mem_wire PRIVATE -Wall)

target_link_libraries(example_mem_wire i2c_slave pico_stdlib)
