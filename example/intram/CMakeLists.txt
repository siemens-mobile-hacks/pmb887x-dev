cmake_minimum_required(VERSION 2.8)
project (app C ASM)

SET(SIE_LOCATION		"intram")
SET(OUTPUT_NAME			"app")
SET(COMMON_PATH			"${PROJECT_SOURCE_DIR}/../../common")
SET(CMAKE_MODULE_PATH	"${COMMON_PATH}/cmake")

set (SOURCES
	main.c
	"${COMMON_PATH}/pmb8876.c"
)
set (HEADERS
	"${COMMON_PATH}/pmb887x.h"
)

INCLUDE(SiemensBootcode)
