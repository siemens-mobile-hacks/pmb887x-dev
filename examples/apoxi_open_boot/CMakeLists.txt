cmake_minimum_required(VERSION 3.15)

set(BOOT none)
set(LDSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tcm.ld")
set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(app LANGUAGES C ASM)
if (BOOT STREQUAL "extram")
	add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)
endif()

add_executable(app main.c init.S)
target_compile_options(app PUBLIC -Os)
if (BOOT STREQUAL "extram")
	target_link_libraries(app PUBLIC pmb887x)
else()
	target_link_options(app PUBLIC -pie -fPIE)
endif()
target_output_bin(app)
