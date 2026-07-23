#include <pmb887x.h>

#include "lcd-controller.h"
#include "lcd-transport.h"

#define SSD1286_REG_OSCILLATOR 0x00U
#define SSD1286_REG_DRIVER_OUTPUT 0x01U
#define SSD1286_REG_DRIVE_AC 0x02U
#define SSD1286_REG_ENTRY_MODE 0x03U
#define SSD1286_REG_DISPLAY_CONTROL 0x07U
#define SSD1286_REG_POWER_1 0x10U
#define SSD1286_REG_POWER_2 0x11U
#define SSD1286_REG_POWER_3 0x12U
#define SSD1286_REG_RAM_ADDRESS 0x21U
#define SSD1286_REG_RAM_DATA 0x22U
#define SSD1286_REG_VCOM_OTP_1 0x28U
#define SSD1286_REG_TEST_2C 0x2CU
#define SSD1286_REG_TEST_2D 0x2DU
#define SSD1286_REG_HORIZONTAL_WINDOW 0x44U
#define SSD1286_REG_VERTICAL_WINDOW 0x45U

#define SSD1286_DRIVER_BGR BIT(11)
#define SSD1286_DRIVER_TB BIT(8)
#define SSD1286_DRIVER_RL BIT(7)
#define SSD1286_DRIVER_OUTPUT_DEFAULT 0x31AFU

#define SSD1286_ENTRY_MODE_BASE 0x6800U
#define SSD1286_ENTRY_VERTICAL_INCREMENT BIT(5)
#define SSD1286_ENTRY_HORIZONTAL_INCREMENT BIT(4)
#define SSD1286_ENTRY_VERTICAL_FIRST BIT(3)
#define SSD1286_ENTRY_MODE_DEFAULT 0x6830U

#define SSD1286_WIDTH 132U
#define SSD1286_HEIGHT 176U

static uint16_t driver_output = SSD1286_DRIVER_OUTPUT_DEFAULT;
static uint16_t entry_mode = SSD1286_ENTRY_MODE_DEFAULT;

static bool ssd1286_write_register(uint8_t reg, uint16_t value) {
	uint8_t data[] = { value >> 8, value };

	return lcd_transport_write_command(reg) && lcd_transport_write_data(data, sizeof(data));
}

static bool ssd1286_set_pixel_format(enum lcd_pixel_format format) {
	if (format != LCD_PIXEL_FORMAT_RGB565)
		return false;

	entry_mode |= BIT(14) | BIT(13);
	return ssd1286_write_register(SSD1286_REG_ENTRY_MODE, entry_mode);
}

static bool ssd1286_set_address_mode(const struct lcd_address_mode *mode) {
	entry_mode = SSD1286_ENTRY_MODE_BASE |
		(mode->reverse_y ? 0 : SSD1286_ENTRY_VERTICAL_INCREMENT) |
		(mode->reverse_x ? 0 : SSD1286_ENTRY_HORIZONTAL_INCREMENT) |
		(mode->swap_axes ? SSD1286_ENTRY_VERTICAL_FIRST : 0);
	driver_output = SSD1286_DRIVER_OUTPUT_DEFAULT;
	if (mode->reverse_x)
		driver_output ^= SSD1286_DRIVER_RL;
	if (mode->reverse_y)
		driver_output ^= SSD1286_DRIVER_TB;
	if (mode->bgr)
		driver_output |= SSD1286_DRIVER_BGR;

	return ssd1286_write_register(SSD1286_REG_DRIVER_OUTPUT, driver_output) &&
		ssd1286_write_register(SSD1286_REG_ENTRY_MODE, entry_mode);
}

static bool ssd1286_set_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	if (x_start > x_end || y_start > y_end || x_end >= SSD1286_WIDTH || y_end >= SSD1286_HEIGHT)
		return false;

	return ssd1286_write_register(SSD1286_REG_HORIZONTAL_WINDOW, (x_end << 8) | x_start) &&
		ssd1286_write_register(SSD1286_REG_VERTICAL_WINDOW, (y_end << 8) | y_start) &&
		ssd1286_write_register(SSD1286_REG_RAM_ADDRESS, (y_start << 8) | x_start);
}

static bool ssd1286_set_cursor(uint16_t x, uint16_t y) {
	if (x >= SSD1286_WIDTH || y >= SSD1286_HEIGHT)
		return false;

	return ssd1286_write_register(SSD1286_REG_RAM_ADDRESS, (y << 8) | x);
}

static bool ssd1286_write_pixels(const struct lcd_color *colors, uint32_t count) {
	if (!lcd_transport_write_command(SSD1286_REG_RAM_DATA) || !lcd_transport_begin_data_stream())
		return false;

	bool success = true;
	for (uint32_t i = 0; i < count && success; i++) {
		uint16_t pixel = ((uint16_t)(colors[i].red >> 1) << 11) |
			((uint16_t)colors[i].green << 5) | (colors[i].blue >> 1);
		success = lcd_transport_write_data_word(pixel);
	}

	return lcd_transport_end_data_stream() && success;
}

static void ssd1286_quantize_color(
	enum lcd_pixel_format format,
	const struct lcd_color *input,
	struct lcd_color *output
) {
	(void) format;
	*output = *input;
	output->red = (input->red & 0x3E) | (input->red >> 5);
	output->blue = (input->blue & 0x3E) | (input->blue >> 5);
}

static bool ssd1286_probe(uint32_t *id) {
	uint8_t raw[4] = { 0 };

	if (!lcd_transport_write_command(SSD1286_REG_OSCILLATOR) ||
		!lcd_transport_read_data(raw, sizeof(raw)))
		return false;
	printf(
		"# SSD1286 R00h device code raw: %02X %02X %02X %02X\n",
		raw[0],
		raw[1],
		raw[2],
		raw[3]
	);
	for (uint32_t i = 0; i + 1 < ARRAY_SIZE(raw); i++) {
		if (raw[i] == 0x12 && raw[i + 1] == 0x86) {
			*id = 0x1286;
			return true;
		}
	}
	*id = ((uint32_t)raw[0] << 8) | raw[1];

	return false;
}

static bool ssd1286_initialize(void) {
	static const uint8_t WHITE_18BIT[] = { 0xFF, 0xFF, 0xFF };
	static const struct lcd_address_mode DEFAULT_MODE = { 0 };
	static const struct {
		uint8_t reg;
		uint16_t value;
	} INITIAL_REGISTERS[] = {
		{ SSD1286_REG_OSCILLATOR, 0x0001 },
		{ SSD1286_REG_POWER_1, 0x1F92 },
		{ SSD1286_REG_POWER_2, 0x0014 },
		{ SSD1286_REG_OSCILLATOR, 0x0001 },
		{ SSD1286_REG_POWER_1, 0x1F92 },
		{ SSD1286_REG_POWER_2, 0x0014 },
		{ SSD1286_REG_VCOM_OTP_1, 0x0006 },
		{ SSD1286_REG_DRIVE_AC, 0x0000 },
		{ SSD1286_REG_POWER_3, 0x040B },
	};

	bool success = true;
	for (uint32_t i = 0; i < ARRAY_SIZE(INITIAL_REGISTERS) && success; i++)
		success = ssd1286_write_register(INITIAL_REGISTERS[i].reg, INITIAL_REGISTERS[i].value);

	/* Preserve the firmware's reset-time 18-bit RAMWR phase without a cosmetic full-screen clear. */
	success &= lcd_transport_write_command(SSD1286_REG_RAM_DATA) &&
		lcd_transport_write_data(WHITE_18BIT, sizeof(WHITE_18BIT));

	entry_mode = SSD1286_ENTRY_MODE_DEFAULT;
	driver_output = SSD1286_DRIVER_OUTPUT_DEFAULT;
	return success && ssd1286_write_register(SSD1286_REG_ENTRY_MODE, entry_mode) &&
		ssd1286_write_register(SSD1286_REG_DRIVER_OUTPUT, driver_output) &&
		ssd1286_write_register(SSD1286_REG_DISPLAY_CONTROL, 0x0033) &&
		ssd1286_write_register(SSD1286_REG_HORIZONTAL_WINDOW, 0x8300) &&
		ssd1286_write_register(SSD1286_REG_VERTICAL_WINDOW, 0xAF00) &&
		ssd1286_write_register(SSD1286_REG_TEST_2C, 0x3000) &&
		ssd1286_write_register(SSD1286_REG_TEST_2D, 0x310F) &&
		ssd1286_set_pixel_format(LCD_PIXEL_FORMAT_RGB565) &&
		ssd1286_set_address_mode(&DEFAULT_MODE) &&
		ssd1286_set_window(0, SSD1286_WIDTH - 1, 0, SSD1286_HEIGHT - 1);
}

const struct lcd_controller lcd_controller_ssd1286 = {
	.name = "SSD1286",
	.type = LCD_CONTROLLER_SSD1286,
	.id = 0x1286,
	.width = SSD1286_WIDTH,
	.height = SSD1286_HEIGHT,
	.pixel_formats = BIT(LCD_PIXEL_FORMAT_RGB565),
	.reset_settle_ms = 0,
	.gram_write_command = { SSD1286_REG_RAM_DATA },
	.gram_write_command_size = 1,
	.swap_axes_changes_gram_order = true,
	.reverse_x_mirrors_coordinates = true,
	.reverse_y_mirrors_coordinates = true,
	.probe = ssd1286_probe,
	.initialize = ssd1286_initialize,
	.quantize_color = ssd1286_quantize_color,
	.set_pixel_format = ssd1286_set_pixel_format,
	.set_address_mode = ssd1286_set_address_mode,
	.set_window = ssd1286_set_window,
	.set_cursor = ssd1286_set_cursor,
	.write_pixels = ssd1286_write_pixels,
};
