INCLUDE(CMakeForceCompiler)

if (NOT COMPILER)
	SET(COMPILER		"arm-none-eabi")
endif()

if (NOT SIE_LOCATION)
	SET(SIE_LOCATION	"intram")
endif()

if (NOT SIE_NO_INIT)
	SET(SOURCES
		"${COMMON_PATH}/init/${SIE_LOCATION}.S"
		"${COMMON_PATH}/init/${SIE_LOCATION}.c"
		"${COMMON_PATH}/eabi/div.c"
		"${COMMON_PATH}/eabi/div.S"
		${SOURCES}
	)
endif()

SET(CMAKE_C_COMPILER	"${COMPILER}-gcc")
SET(CMAKE_CXX_COMPILER	"${COMPILER}-g++")
SET(CMAKE_ASM_COMPILER	"${COMPILER}-gcc")
SET(CMAKE_AR			"${COMPILER}-ar")
SET(CMAKE_LD			"${COMPILER}-ld")

SET(SIE_LINKER_SCRIPT	"${COMMON_PATH}/linker/${SIE_LOCATION}.ld")

SET(SIE_LDFLAGS "-gc-sections -nostdlib -T '${SIE_LINKER_SCRIPT}'")
SET(SIE_CFLAGS "-e __enter_reset -fvisibility=hidden -fdata-sections -ffunction-sections -c -fomit-frame-pointer -mcpu=arm926ej-s -nostdlib -std=gnu11")

SET(CMAKE_CXX_FLAGS			"${CMAKE_CXX_FLAGS} ${SIE_CFLAGS}")
SET(CMAKE_C_FLAGS			"${CMAKE_C_FLAGS} ${SIE_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS	"${CMAKE_EXE_LINKER_FLAGS} ${SIE_LDFLAGS}")

SET(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS		"")
SET(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS		"")

# Правильный линкер
SET(CMAKE_C_LINK_EXECUTABLE			"${CMAKE_LD} --gc-sections -nostdlib <OBJECTS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>")
SET(CMAKE_CXX_LINK_EXECUTABLE		"${CMAKE_LD} --gc-sections -nostdlib <OBJECTS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <LINK_LIBRARIES> -o <TARGET>")

# Зачем-то насильно переопределяем компилятор
CMAKE_FORCE_C_COMPILER(${CMAKE_C_COMPILER} GNU)
CMAKE_FORCE_CXX_COMPILER(${CMAKE_CXX_COMPILER} GNU)

include_directories(${COMMON_PATH})
add_executable ("${OUTPUT_NAME}.elf" ${SOURCES} ${HEADERS})
add_custom_command(TARGET "${OUTPUT_NAME}.elf" POST_BUILD COMMAND arm-none-eabi-objcopy -O binary "${OUTPUT_NAME}.elf" "${OUTPUT_NAME}.bin")
