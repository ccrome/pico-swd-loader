cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ram_blink C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(ram_blink
    blink_sdk_minimal.c
)

# Configure for RAM-only execution (no flash)
pico_set_binary_type(ram_blink no_flash)

target_link_libraries(ram_blink
    pico_stdlib
)

pico_enable_stdio_usb(ram_blink 0)
pico_enable_stdio_uart(ram_blink 0)

