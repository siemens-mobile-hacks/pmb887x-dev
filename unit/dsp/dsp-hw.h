#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DSP_HW_BOOT_MAX_WORDS 507
#define DSP_HW_STARTUP_ADDRESS 0x0100

extern volatile uint16_t *const dsp_hw_shared_memory;

bool dsp_hw_reset(void);
bool dsp_hw_read_data(uint16_t address, uint16_t *values, size_t words);
bool dsp_hw_read_program(uint16_t address, uint16_t *values, size_t words);
bool dsp_hw_read_reg(uint16_t address, uint16_t *value);
bool dsp_hw_write_reg(uint16_t address, uint16_t value);
bool dsp_hw_load_image(const uint8_t *image, size_t image_size);
bool dsp_hw_branch(uint16_t destination);
bool dsp_hw_wait_shared(size_t offset, uint16_t expected, uint32_t timeout_ms);
