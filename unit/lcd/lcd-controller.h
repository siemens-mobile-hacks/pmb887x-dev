#pragma once

#include <stdbool.h>
#include <stdint.h>

enum lcd_pixel_format {
	LCD_PIXEL_FORMAT_RGB565,
	LCD_PIXEL_FORMAT_RGB666_8_8_2,
	LCD_PIXEL_FORMAT_RGB666_2_8_8,
	LCD_PIXEL_FORMAT_RGB666,
};

enum lcd_controller_type {
	LCD_CONTROLLER_UNKNOWN,
	LCD_CONTROLLER_L5F30539P00,
	LCD_CONTROLLER_JBT6K71,
	LCD_CONTROLLER_PCF8882,
	LCD_CONTROLLER_SSD1286,
	LCD_CONTROLLER_LS020,
};

struct lcd_color {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};

struct lcd_address_mode {
	bool swap_axes;
	bool reverse_x;
	bool reverse_y;
	bool bgr;
};

struct lcd_controller {
	const char *name;
	enum lcd_controller_type type;
	uint32_t id;
	uint16_t width;
	uint16_t height;
	uint32_t pixel_formats;
	uint16_t reset_settle_ms;
	uint8_t gram_write_command[2];
	uint8_t gram_write_command_size;
	uint8_t gram_read_command[2];
	uint8_t gram_read_command_size;
	uint8_t rgb565_read_dummy_bytes;
	// Address-mode bits differ: some controllers alter traversal only, while others also remap coordinates.
	bool swap_axes_changes_gram_order;
	bool reverse_x_mirrors_coordinates;
	bool reverse_y_mirrors_coordinates;
	bool (*probe)(uint32_t *id);
	bool (*initialize)(void);
	void (*quantize_color)(enum lcd_pixel_format format, const struct lcd_color *input, struct lcd_color *output);
	bool (*set_pixel_format)(enum lcd_pixel_format format);
	bool (*set_address_mode)(const struct lcd_address_mode *mode);
	bool (*set_window)(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end);
	bool (*set_cursor)(uint16_t x, uint16_t y);
	bool (*write_pixels)(const struct lcd_color *colors, uint32_t count);
	bool (*read_pixels)(struct lcd_color *colors, uint32_t count);
};

extern const struct lcd_controller lcd_controller_l5f30539p00;
extern const struct lcd_controller lcd_controller_jbt6k71;
extern const struct lcd_controller lcd_controller_pcf8882;
extern const struct lcd_controller lcd_controller_ssd1286;

void lcd_controller_reset(const struct lcd_controller *lcd);
const struct lcd_controller *lcd_controller_detect(uint32_t *detected_id);
