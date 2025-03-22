cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(i2c LANGUAGES C)
add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_executable(i2c main.c)
target_compile_options(i2c PUBLIC -Os)
target_link_libraries(i2c PUBLIC pmb887x)
target_output_bin(i2c)
