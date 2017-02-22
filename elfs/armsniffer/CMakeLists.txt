cmake_minimum_required(VERSION 2.8)
project (armsniffer C ASM) 
INCLUDE(CMakeForceCompiler)

SET(CPREF				arm-none-eabi)
SET(CMAKE_C_COMPILER	${CPREF}-gcc CACHE STRING "arm-none-eabi-gcc" FORCE)
SET(CMAKE_CXX_COMPILER	${CPREF}-g++ CACHE STRING "arm-none-eabi-g++" FORCE)
SET(CMAKE_AR			${CPREF}-ar CACHE STRING "arm-none-eabi-ar" FORCE)
SET(CMAKE_LD			${CPREF}-ld CACHE STRING "arm-none-eabi-ld" FORCE)
SET(OBJCOPY				${CPREF}-objcopy CACHE STRING "arm-none-eabi-objcopy" FORCE)
SET(OBJDUMP				${CPREF}-objdump CACHE STRING "arm-none-eabi-objdump" FORCE)
SET(READELF				${CPREF}-readelf CACHE STRING "arm-none-eabi-readelf" FORCE)
SET(CMAKE_RANLIB		${CPREF}-ranlib CACHE STRING "arm-none-eabi-ranlib" FORCE)

CMAKE_FORCE_C_COMPILER(${CPREF}-gcc GNU)
CMAKE_FORCE_CXX_COMPILER(${CPREF}-g++ GNU)

set (SOURCES
	main.c
	sniffer/system.c
	sniffer/sniffer.c
	sniffer/da_handler.S
)
set (HEADERS
	sniffer/arm_defs.h
	sniffer/sniffer.h
	sniffer/sniffer.h
)

SET(CMAKE_CXX_FLAGS "-Wall -msoft-float -fshort-wchar -D__arm='' -std=gnu99 -marm -mlittle-endian -mcpu=arm926ej-s -O0 -fno-builtin -nodefaultlibs -nostdlib -nostdinc -pipe -fno-exceptions -fdata-sections -ffunction-sections")
SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--defsym=__ex=0 -Wl,-pie -Wl,-s -Wl,-gc-sections -Wl,-zmax-page-size=1 -Wl,-nostdlib")

link_directories("${SIEMENS_DEV_ROOT}/lib")
include_directories("${SIEMENS_DEV_ROOT}/include")

add_executable (test.elf ${SOURCES} ${HEADERS})
target_link_libraries(test.elf crt crt_helper c-1.0)
