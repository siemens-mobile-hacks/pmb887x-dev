cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ../lib/toolchain.cmake)

project(unit LANGUAGES C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
option(TEST_COLOR "Use ANSI colors in test output" OFF)

add_subdirectory(${PMB887X_LIB_PATH} libpmb887x)

add_library(test_support OBJECT test.c)
target_compile_options(test_support PUBLIC -Os -Wall -Wextra -Werror)
target_include_directories(test_support PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(TEST_COLOR)
	target_compile_definitions(test_support PUBLIC TEST_COLOR=1)
endif()

function(add_peripheral_test name)
	add_executable(${name} ${ARGN})
	target_compile_options(${name} PUBLIC -Os -Wall -Wextra -Werror)
	target_link_libraries(${name} PUBLIC pmb887x test_support)
	target_output_bin(${name})
endfunction()

add_peripheral_test(stm stm/test.c)
add_peripheral_test(dmac dmac/test.c)
add_peripheral_test(dsp dsp/test.c)
add_peripheral_test(dsp-handshake dsp/handshake.c)
add_peripheral_test(dsp-instructions dsp/instructions.c)
add_peripheral_test(dsp-opcode-aliases dsp/instructions.c)
target_compile_definitions(dsp-opcode-aliases PRIVATE
	DSP_INSTRUCTION_CASES_INCLUDE="opcode-aliases/instructions-cases.inc"
	DSP_INSTRUCTION_GOLDEN_INCLUDE="opcode-aliases/instructions-golden-packed.inc"
	DSP_INSTRUCTION_IMAGES_INCLUDE="opcode-aliases/instructions-images-packed.inc"
	DSP_INSTRUCTION_TEST_TITLE="DSP TeakLite I opcode alias test"
)
set(DSP_OPCODE_PROBE_FIRST "0x0000" CACHE STRING "First TeakLite opcode included in the raw hardware probe")
set(DSP_OPCODE_PROBE_LAST "0x00FF" CACHE STRING "Last TeakLite opcode included in the raw hardware probe")
add_peripheral_test(dsp-opcode-probe dsp/opcode-probe.c)
target_compile_definitions(dsp-opcode-probe PRIVATE
	DSP_OPCODE_PROBE_FIRST=${DSP_OPCODE_PROBE_FIRST}
	DSP_OPCODE_PROBE_LAST=${DSP_OPCODE_PROBE_LAST}
)
set(DSP_EXPANSION_PROBE_FAMILY_FIRST "0" CACHE STRING "First TeakLite two-word family included in the expansion probe")
set(DSP_EXPANSION_PROBE_FAMILY_LAST "0" CACHE STRING "Last TeakLite two-word family included in the expansion probe")
set(DSP_EXPANSION_PROBE_WORD_FIRST "0x0000" CACHE STRING "First expansion word included in the expansion probe")
set(DSP_EXPANSION_PROBE_WORD_LAST "0x00FF" CACHE STRING "Last expansion word included in the expansion probe")
add_peripheral_test(dsp-expansion-probe dsp/opcode-probe.c)
target_compile_definitions(dsp-expansion-probe PRIVATE
	DSP_EXPANSION_PROBE=1
	DSP_EXPANSION_PROBE_FAMILY_FIRST=${DSP_EXPANSION_PROBE_FAMILY_FIRST}
	DSP_EXPANSION_PROBE_FAMILY_LAST=${DSP_EXPANSION_PROBE_FAMILY_LAST}
	DSP_EXPANSION_PROBE_WORD_FIRST=${DSP_EXPANSION_PROBE_WORD_FIRST}
	DSP_EXPANSION_PROBE_WORD_LAST=${DSP_EXPANSION_PROBE_WORD_LAST}
)
add_peripheral_test(dsp-irqs dsp/irqs.c)
add_peripheral_test(dsp-mask-rom-commands dsp/commands.c)
add_peripheral_test(dsp-peripheral-map dsp/peripheral-map.c)
add_peripheral_test(dsp-peripherals-registers dsp/safe-peripherals.c)
add_peripheral_test(dsp-benchmark dsp/benchmark.c dsp/dsp-hw.c)
add_peripheral_test(dsp-irq-shared-benchmark dsp/irq-shared-benchmark.c dsp/dsp-hw.c)
add_peripheral_test(dsp-timers-functional dsp/timers-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-mcs-functional dsp/mcs-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-interrupt-functional dsp/interrupt-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-interrupt-sources dsp/interrupt-sources.c dsp/dsp-hw.c)
add_peripheral_test(dsp-ssc-functional dsp/ssc-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-cipher-functional dsp/cipher-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-cipher-a53-vectors dsp/cipher-a53-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-cipher-kgcore-vectors dsp/cipher-kgcore-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-channel-decoder-functional dsp/channel-decoder-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-channel-decoder-vectors dsp/channel-decoder-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-equalizer-functional dsp/equalizer-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-equalizer-vectors dsp/equalizer-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-equalizer-output dsp/equalizer-output.c dsp/dsp-hw.c)
add_peripheral_test(dsp-equalizer-burst dsp/equalizer-burst.c dsp/dsp-hw.c)
foreach(I2S_INSTANCE RANGE 1 3)
	add_peripheral_test(dsp-i2s${I2S_INSTANCE}-functional dsp/i2s-functional.c dsp/dsp-hw.c)
	target_compile_definitions(dsp-i2s${I2S_INSTANCE}-functional PRIVATE I2S_INSTANCE=${I2S_INSTANCE})
endforeach()
add_peripheral_test(dsp-afe-functional dsp/afe-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-modulator-functional dsp/modulator-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-baseband-functional dsp/baseband-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-tpu-events dsp/tpu-events.c dsp/dsp-hw.c)
add_peripheral_test(dsp-tpu-bb-events dsp/tpu-bb-events.c dsp/dsp-hw.c)
add_peripheral_test(dsp-l1mon-vectors dsp/l1mon-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-l1mon-functional dsp/l1mon-functional.c dsp/dsp-hw.c)
add_peripheral_test(
	dsp-gsm-channel-scan
	dsp/gsm-channel-scan.c
	dsp/board/siemens-x85.c
	dsp/board/hd155153np.c
	dsp/dsp-hw.c
	eeprom.c
)
add_peripheral_test(dsp-boot-state dsp/boot-state.c dsp/dsp-hw.c)
add_peripheral_test(dsp-control-functional dsp/dsp-control-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-io-functional dsp/dsp-io-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-rom-dump dsp/rom-dump.c)
add_peripheral_test(dsp-data-rom-dump dsp/rom-dump.c)
target_compile_definitions(dsp-data-rom-dump PRIVATE DSP_DUMP_DATA_ROM=1)
add_peripheral_test(ebu ebu/test.c)
add_peripheral_test(gpio gpio/test.c)
add_peripheral_test(gpio-mon gpio/monitor.c)
add_peripheral_test(gprscu gprscu/test.c)
add_peripheral_test(keypad keypad/test.c)
add_peripheral_test(mod mod/test.c)
add_peripheral_test(mmicif mmicif/test.c mmicif/host.c)
add_peripheral_test(scu scu/test.c scu/watchdog.c)
add_peripheral_test(scu-reset scu/reset.c scu/watchdog.c)
add_peripheral_test(scu-wdt-reset scu/wdt-reset.c scu/watchdog.c)
add_peripheral_test(adc adc/test.c)
add_peripheral_test(afc afc/test.c)
get_directory_property(DEFAULT_LINK_OPTIONS LINK_OPTIONS)
set(CGU_LINK_OPTIONS)
foreach(LINK_OPTION IN LISTS DEFAULT_LINK_OPTIONS)
	if(NOT LINK_OPTION MATCHES "/ld/(extram|intram|flash)\\.ld$")
		list(APPEND CGU_LINK_OPTIONS "${LINK_OPTION}")
	endif()
endforeach()
list(APPEND CGU_LINK_OPTIONS "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/cgu/extram-intram.ld")
set_directory_properties(PROPERTIES LINK_OPTIONS "${CGU_LINK_OPTIONS}")
add_peripheral_test(cgu cgu/test.c cgu/measure.S cgu/loader.S)
set_directory_properties(PROPERTIES LINK_OPTIONS "${DEFAULT_LINK_OPTIONS}")
add_peripheral_test(rtc rtc/test.c)
add_peripheral_test(vic vic/test.c)
add_peripheral_test(gptu gptu/test.c)
add_peripheral_test(sccu sccu/test.c)
add_peripheral_test(tpu tpu/test.c)
add_peripheral_test(tpu-gp tpu/gp.c)
add_peripheral_test(tpu-ram tpu/ram.c)
add_peripheral_test(tpu-rf-ssc tpu/rf-ssc.c dsp/rf-ssc-tcm.c dsp/rf-ssc-tcm.S)
add_peripheral_test(tcm tcm/test.c tcm/execution.S)
add_peripheral_test(usb usb/test.c)
add_peripheral_test(i2c-v1 i2c-v1/test.c i2c-v1/i2c-v1.c)
add_peripheral_test(i2c-v2 i2c-v2/test.c i2c-v2/i2c-v2.c)
add_peripheral_test(i2c-v2-dma i2c-v2/test-dma.c i2c-v2/i2c-v2.c)
add_peripheral_test(i2c-fm-radio i2c-fm-radio/test.c i2c-v2/i2c-v2.c)
target_include_directories(i2c-fm-radio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/i2c-v2)
add_peripheral_test(ssc ssc/test.c ssc/ssc.c)
add_peripheral_test(ssc-dma ssc/test-dma.c ssc/ssc.c)
add_peripheral_test(dif-v1 dif-v1/test.c dif-v1/dif-v1.c dif-v1/common.c)
add_peripheral_test(dif-v1-dma dif-v1/test-dma.c dif-v1/dma.c dif-v1/common.c)
add_peripheral_test(
	dif-v1-lcd
	dif-v1/lcd.c
	dif-v1/lcd-board.c
	dif-v1/lcd-transport.c
	dif-v1/s1d13732.c
	lcd/lcd-controller-test.c
	lcd/pcf8882.c
	lcd/ssd1286.c
)
add_peripheral_test(
	dif-v1-lcd-if
	dif-v1/lcd-if.c
	dif-v1/s1d13732.c
)
add_peripheral_test(usart usart/test.c usart/usart.c)
add_peripheral_test(usart-dma usart/test-dma.c usart/usart.c)
add_peripheral_test(dif-v2 dif-v2/test.c dif-v2/dif-v2.c)
add_peripheral_test(dif-v2-dma dif-v2/test-dma.c dif-v2/dif-v2.c)
add_peripheral_test(
	dif-v2-lcd
	dif-v2/lcd-test.c
	dif-v2/lcd-board.c
	dif-v2/lcd-transport.c
	lcd/lcd-controller.c
	lcd/l5f30539p00.c
	lcd/jbt6k71.c
)
add_peripheral_test(
	dif-v2-lcd-if
	dif-v2/lcd.c
	dif-v2/lcd-board.c
	lcd/lcd-controller.c
	lcd/l5f30539p00.c
	lcd/jbt6k71.c
)
add_peripheral_test(sim sim/test.c)
add_peripheral_test(sim-dma sim/test-dma.c)
add_peripheral_test(sim-card sim/test-card.c)
add_peripheral_test(sim-card-dma sim/test-card.c)
target_compile_definitions(sim-card-dma PRIVATE SIM_HARDWARE_T0=1)
add_peripheral_test(sim-card-timers sim/test-card.c)
target_compile_definitions(sim-card-timers PRIVATE SIM_TIMER_TEST=1)
add_peripheral_test(sim-card-info sim/test-card.c)
target_compile_definitions(sim-card-info PRIVATE SIM_DUMP_CARD_INFO=1)
add_peripheral_test(sim-no-card sim/test-card.c)
target_compile_definitions(sim-no-card PRIVATE SIM_NO_CARD=1)
add_peripheral_test(cfi-intel cfi-intel/test.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-rw cfi-intel/rw.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-buffer-abort cfi-intel/buffer-abort.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-efa-rw cfi-intel/efa-rw.c cfi-intel/cfi.c)
add_peripheral_test(cfi-intel-otp-rw cfi-intel/otp-rw.c cfi-intel/cfi.c)
