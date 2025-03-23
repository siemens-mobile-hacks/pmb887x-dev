cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(app LANGUAGES C)
add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_executable(app main.c)
target_compile_options(app PUBLIC -O2)
target_link_libraries(app PUBLIC pmb887x)
target_output_bin(app)
