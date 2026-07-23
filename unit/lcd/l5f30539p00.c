#include <pmb887x.h>

#include "lcd-controller.h"
#include "lcd-transport.h"

#define L5F_CMD_SLPOUT 0x11U
#define L5F_CMD_DISPON 0x29U
#define L5F_CMD_CASET 0x2AU
#define L5F_CMD_PASET 0x2BU
#define L5F_CMD_RAMWR 0x2CU
#define L5F_CMD_RGBSET 0x2DU
#define L5F_CMD_RAMRD 0x2EU
#define L5F_CMD_MADCTL 0x36U
#define L5F_CMD_COLMOD 0x3AU

#define L5F_MADCTL_REVERSE_Y BIT(7)
#define L5F_MADCTL_REVERSE_X BIT(6)
#define L5F_MADCTL_SWAP_AXES BIT(5)
#define L5F_MADCTL_BGR BIT(3)

#define L5F_COLMOD_RGB565 0x05U
#define L5F_COLMOD_RGB666 0x06U

static enum lcd_pixel_format current_format = LCD_PIXEL_FORMAT_RGB565;

static bool l5f_write_command_data(uint8_t command, const uint8_t *data, uint32_t size) {
	return lcd_transport_write_command(command) && lcd_transport_write_data(data, size);
}

static bool l5f_set_pixel_format(enum lcd_pixel_format format) {
	if (format == LCD_PIXEL_FORMAT_RGB565) {
		uint8_t lookup_table[128];

		for (uint32_t i = 0; i < 32; i++) {
			lookup_table[i] = i == 31 ? 0x3F : i * 2;
			lookup_table[96 + i] = lookup_table[i];
		}
		for (uint32_t i = 0; i < 64; i++)
			lookup_table[32 + i] = i;

		uint8_t value = L5F_COLMOD_RGB565;
		if (!l5f_write_command_data(L5F_CMD_RGBSET, lookup_table, sizeof(lookup_table)) ||
			!l5f_write_command_data(L5F_CMD_COLMOD, &value, 1))
			return false;
	} else if (format == LCD_PIXEL_FORMAT_RGB666) {
		uint8_t value = L5F_COLMOD_RGB666;

		if (!l5f_write_command_data(L5F_CMD_COLMOD, &value, 1))
			return false;
	} else {
		return false;
	}

	current_format = format;
	return true;
}

static bool l5f_set_address_mode(const struct lcd_address_mode *mode) {
	uint8_t value = (mode->reverse_y ? L5F_MADCTL_REVERSE_Y : 0) |
		(mode->reverse_x ? L5F_MADCTL_REVERSE_X : 0) |
		(mode->swap_axes ? L5F_MADCTL_SWAP_AXES : 0) |
		(mode->bgr ? L5F_MADCTL_BGR : 0);

	return l5f_write_command_data(L5F_CMD_MADCTL, &value, 1);
}

static bool l5f_set_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	uint8_t columns[] = { x_start >> 8, x_start, x_end >> 8, x_end };
	uint8_t pages[] = { y_start >> 8, y_start, y_end >> 8, y_end };

	return l5f_write_command_data(L5F_CMD_CASET, columns, sizeof(columns)) &&
		l5f_write_command_data(L5F_CMD_PASET, pages, sizeof(pages));
}

static bool l5f_write_pixels(const struct lcd_color *colors, uint32_t count) {
	if (!lcd_transport_write_command(L5F_CMD_RAMWR))
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (current_format == LCD_PIXEL_FORMAT_RGB565) {
			uint16_t pixel = ((uint16_t)(colors[i].red >> 1) << 11) |
				((uint16_t)colors[i].green << 5) | (colors[i].blue >> 1);
			uint8_t data[] = { pixel >> 8, pixel };

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

static uint8_t l5f_expand_5bit(uint8_t value) {
	return value == 31 ? 63 : value << 1;
}

static void l5f_quantize_color(
	enum lcd_pixel_format format,
	const struct lcd_color *input,
	struct lcd_color *output
) {
	*output = *input;
	if (format == LCD_PIXEL_FORMAT_RGB565) {
		output->red = l5f_expand_5bit(input->red >> 1);
		output->blue = l5f_expand_5bit(input->blue >> 1);
	}
}

static bool l5f_read_pixels(struct lcd_color *colors, uint32_t count) {
	if (!lcd_transport_write_command(L5F_CMD_RAMRD))
		return false;

	uint8_t dummy;
	if (!lcd_transport_read_data(&dummy, 1))
		return false;

	for (uint32_t i = 0; i < count; i++) {
		if (current_format == LCD_PIXEL_FORMAT_RGB565) {
			uint8_t data[2];

			if (!lcd_transport_read_data(data, sizeof(data)))
				return false;
			uint16_t pixel = ((uint16_t)data[0] << 8) | data[1];
			colors[i] = (struct lcd_color) {
				.red = l5f_expand_5bit(pixel >> 11),
				.green = (pixel >> 5) & 0x3F,
				.blue = l5f_expand_5bit(pixel & 0x1F),
			};
		} else {
			uint8_t data[3];

			if (!lcd_transport_read_data(data, sizeof(data)))
				return false;
			colors[i] = (struct lcd_color) {
				.red = data[0] >> 2,
				.green = data[1] >> 2,
				.blue = data[2] >> 2,
			};
		}
	}

	return true;
}

static bool l5f_initialize(void) {
	static const struct lcd_address_mode DEFAULT_MODE = { 0 };

	if (!lcd_transport_write_command(L5F_CMD_SLPOUT))
		return false;
	stopwatch_usleep_wd(25000);

	return l5f_set_pixel_format(LCD_PIXEL_FORMAT_RGB565) &&
		l5f_set_address_mode(&DEFAULT_MODE) && lcd_transport_write_command(L5F_CMD_DISPON);
}

static bool l5f_probe(uint32_t *id) {
	static const struct lcd_address_mode DEFAULT_MODE = { 0 };
	static const struct lcd_color EXPECTED = { .red = 0x2A, .green = 0x15, .blue = 0x3F };
	struct lcd_color actual = { 0 };

	if (!lcd_transport_write_command(L5F_CMD_SLPOUT))
		return false;
	stopwatch_usleep_wd(25000);
	if (!l5f_set_pixel_format(LCD_PIXEL_FORMAT_RGB666) || !l5f_set_address_mode(&DEFAULT_MODE) ||
		!l5f_set_window(0, 0, 0, 0) || !l5f_write_pixels(&EXPECTED, 1) ||
		!l5f_set_window(0, 0, 0, 0) || !l5f_read_pixels(&actual, 1))
		return false;
	if (actual.red != EXPECTED.red || actual.green != EXPECTED.green || actual.blue != EXPECTED.blue)
		return false;

	*id = 3;
	return true;
}

const struct lcd_controller lcd_controller_l5f30539p00 = {
	.name = "L5F30539P00",
	.type = LCD_CONTROLLER_L5F30539P00,
	.id = 3,
	.width = 240,
	.height = 320,
	.pixel_formats = BIT(LCD_PIXEL_FORMAT_RGB565) | BIT(LCD_PIXEL_FORMAT_RGB666),
	.reset_settle_ms = 0,
	.gram_write_command = { L5F_CMD_RAMWR },
	.gram_write_command_size = 1,
	.gram_read_command = { L5F_CMD_RAMRD },
	.gram_read_command_size = 1,
	.rgb565_read_dummy_bytes = 1,
	// The connected OEM module accepts MADCTL.B5 but keeps the same observable GRAM traversal order.
	.swap_axes_changes_gram_order = false,
	.reverse_x_mirrors_coordinates = true,
	.reverse_y_mirrors_coordinates = true,
	.probe = l5f_probe,
	.initialize = l5f_initialize,
	.quantize_color = l5f_quantize_color,
	.set_pixel_format = l5f_set_pixel_format,
	.set_address_mode = l5f_set_address_mode,
	.set_window = l5f_set_window,
	.write_pixels = l5f_write_pixels,
	.read_pixels = l5f_read_pixels,
};
