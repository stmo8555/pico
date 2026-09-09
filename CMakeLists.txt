cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
project(pico C CXX ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Keep compile_commands.json symlinked at the source root for clangd,
# which searches upward from each file rather than into build/.
file(CREATE_LINK
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_SOURCE_DIR}/compile_commands.json
  SYMBOLIC
)

pico_sdk_init()

add_executable(pico main.cpp)
target_include_directories(pico PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${PICO_SDK_PATH}/lib/tinyusb/lib/fatfs/source
)
target_link_libraries(pico
    pico_stdlib
    hardware_flash
    hardware_sync
    tinyusb_device
    tinyusb_board
)

# Route stdio over USB CDC instead of UART
# Might need to remove if we use tinyUSB, see page 601
# pico_enable_stdio_usb(pico 1)
# pico_enable_stdio_uart(pico 0)
