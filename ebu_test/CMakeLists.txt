cmake_minimum_required(VERSION 2.8)
project (app C ASM)

SET(SIE_LOCATION		"intram")
SET(OUTPUT_NAME			"app")
SET(COMMON_PATH			"${PROJECT_SOURCE_DIR}/../common")
SET(CMAKE_MODULE_PATH	"${COMMON_PATH}/cmake")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")

set (SOURCES
	main.c
	"${COMMON_PATH}/pmb8876.c"
	"${COMMON_PATH}/i2c.c"
	"${COMMON_PATH}/printf.c"
)
set (HEADERS
	main.h
	"${COMMON_PATH}/pmb8876.h"
	"${COMMON_PATH}/i2c.h"
	"${COMMON_PATH}/printf.h"
)

INCLUDE(SiemensBootcode)
