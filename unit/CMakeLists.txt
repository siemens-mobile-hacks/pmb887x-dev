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
add_peripheral_test(stm-clock stm/clock.c cgu/clock.c)
add_peripheral_test(dmac dmac/test.c)
add_peripheral_test(dmac-clock dmac/clock.c)
add_peripheral_test(eeprom-parser eeprom/test.c eeprom.c)
add_peripheral_test(dsp-lle-core dsp/lle/test.c)
add_peripheral_test(dsp-lle-clock dsp/lle/clock.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-timer-clock dsp/lle/timer-clock.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-boot-timing dsp/lle/boot-timing.c)
add_peripheral_test(dsp-lle-instructions dsp/lle/instructions.c)
add_peripheral_test(dsp-lle-opcode-aliases dsp/lle/instructions.c)
target_compile_definitions(dsp-lle-opcode-aliases PRIVATE
	DSP_INSTRUCTION_CASES_INCLUDE="opcode-aliases/instructions-cases.inc"
	DSP_INSTRUCTION_GOLDEN_INCLUDE="opcode-aliases/instructions-golden-packed.inc"
	DSP_INSTRUCTION_IMAGES_INCLUDE="opcode-aliases/instructions-images-packed.inc"
	DSP_INSTRUCTION_TEST_TITLE="DSP TeakLite I opcode alias test"
)
set(DSP_OPCODE_PROBE_FIRST "0x0000" CACHE STRING "First TeakLite opcode included in the raw hardware probe")
set(DSP_OPCODE_PROBE_LAST "0x00FF" CACHE STRING "Last TeakLite opcode included in the raw hardware probe")
add_peripheral_test(dsp-lle-opcode-probe dsp/lle/opcode-probe.c)
target_compile_definitions(dsp-lle-opcode-probe PRIVATE
	DSP_OPCODE_PROBE_FIRST=${DSP_OPCODE_PROBE_FIRST}
	DSP_OPCODE_PROBE_LAST=${DSP_OPCODE_PROBE_LAST}
)
set(DSP_EXPANSION_PROBE_FAMILY_FIRST "0" CACHE STRING "First TeakLite two-word family included in the expansion probe")
set(DSP_EXPANSION_PROBE_FAMILY_LAST "0" CACHE STRING "Last TeakLite two-word family included in the expansion probe")
set(DSP_EXPANSION_PROBE_WORD_FIRST "0x0000" CACHE STRING "First expansion word included in the expansion probe")
set(DSP_EXPANSION_PROBE_WORD_LAST "0x00FF" CACHE STRING "Last expansion word included in the expansion probe")
add_peripheral_test(dsp-lle-expansion-probe dsp/lle/opcode-probe.c)
target_compile_definitions(dsp-lle-expansion-probe PRIVATE
	DSP_EXPANSION_PROBE=1
	DSP_EXPANSION_PROBE_FAMILY_FIRST=${DSP_EXPANSION_PROBE_FAMILY_FIRST}
	DSP_EXPANSION_PROBE_FAMILY_LAST=${DSP_EXPANSION_PROBE_FAMILY_LAST}
	DSP_EXPANSION_PROBE_WORD_FIRST=${DSP_EXPANSION_PROBE_WORD_FIRST}
	DSP_EXPANSION_PROBE_WORD_LAST=${DSP_EXPANSION_PROBE_WORD_LAST}
)
add_peripheral_test(dsp-lle-irqs dsp/lle/irqs.c)
add_peripheral_test(dsp-lle-mask-rom-commands dsp/lle/commands.c)
add_peripheral_test(dsp-hle-runtime-firmware dsp/hle/runtime-firmware.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
add_peripheral_test(dsp-hle-runtime-transport dsp/hle/runtime-transport.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
add_peripheral_test(dsp-hle-runtime-notifications dsp/hle/runtime-notifications.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
add_peripheral_test(dsp-hle-runtime-semaphores dsp/hle/runtime-semaphores.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
add_peripheral_test(dsp-hle-runtime-idle dsp/hle/runtime-idle.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
set(DSP_AUDIO_BOARD_SOURCE)
if(BOARD STREQUAL "siemens-cx75")
	set(DSP_AUDIO_BOARD_SOURCE dsp/audio/board/siemens/cx75.c)
elseif(BOARD STREQUAL "siemens-e71")
	set(DSP_AUDIO_BOARD_SOURCE dsp/audio/board/siemens/e71.c)
elseif(BOARD STREQUAL "siemens-el71")
	set(DSP_AUDIO_BOARD_SOURCE dsp/audio/board/siemens/el71.c)
endif()
set(DSP_AUDIO_SOURCES
	dsp/audio/board/siemens/commands.c
	dsp/audio/board/siemens/pasic.c
	dsp/audio/board/siemens/siemens.c
	dsp/audio/pcm.c
	${DSP_AUDIO_BOARD_SOURCE}
	dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
if(DSP_AUDIO_BOARD_SOURCE)
	add_peripheral_test(dsp-hle-i2s-playback dsp/audio/i2s-playback/test.c ${DSP_AUDIO_SOURCES})
endif()
if(BOARD STREQUAL "siemens-el71")
	add_peripheral_test(dsp-hle-afe-playback dsp/audio/afe-playback/test.c ${DSP_AUDIO_SOURCES})
endif()
add_peripheral_test(dsp-hle-startup-transport dsp/hle/startup-transport.c dsp/runtime.c dsp/hle/user-rom.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-peripheral-map dsp/lle/peripheral-map.c)
add_peripheral_test(dsp-lle-peripherals-registers dsp/lle/safe-peripherals.c)
add_peripheral_test(dsp-lle-benchmark dsp/lle/benchmark.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-irq-shared-benchmark dsp/lle/irq-shared-benchmark.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-timers-functional dsp/lle/timers-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-mcs-functional dsp/lle/mcs-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-delay-slot-functional dsp/lle/delay-slot-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-interrupt-functional dsp/lle/interrupt-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-interrupt-block-repeat dsp/lle/interrupt-block-repeat.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-clock-domains dsp/lle/clock-domains.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-interrupt-sources dsp/lle/interrupt-sources.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-ssc-functional dsp/lle/ssc-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-cipher-functional dsp/lle/cipher-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-cipher-a53-vectors dsp/lle/cipher-a53-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-cipher-kgcore-vectors dsp/lle/cipher-kgcore-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-channel-decoder-functional dsp/lle/channel-decoder-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-channel-decoder-vectors dsp/lle/channel-decoder-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-sch-decoder-vectors dsp/lle/sch-decoder-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-equalizer-functional dsp/lle/equalizer-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-equalizer-vectors dsp/lle/equalizer-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-equalizer-gmsk-vectors dsp/lle/equalizer-gmsk-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-equalizer-output dsp/lle/equalizer-output.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-equalizer-burst dsp/lle/equalizer-burst.c dsp/dsp-hw.c)
foreach(I2S_INSTANCE RANGE 1 3)
	add_peripheral_test(dsp-lle-i2s${I2S_INSTANCE}-functional dsp/lle/i2s-functional.c dsp/dsp-hw.c)
	target_compile_definitions(dsp-lle-i2s${I2S_INSTANCE}-functional PRIVATE I2S_INSTANCE=${I2S_INSTANCE})
endforeach()
add_peripheral_test(dsp-lle-afe-functional dsp/lle/afe-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-modulator-functional dsp/lle/modulator-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-baseband-functional dsp/lle/baseband-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-tpu-events dsp/lle/tpu-events.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-tpu-bb-events dsp/lle/tpu-bb-events.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-l1mon-vectors dsp/lle/l1mon-vectors.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-l1mon-functional dsp/lle/l1mon-functional.c dsp/dsp-hw.c)
add_peripheral_test(
	dsp-hle-gsm-channel-scan
	dsp/hle/gsm-channel-scan.c
	dsp/hle/gsm-l1/gsm-l1.c
	dsp/hle/gsm-l1/board/siemens-x85.c
	dsp/hle/gsm-l1/board/hd155153np.c
	dsp/runtime.c
	dsp/hle/user-rom.c
	dsp/dsp-hw.c
	eeprom.c
)
add_peripheral_test(dsp-lle-boot-state dsp/lle/boot-state.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-control-functional dsp/lle/dsp-control-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-program-banks dsp/lle/program-banks.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-io-functional dsp/lle/dsp-io-functional.c dsp/dsp-hw.c)
add_peripheral_test(dsp-lle-rom-dump dsp/lle/rom-dump.c)
add_peripheral_test(dsp-lle-data-rom-dump dsp/lle/rom-dump.c)
target_compile_definitions(dsp-lle-data-rom-dump PRIVATE DSP_DUMP_DATA_ROM=1)
add_peripheral_test(ebu ebu/test.c)
add_peripheral_test(ebu-clock ebu/clock.c cgu/clock.c)
add_peripheral_test(gpio gpio/test.c)
add_peripheral_test(gpio-mon gpio/monitor.c)
add_peripheral_test(gprscu gprscu/test.c)
add_peripheral_test(keypad keypad/test.c)
add_peripheral_test(mod mod/test.c)
add_peripheral_test(mmicif mmicif/test.c mmicif/host.c)
add_peripheral_test(scu scu/test.c scu/watchdog.c)
add_peripheral_test(scu-reset scu/reset.c scu/watchdog.c)
add_peripheral_test(scu-wdt-reset scu/wdt-reset.c scu/watchdog.c)
add_peripheral_test(scu-wdt-clock scu/wdt-clock.c scu/watchdog.c cgu/clock.c)
add_peripheral_test(adc adc/test.c)
add_peripheral_test(afc afc/test.c)
add_peripheral_test(cgu-ahb cgu/ahb.c cgu/clock.c cgu/measure.S)
add_peripheral_test(cgu-pll cgu/pll.c)
add_peripheral_test(cgu-fsys cgu/fsys.c cgu/clock.c)
add_peripheral_test(cgu-cpu cgu/cpu.c)
add_peripheral_test(capcom capcom/test.c)
add_peripheral_test(capcom-capture capcom/capture.c)
add_peripheral_test(capcom-compare capcom/compare.c)
add_peripheral_test(capcom-interrupts capcom/interrupts.c)
add_peripheral_test(capcom-clock capcom/clock.c cgu/clock.c)
add_peripheral_test(rtc rtc/test.c)
add_peripheral_test(rtc-clock rtc/clock.c)
add_peripheral_test(vic vic/test.c)
add_peripheral_test(gptu gptu/test.c)
add_peripheral_test(gptu-firmware gptu/firmware.c)
add_peripheral_test(gptu-interrupts gptu/interrupts.c)
add_peripheral_test(gptu-clock gptu/clock.c cgu/clock.c)
add_peripheral_test(sccu sccu/test.c)
add_peripheral_test(sccu-clock sccu/clock.c cgu/clock.c)
add_peripheral_test(tpu tpu/test.c)
add_peripheral_test(tpu-gp tpu/gp.c)
add_peripheral_test(tpu-ram tpu/ram.c)
add_peripheral_test(tpu-rf-ssc tpu/rf-ssc.c dsp/rf-ssc-tcm.c dsp/rf-ssc-tcm.S)
add_peripheral_test(tpu-baseband-clock tpu/baseband-clock.c dsp/dsp-hw.c cgu/clock.c)
add_peripheral_test(tpu-frame-timing tpu/frame-timing.c)
add_peripheral_test(tcm tcm/test.c tcm/execution.S)
add_peripheral_test(mmci-clock mmci/clock.c cgu/clock.c)
add_peripheral_test(usif-clock usif/clock.c)
add_peripheral_test(usb usb/test.c)
add_peripheral_test(i2c-v1 i2c-v1/test.c i2c-v1/i2c-v1.c)
add_peripheral_test(i2c-v2 i2c-v2/test.c i2c-v2/i2c-v2.c i2c-v2/driver.c)
add_peripheral_test(i2c-v2-dma i2c-v2/test-dma.c i2c-v2/i2c-v2.c i2c-v2/driver.c)
add_peripheral_test(pmic-pmb6812-pec pmic-pmb6812-pec/test.c)
add_peripheral_test(i2c-fm-radio i2c-fm-radio/test.c)
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
add_peripheral_test(usart-clock usart/clock.c cgu/clock.c)
add_peripheral_test(usart-autobaud usart/autobaud.c)
add_peripheral_test(usart-host-rx usart/host-rx.c)
add_peripheral_test(usart-irq-rx usart/irq-rx.c)
add_peripheral_test(usart-flow-control usart/flow-control.c)
add_peripheral_test(dif-v2 dif-v2/test.c dif-v2/dif-v2.c)
add_peripheral_test(dif-v2-dma dif-v2/test-dma.c dif-v2/dif-v2.c)
add_peripheral_test(
	dif-v2-lcd
	dif-v2/lcd-test.c
	dif-v2/lcd-board.c
	dif-v2/lcd-transport.c
	lcd/lcd-controller.c
	lcd/r61505.c
	lcd/l5f30539p00.c
	lcd/jbt6k71.c
)
add_peripheral_test(
	dif-v2-lcd-if
	dif-v2/lcd.c
	dif-v2/lcd-board.c
	lcd/lcd-controller.c
	lcd/r61505.c
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
