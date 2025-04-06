cmake_minimum_required(VERSION 3.15)

set(BOOT custom)
set(LDSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tcm.ld")
set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(app LANGUAGES C)
if (BOOT STREQUAL "extram")
	add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)
endif()

add_executable(app main.c)
target_compile_options(app PUBLIC -Os -fno-toplevel-reorder)
if (BOOT STREQUAL "extram")
	target_link_libraries(app PUBLIC pmb887x)
endif()
target_output_bin(app)
