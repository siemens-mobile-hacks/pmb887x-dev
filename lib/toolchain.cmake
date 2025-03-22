get_filename_component(PMB887X_LIB_PATH ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)

set(CMAKE_C_COMPILER_WORKS true)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

if (NOT DEFINED BOARD)
	set(BOARD "SIEMENS_EL71")
endif()

if (NOT DEFINED BOOT)
	set(BOOT "intram")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_VERSION 1)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_LIBRARY_ARCHITECTURE arm-none-eabi)

find_program(ARM_C_COMPILER arm-none-eabi-gcc${CMAKE_EXECUTABLE_SUFFIX} NO_CACHE REQUIRED)
find_program(ARM_CXX_COMPILER arm-none-eabi-g++${CMAKE_EXECUTABLE_SUFFIX} NO_CACHE REQUIRED)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy${CMAKE_EXECUTABLE_SUFFIX} NO_CACHE REQUIRED)

set(CMAKE_C_COMPILER ${ARM_C_COMPILER} CACHE INTERNAL "CMAKE_C_COMPILER")
set(CMAKE_CXX_COMPILER ${ARM_CXX_COMPILER} CACHE INTERNAL "CMAKE_CXX_COMPILER")

add_compile_options(-mcpu=arm926ej-s -mthumb-interwork -msoft-float -mlittle-endian -ffreestanding -ffunction-sections -fdata-sections)
include_directories(${PMB887X_LIB_PATH})
add_link_options(-Wl,-z,max-page-size=1 -ffreestanding -nostartfiles -Wl,--gc-sections)
add_compile_definitions(BOARD_${BOARD})

if (BOOT STREQUAL "intram")
	add_compile_definitions(BOOT_INTRAM)
	add_link_options(-Wl,-T,${PMB887X_LIB_PATH}/ld/intram.ld)
elseif (BOOT STREQUAL "extram")
	add_compile_definitions(BOOT_EXTRAM)
	add_link_options(-Wl,-T,${PMB887X_LIB_PATH}/ld/extram.ld)
elseif (BOOT STREQUAL "flash")
	add_compile_definitions(BOOT_FLASH)
	add_link_options(-Wl,-T,${PMB887X_LIB_PATH}/ld/flash)
endif()

function(target_output_bin target)
	set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE OFF)
	set_property(TARGET ${target} PROPERTY SUFFIX ".elf")
    set(input_name "${CMAKE_CURRENT_BINARY_DIR}/${target}.elf")
    set(output_name "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O binary ${input_name} ${output_name}
        COMMENT "Generating binary file: ${output_name}"
    )
endfunction()
