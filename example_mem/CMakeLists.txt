add_executable(example_mem example_mem.c)

pico_enable_stdio_uart(example_mem 1)
pico_enable_stdio_usb(example_mem 1)

pico_add_extra_outputs(example_mem)

target_compile_options(example_mem PRIVATE -Wall)

target_link_libraries(example_mem i2c_slave pico_stdlib)
