#include <pmb887x.h>
#include <lcd/JBT6K71.h>

#include "lcd-controller.h"
#include "lcd-transport.h"

static enum lcd_pixel_format current_format = LCD_PIXEL_FORMAT_RGB565;
static struct lcd_address_mode current_mode;

static const struct jbt_register {
	uint16_t index;
	uint16_t value;
} JBT_INITIAL_STATE[] = {
	{ JBT6K71_OSCILLATION, 0x0001 },
	{ JBT6K71_DRIVER_OUTPUT_CONTROL, 0x0027 },
	{ JBT6K71_LCD_DRIVING_SIGNAL, 0x0200 },
	{ JBT6K71_ENTRY_MODE, 0x0120 },
	{ JBT6K71_DISPLAY_MODE_1, 0x4004 },
	{ JBT6K71_FR_FREQUENCY_ADJUSTMENT, 0x0011 },
	{ JBT6K71_LTPS_CONTROL_1, 0x0303 },
	{ JBT6K71_LTPS_CONTROL_2, 0x0102 },
	{ JBT6K71_AMPLIFIER_CAPABILITY, 0x0000 },
	{ JBT6K71_POWER_SUPPLY_CONTROL_1, 0x00F6 },
	{ JBT6K71_POWER_SUPPLY_CONTROL_2, 0x0007 },
	{ JBT6K71_POWER_SUPPLY_CONTROL_4, 0x0111 },
	{ JBT6K71_GRAY_SCALE_1, 0x0200 },
	{ JBT6K71_GRAY_SCALE_2, 0x0002 },
	{ JBT6K71_GRAY_SCALE_3, 0x0000 },
	{ JBT6K71_GRAY_SCALE_4, 0x0300 },
	{ JBT6K71_GRAY_SCALE_5, 0x0700 },
	{ JBT6K71_BLUE_OFFSET, 0x0070 },
	{ JBT6K71_SCREEN1_DRIVE_START, 0x0000 },
	{ JBT6K71_SCREEN1_DRIVE_END, 0x013F },
	{ JBT6K71_HORIZONTAL_RAM_START, 0x0000 },
	{ JBT6K71_HORIZONTAL_RAM_END, 0x00EF },
	{ JBT6K71_VERTICAL_RAM_START, 0x0000 },
	{ JBT6K71_VERTICAL_RAM_END, 0x013F },
	{ JBT6K71_RAM_ADDRESS_LOW, 0x00EF },
	{ JBT6K71_RAM_ADDRESS_HIGH, 0x0000 },
};

static bool jbt_write_command(uint16_t command) {
	return lcd_transport_write_command(command >> 8) && lcd_transport_write_command(command);
}

static bool jbt_write_register(uint16_t index, uint16_t value) {
	uint8_t data[] = { value >> 8, value };

	return jbt_write_command(index) && lcd_transport_write_data(data, sizeof(data));
}

static uint16_t jbt_entry_mode(void) {
	uint16_t value = (current_mode.reverse_x ? 0 : 1U << JBT6K71_ENTRY_MODE_ID_SHIFT) |
		(current_mode.reverse_y ? 0 : 2U << JBT6K71_ENTRY_MODE_ID_SHIFT) |
		(current_mode.swap_axes ? JBT6K71_ENTRY_MODE_AM : 0) |
		(current_mode.bgr ? JBT6K71_ENTRY_MODE_BGR : 0);

	if (current_format == LCD_PIXEL_FORMAT_RGB666_8_8_2)
		value |= JBT6K71_ENTRY_MODE_TRI | 1U << JBT6K71_ENTRY_MODE_DFM_SHIFT;
	else if (current_format == LCD_PIXEL_FORMAT_RGB666_2_8_8)
		value |= JBT6K71_ENTRY_MODE_TRI | 2U << JBT6K71_ENTRY_MODE_DFM_SHIFT;
	else if (current_format == LCD_PIXEL_FORMAT_RGB666)
		value |= JBT6K71_ENTRY_MODE_TRI | JBT6K71_ENTRY_MODE_DFM;
	return value;
}

static bool jbt_apply_entry_mode(void) {
	return jbt_write_register(JBT6K71_ENTRY_MODE, jbt_entry_mode());
}

static bool jbt_set_pixel_format(enum lcd_pixel_format format) {
	if (format != LCD_PIXEL_FORMAT_RGB565 && format != LCD_PIXEL_FORMAT_RGB666_8_8_2 &&
		format != LCD_PIXEL_FORMAT_RGB666_2_8_8 && format != LCD_PIXEL_FORMAT_RGB666)
		return false;
	current_format = format;
	return jbt_apply_entry_mode();
}

static bool jbt_set_address_mode(const struct lcd_address_mode *mode) {
	current_mode = *mode;
	return jbt_apply_entry_mode();
}

static bool jbt_set_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	uint16_t current_x = current_mode.reverse_x ? x_end : x_start;
	uint16_t current_y = current_mode.reverse_y ? y_end : y_start;

	return jbt_write_register(JBT6K71_HORIZONTAL_RAM_START, x_start) &&
		jbt_write_register(JBT6K71_HORIZONTAL_RAM_END, x_end) &&
		jbt_write_register(JBT6K71_VERTICAL_RAM_START, y_start) &&
		jbt_write_register(JBT6K71_VERTICAL_RAM_END, y_end) &&
		jbt_write_register(JBT6K71_RAM_ADDRESS_LOW, current_x) &&
		jbt_write_register(JBT6K71_RAM_ADDRESS_HIGH, current_y);
}

static bool jbt_set_cursor(uint16_t x, uint16_t y) {
	return jbt_write_register(JBT6K71_RAM_ADDRESS_LOW, x) && jbt_write_register(JBT6K71_RAM_ADDRESS_HIGH, y);
}

static bool jbt_write_pixels(const struct lcd_color *colors, uint32_t count) {
	if (!jbt_write_command(JBT6K71_GRAM_DATA))
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (current_format == LCD_PIXEL_FORMAT_RGB565) {
			uint16_t pixel = ((uint16_t)(colors[i].red >> 1) << 11) |
				((uint16_t)colors[i].green << 5) | (colors[i].blue >> 1);
			uint8_t data[] = { pixel >> 8, pixel };

			if (!lcd_transport_write_data(data, sizeof(data)))
				return false;
		} else if (current_format == LCD_PIXEL_FORMAT_RGB666_8_8_2) {
			uint8_t data[] = {
				colors[i].red << 2 | colors[i].green >> 4,
				colors[i].green << 4 | colors[i].blue >> 2,
				colors[i].blue << 6,
			};

			if (!lcd_transport_write_data(data, sizeof(data)))
				return false;
		} else if (current_format == LCD_PIXEL_FORMAT_RGB666_2_8_8) {
			uint8_t data[] = {
				colors[i].red >> 4,
				colors[i].red << 4 | colors[i].green >> 2,
				colors[i].green << 6 | colors[i].blue,
			};

			if (!lcd_transport_write_data(data, sizeof(data)))
				return false;
		} else {
			uint8_t data[] = { colors[i].red << 2, colors[i].green << 2, colors[i].blue << 2 };

			if (!lcd_transport_write_data(data, sizeof(data)))
				return false;
		}
	}
	return true;
}

static uint8_t jbt_expand_5bit(uint8_t value) {
	return (value << 1) | (value >> 4);
}

static bool jbt_read_pixels(struct lcd_color *colors, uint32_t count) {
	uint32_t bytes_per_pixel = current_format == LCD_PIXEL_FORMAT_RGB565 ? 2 : 3;
	uint8_t data[3];

	if (!jbt_write_command(JBT6K71_GRAM_DATA))
		return false;
	// JBT6K71 outputs one complete dummy pixel, so the dummy length follows the active pixel format.
	if (!lcd_transport_read_data(data, bytes_per_pixel))
		return false;
	for (uint32_t i = 0; i < count; i++) {
		if (!lcd_transport_read_data(data, bytes_per_pixel))
			return false;

		if (current_format == LCD_PIXEL_FORMAT_RGB565) {
			uint16_t pixel = ((uint16_t)data[0] << 8) | data[1];
			colors[i] = (struct lcd_color) {
				.red = jbt_expand_5bit(pixel >> 11),
				.green = (pixel >> 5) & 0x3F,
				.blue = jbt_expand_5bit(pixel & 0x1F),
			};
		} else if (current_format == LCD_PIXEL_FORMAT_RGB666_8_8_2) {
			colors[i] = (struct lcd_color) {
				.red = data[0] >> 2,
				.green = (data[0] & 0x03) << 4 | data[1] >> 4,
				.blue = (data[1] & 0x0F) << 2 | data[2] >> 6,
			};
		} else if (current_format == LCD_PIXEL_FORMAT_RGB666_2_8_8) {
			colors[i] = (struct lcd_color) {
				.red = (data[0] & 0x03) << 4 | data[1] >> 4,
				.green = (data[1] & 0x0F) << 2 | data[2] >> 6,
				.blue = data[2] & 0x3F,
			};
		} else {
			colors[i] = (struct lcd_color) {
				.red = data[0] >> 2,
				.green = data[1] >> 2,
				.blue = data[2] >> 2,
			};
		}
	}
	return true;
}

static void jbt_quantize_color(
	enum lcd_pixel_format format,
	const struct lcd_color *input,
	struct lcd_color *output
) {
	*output = *input;
	if (format == LCD_PIXEL_FORMAT_RGB565) {
		output->red = jbt_expand_5bit(input->red >> 1);
		output->blue = jbt_expand_5bit(input->blue >> 1);
	}
}

static bool jbt_initialize(void) {
	current_format = LCD_PIXEL_FORMAT_RGB565;
	current_mode = (struct lcd_address_mode) { 0 };
	bool success = true;

	for (uint32_t i = 0; i < 3; i++) {
		success &= jbt_write_command(JBT6K71_OSCILLATION);
		stopwatch_usleep_wd(1000);
	}
	success &= jbt_write_register(0x05FF, 0x0000);
	success &= lcd_transport_sync_parallel_interface();
	success &= jbt_write_register(JBT6K71_MODE, 0x0005);
	stopwatch_usleep_wd(1000);
	for (uint32_t i = 0; i < ARRAY_SIZE(JBT_INITIAL_STATE); i++)
		success &= jbt_write_register(JBT_INITIAL_STATE[i].index, JBT_INITIAL_STATE[i].value);
	success &= jbt_write_register(JBT6K71_DISPLAY_CONTROL, 0xC010);
	stopwatch_usleep_wd(30000);
	success &= jbt_write_register(JBT6K71_AUTO_MANAGEMENT_CONTROL, 0x0001);
	success &= jbt_write_register(JBT6K71_DISPLAY_CONTROL, 0xF7FE);

	return success && jbt_apply_entry_mode();
}

static bool jbt_probe(uint32_t *id) {
	uint8_t code[8] = { 0 };

	if (!jbt_initialize() || !jbt_write_command(JBT6K71_OSCILLATION) ||
		!lcd_transport_read_data(code, sizeof(code)))
		return false;
	for (uint32_t i = 2; i < sizeof(code); i += 2) {
		if (code[i] != code[0] || code[i + 1] != code[1])
			return false;
	}
	if (code[0] != 0x71 || code[1] != 0x14)
		return false;

	*id = 2;
	return true;
}

const struct lcd_controller lcd_controller_jbt6k71 = {
	.name = "JBT6K71",
	.type = LCD_CONTROLLER_JBT6K71,
	.id = 2,
	.width = 240,
	.height = 320,
	.pixel_formats = BIT(LCD_PIXEL_FORMAT_RGB565) | BIT(LCD_PIXEL_FORMAT_RGB666_8_8_2) |
		BIT(LCD_PIXEL_FORMAT_RGB666_2_8_8) | BIT(LCD_PIXEL_FORMAT_RGB666),
	.reset_settle_ms = 110,
	.gram_write_command = { JBT6K71_GRAM_DATA >> 8, JBT6K71_GRAM_DATA & 0xFF },
	.gram_write_command_size = 2,
	.gram_read_command = { JBT6K71_GRAM_DATA >> 8, JBT6K71_GRAM_DATA & 0xFF },
	.gram_read_command_size = 2,
	.rgb565_read_dummy_bytes = 2,
	// ID0/ID1 reverse counters within the window; AM selects the primary counter.
	.swap_axes_changes_gram_order = true,
	.reverse_x_mirrors_coordinates = false,
	.reverse_y_mirrors_coordinates = false,
	.probe = jbt_probe,
	.initialize = jbt_initialize,
	.quantize_color = jbt_quantize_color,
	.set_pixel_format = jbt_set_pixel_format,
	.set_address_mode = jbt_set_address_mode,
	.set_window = jbt_set_window,
	.set_cursor = jbt_set_cursor,
	.write_pixels = jbt_write_pixels,
	.read_pixels = jbt_read_pixels,
};
