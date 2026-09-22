cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE toolchain.cmake)

project(pmb887x LANGUAGES C ASM)

add_compile_options(-Os)

set(BUILD_SHARED_LIBS OFF)
set(PRINTF_BUILD_TESTS OFF)
set(PRINTF_NO_INSTALL ON)
set(SUPPORT_DECIMAL_SPECIFIERS OFF)
set(SUPPORT_EXPONENTIAL_SPECIFIERS OFF)
set(SUPPORT_MSVC_STYLE_INTEGER_SPECIFIERS OFF)
set(SUPPORT_WRITEBACK_SPECIFIER OFF)
set(SUPPORT_LONG_LONG OFF)
set(USE_DOUBLE_INTERNALLY OFF)
set(CHECK_FOR_NUL_IN_FORMAT_SPECIFIER OFF)
set(ALIAS_STANDARD_FUNCTION_NAMES HARD CACHE STRING "" FORCE)
add_subdirectory(../third_party/printf third_party/printf EXCLUDE_FROM_ALL)

add_library(pmb887x OBJECT
	init/header.S
	init/start.S
	init/relocate.S
	init/signature.S
	init/reset_handler.c
	usart.c
	i2c.c
	i2c-v2.c
	printf.c
	wdt.c
	stopwatch.c
	cpu.c
)
target_link_libraries(pmb887x PUBLIC printf::printf)
