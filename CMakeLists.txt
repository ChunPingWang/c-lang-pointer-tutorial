cmake_minimum_required(VERSION 3.16)

project(c_pointer_tutorial
    VERSION 0.1.0
    LANGUAGES C
    DESCRIPTION "C pointer tutorial driven by an STM32 + GPS NMEA pipeline")

set(CMAKE_C_STANDARD          11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS        OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -Wno-sign-conversion
)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(gps_core STATIC
    src/gps/ring_buffer.c
    src/gps/nmea_parser.c
    src/hal/uart_mock.c
)
target_link_libraries(gps_core PUBLIC m)

add_executable(gps_demo src/app/main.c)
target_link_libraries(gps_demo PRIVATE gps_core)

add_executable(nmea_checksum scripts/checksum.c)

enable_testing()
add_subdirectory(tests)
add_subdirectory(examples)
