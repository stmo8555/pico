cmake_minimum_required(VERSION 4.3.0)

include(PICO_SDK_PATH/external/pico_sdk_import.cmake)
project(pico)

set(CMAKE_CXX_STANDARD 23)

pico_sdk_init()

add_executable(pico main.cpp)

target_link_libraries(pico pico_stdlib)
