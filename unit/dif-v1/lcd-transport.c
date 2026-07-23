#include <stddef.h>

#include "lcd/lcd-transport.h"
#include "s1d13732.h"

void lcd_transport_init(void) {
	s1d13732_init();
}

void lcd_transport_reset_controller(void) {
	s1d13732_reset_display();
}

bool lcd_transport_sync_parallel_interface(void) {
	return true;
}

bool lcd_transport_write_command(uint8_t command) {
	s1d13732_select_display(true);
	s1d13732_write_display_command(command);
	return true;
}

bool lcd_transport_write_data(const uint8_t *data, uint32_t size) {
	s1d13732_select_display(true);
	for (uint32_t i = 0; i < size; i++)
		s1d13732_write_display_data(data[i]);
	return true;
}

bool lcd_transport_read_data(uint8_t *data, uint32_t size) {
	/* The serial host transport exposes no known read transaction for the parallel PCF8882 panel. */
	(void) data;
	return size == 0;
}

bool lcd_transport_begin_data_stream(void) {
	s1d13732_select_display(true);
	s1d13732_begin_display_data_stream();
	return true;
}

bool lcd_transport_write_data_word(uint16_t data) {
	s1d13732_write_display_word(data);
	return true;
}

bool lcd_transport_end_data_stream(void) {
	s1d13732_end_display_data_stream();
	return true;
}
