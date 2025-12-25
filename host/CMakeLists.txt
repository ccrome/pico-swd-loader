cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(swd_loader C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Main executable
add_executable(${PROJECT_NAME}
    main.cpp
    swd_load.cpp
    pio_helpers.cpp
)

# Generate PIO header (must come AFTER add_executable)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/swd.pio)

# Pull in common dependencies
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pio
    hardware_clocks
    hardware_gpio
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Add additional compile definitions for USB
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=3000
)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

