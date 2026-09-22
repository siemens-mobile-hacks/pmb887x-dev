cmake_minimum_required(VERSION 3.15)

if (NOT BOARD)
	message(FATAL_ERROR "Set BOARD or run build.sh")
endif()

set(CMAKE_TOOLCHAIN_FILE ../lib/toolchain.cmake)
set(BOOT tcm)
set(SECURE_BOOT ON)

project(preloader LANGUAGES C ASM)
add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_executable(preloader src/crc.c src/lz4.c src/main.c)
target_compile_options(preloader PUBLIC -Os)
target_link_libraries(preloader PUBLIC pmb887x)
target_output_bin(preloader)
if (BOARD MATCHES "^apoxi-")
	target_sign_boot(preloader)
endif()
