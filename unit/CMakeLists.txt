cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ../lib/toolchain.cmake)

project(unit LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_library(test_support OBJECT test.c)
target_compile_options(test_support PUBLIC -Os -Wall -Wextra -Werror)
target_include_directories(test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

function(add_peripheral_test name)
	add_executable(${name} ${ARGN})
	target_compile_options(${name} PUBLIC -Os -Wall -Wextra -Werror)
	target_link_libraries(${name} PUBLIC pmb887x test_support)
	target_output_bin(${name})
endfunction()

add_peripheral_test(stm stm/main.c)
add_peripheral_test(dmac dmac/main.c)
add_peripheral_test(mod mod/main.c)
add_peripheral_test(adc adc/main.c)
add_peripheral_test(i2c-v2 i2c-v2/main.c)
add_peripheral_test(usart usart/main.c)
add_peripheral_test(dif-v2 dif-v2/main.c)
add_peripheral_test(cfi-intel cfi-intel/main.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-rw cfi-intel/rw.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-efa-rw cfi-intel/efa-rw.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-otp-rw cfi-intel/otp-rw.c cfi-intel/cfi.c)
