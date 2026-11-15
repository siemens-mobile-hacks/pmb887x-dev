cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(app LANGUAGES C)
add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_executable(app main.c ../../unit/cgu/clock.c)
target_include_directories(app PRIVATE ../../unit)
target_compile_options(app PUBLIC -Os)
target_link_libraries(app PUBLIC pmb887x)
target_output_bin(app)
