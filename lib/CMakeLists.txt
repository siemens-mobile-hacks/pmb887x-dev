cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE toolchain.cmake)

project(pmb887x LANGUAGES C ASM)
add_compile_options(-Os)
add_library(pmb887x OBJECT
	init/start.S
	init/reset_handler.c
	libc.c
	usart.c
	i2c.c
	printf.c
	wdt.c
	stopwatch.c
	cpu.c
)
