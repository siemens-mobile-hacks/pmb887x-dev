cmake_minimum_required(VERSION 2.8)
project (armsniffer C ASM) 
INCLUDE(CMakeForceCompiler)

SET(CPREF				"arm-none-eabi")
SET(CMAKE_C_COMPILER	"${CPREF}-gcc")
SET(CMAKE_CXX_COMPILER	"${CPREF}-g++")
SET(CMAKE_ASM_COMPILER	"${CPREF}-gcc")
SET(CMAKE_AR			"${CPREF}-ar")
SET(CMAKE_LD			"${CPREF}-ld")

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

SET(SIE_LDFLAGS "-Wl,--defsym=__ex=0 -Wl,-pie -Wl,-s -Wl,-gc-sections -Wl,-zmax-page-size=1 -Wl,-nostdlib")
SET(SIE_CFLAGS "-Wall -msoft-float -fshort-wchar -D__arm='' -std=gnu99 -marm -mlittle-endian -mcpu=arm926ej-s -O0 -fno-builtin -nodefaultlibs -nostdlib -nostdinc -pipe -fno-exceptions -fdata-sections -ffunction-sections")

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIE_CFLAGS}")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SIE_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SIE_LDFLAGS}")

link_directories(${SIEMENS_DEV_ROOT}/lib)
include_directories(${SIEMENS_DEV_ROOT}/include)

add_executable (test.elf ${SOURCES} ${HEADERS})
target_link_libraries(test.elf crt crt_helper c-1.0)
