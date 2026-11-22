cmake_minimum_required(VERSION 3.15)

set(BOOT custom)
set(LDSCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/payload.ld")
set(CMAKE_TOOLCHAIN_FILE ../../lib/toolchain.cmake)

project(dwd-usb-preloader LANGUAGES C ASM)

add_executable(dwd-usb-preloader src/main.c src/usb.c src/crc.c src/wdt.c src/sdram.c)
target_compile_options(dwd-usb-preloader PUBLIC -Os -Werror -Wall -Wno-unused-variable -Wno-unused-function)
target_link_libraries(dwd-usb-preloader)
target_output_bin(dwd-usb-preloader)
