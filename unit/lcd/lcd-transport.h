#pragma once

#include <stdbool.h>
#include <stdint.h>

void lcd_transport_init(void);
void lcd_transport_reset_controller(void);
bool lcd_transport_sync_parallel_interface(void);
bool lcd_transport_write_command(uint8_t command);
bool lcd_transport_write_data(const uint8_t *data, uint32_t size);
bool lcd_transport_read_data(uint8_t *data, uint32_t size);
bool lcd_transport_begin_data_stream(void);
bool lcd_transport_write_data_word(uint16_t data);
bool lcd_transport_end_data_stream(void);
