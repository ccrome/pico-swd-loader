cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK
# Use environment variable if set, otherwise use relative path
if(DEFINED ENV{PICO_SDK_PATH})
    get_filename_component(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" ABSOLUTE)
else()
    # Default to relative path from project root
    get_filename_component(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../pico-sdk" ABSOLUTE)
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(ram_usb_cdc C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Create the executable
add_executable(ram_usb_cdc
    usb_cdc_loopback.c
)

# Configure for RAM-only execution (no flash)
pico_set_binary_type(ram_usb_cdc no_flash)

target_link_libraries(ram_usb_cdc
    pico_stdlib
)

# Enable USB CDC stdio, disable UART
pico_enable_stdio_usb(ram_usb_cdc 1)
pico_enable_stdio_uart(ram_usb_cdc 0)

# Add USB connection timeout
target_compile_definitions(ram_usb_cdc PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
)

