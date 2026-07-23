#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lcd/lcd-controller.h"

struct s1d13732_display_detect_result {
	enum lcd_controller_type controller_type;
	uint8_t sampled_patterns;
	uint8_t high_patterns;
};

void s1d13732_init(void);
struct s1d13732_display_detect_result s1d13732_detect_display(void);
void s1d13732_enable_memory_interface(void);
void s1d13732_select_display(bool selected);
uint16_t s1d13732_read_register(uint16_t reg);
void s1d13732_write_register(uint16_t reg, uint16_t value);
void s1d13732_read_memory16(uint32_t address, uint16_t *data, uint32_t count);
void s1d13732_write_memory16(uint32_t address, const uint16_t *data, uint32_t count);
void s1d13732_begin_memory_write16(uint32_t address);
void s1d13732_write_memory_word16(uint16_t data);
void s1d13732_end_memory_write16(void);
void s1d13732_reset_display(void);
void s1d13732_write_display_command(uint8_t command);
void s1d13732_write_display_data(uint8_t data);
void s1d13732_begin_display_data_stream(void);
void s1d13732_write_display_word(uint16_t data);
void s1d13732_end_display_data_stream(void);
