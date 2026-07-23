#include <pmb887x.h>
#include <string.h>

#include "lcd-controller-test.h"
#include "test.h"

#define LCD_TEST_MAX_WIDTH 240U

static const struct lcd_color TEST_COLORS[] = {
	{ .red = 0x3F, .green = 0x00, .blue = 0x00 },
	{ .red = 0x00, .green = 0x3F, .blue = 0x00 },
	{ .red = 0x00, .green = 0x00, .blue = 0x3F },
	{ .red = 0x2A, .green = 0x15, .blue = 0x0A },
};

static const struct lcd_color PATTERN_COLORS[] = {
	{ .red = 0x3F, .green = 0x00, .blue = 0x00 },
	{ .red = 0x00, .green = 0x3F, .blue = 0x00 },
	{ .red = 0x00, .green = 0x00, .blue = 0x3F },
	{ .red = 0x3F, .green = 0x3F, .blue = 0x3F },
	{ .red = 0x3F, .green = 0x3F, .blue = 0x00 },
	{ .red = 0x00, .green = 0x3F, .blue = 0x3F },
	{ .red = 0x3F, .green = 0x00, .blue = 0x3F },
	{ .red = 0x00, .green = 0x00, .blue = 0x00 },
};

static const struct pixel_format_profile {
	enum lcd_pixel_format format;
	const char *name;
} PIXEL_FORMATS[] = {
	{ LCD_PIXEL_FORMAT_RGB565, "RGB565 / 2 transfers" },
	{ LCD_PIXEL_FORMAT_RGB666_8_8_2, "RGB666 / 8+8+2" },
	{ LCD_PIXEL_FORMAT_RGB666_2_8_8, "RGB666 / 2+8+8" },
	{ LCD_PIXEL_FORMAT_RGB666, "RGB666 / 6+6+6" },
};

static bool colors_equal(const struct lcd_color *expected, const struct lcd_color *actual, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		if (expected[i].red != actual[i].red || expected[i].green != actual[i].green ||
			 expected[i].blue != actual[i].blue)
			return false;
	}
	return true;
}

static enum lcd_pixel_format select_address_test_format(const struct lcd_controller *lcd) {
	if ((lcd->pixel_formats & BIT(LCD_PIXEL_FORMAT_RGB666)) != 0)
		return LCD_PIXEL_FORMAT_RGB666;
	return LCD_PIXEL_FORMAT_RGB565;
}

static bool read_pixel(const struct lcd_controller *lcd, uint16_t x, uint16_t y, struct lcd_color *color) {
	return lcd->set_window(x, x, y, y) && lcd->read_pixels(color, 1);
}

static void test_pixel_formats(const struct lcd_controller *lcd) {
	test_category("GRAM pixel formats");

	for (uint32_t i = 0; i < ARRAY_SIZE(PIXEL_FORMATS); i++) {
		if ((lcd->pixel_formats & BIT(PIXEL_FORMATS[i].format)) == 0)
			continue;
		struct lcd_color expected[ARRAY_SIZE(TEST_COLORS)];
		struct lcd_color actual[ARRAY_SIZE(TEST_COLORS)] = { 0 };
		for (uint32_t color = 0; color < ARRAY_SIZE(TEST_COLORS); color++)
			lcd->quantize_color(PIXEL_FORMATS[i].format, &TEST_COLORS[color], &expected[color]);
		printf("# pixel format: %s\n", PIXEL_FORMATS[i].name);
		bool success = lcd->set_pixel_format(PIXEL_FORMATS[i].format) &&
			lcd->set_window(20, 23, 20, 20) && lcd->write_pixels(TEST_COLORS, ARRAY_SIZE(TEST_COLORS));

		if (lcd->read_pixels == NULL) {
			test_check("pixel format GRAM write completes", success);
			continue;
		}
		success &= lcd->set_window(20, 23, 20, 20) && lcd->read_pixels(actual, ARRAY_SIZE(actual));
		test_check("pixel format GRAM round-trip completes", success);
		test_check("pixel format GRAM values match",
			success && colors_equal(expected, actual, ARRAY_SIZE(actual)));
	}
}

static void test_address_modes_write_only(const struct lcd_controller *lcd, enum lcd_pixel_format format) {
	static const struct lcd_address_mode NORMAL_MODE = { 0 };

	for (uint32_t bits = 0; bits < 8; bits++) {
		struct lcd_address_mode mode = {
			.swap_axes = (bits & BIT(0)) != 0,
			.reverse_x = (bits & BIT(1)) != 0,
			.reverse_y = (bits & BIT(2)) != 0,
		};
		bool success = lcd->set_address_mode(&mode) && lcd->set_window(40, 41, 50, 51) &&
			lcd->write_pixels(TEST_COLORS, ARRAY_SIZE(TEST_COLORS));

		printf("# address mode: swap=%u reverse_x=%u reverse_y=%u\n",
			mode.swap_axes, mode.reverse_x, mode.reverse_y);
		test_check("address mode GRAM write completes", success);
	}
	test_check("normal address mode restored", lcd->set_address_mode(&NORMAL_MODE) && lcd->set_pixel_format(format));
}

static void test_address_modes(const struct lcd_controller *lcd) {
	test_category("GRAM address mode matrix");
	static const struct lcd_address_mode NORMAL_MODE = { 0 };
	enum lcd_pixel_format format = select_address_test_format(lcd);

	test_check("address-test pixel format selected", lcd->set_pixel_format(format));
	if (lcd->read_pixels == NULL) {
		test_address_modes_write_only(lcd, format);
		return;
	}

	for (uint32_t bits = 0; bits < 8; bits++) {
		struct lcd_address_mode mode = {
			.swap_axes = (bits & BIT(0)) != 0,
			.reverse_x = (bits & BIT(1)) != 0,
			.reverse_y = (bits & BIT(2)) != 0,
		};
		struct lcd_color expected[4] = { 0 };
		struct lcd_color actual[4] = { 0 };
		bool mirror_x = mode.reverse_x && lcd->reverse_x_mirrors_coordinates;
		bool mirror_y = mode.reverse_y && lcd->reverse_y_mirrors_coordinates;
		uint16_t physical_x = mirror_x ? lcd->width - 42 : 40;
		uint16_t physical_y = mirror_y ? lcd->height - 52 : 50;
		bool effective_swap = mode.swap_axes && lcd->swap_axes_changes_gram_order;
		bool success = lcd->set_address_mode(&mode) && lcd->set_window(40, 41, 50, 51) &&
			lcd->write_pixels(TEST_COLORS, ARRAY_SIZE(TEST_COLORS)) && lcd->set_address_mode(&NORMAL_MODE);

		for (uint32_t i = 0; i < ARRAY_SIZE(TEST_COLORS); i++) {
			uint32_t address_x = effective_swap ? i / 2 : i % 2;
			uint32_t address_y = effective_swap ? i % 2 : i / 2;
			uint32_t physical_offset_x = mode.reverse_x ? 1 - address_x : address_x;
			uint32_t physical_offset_y = mode.reverse_y ? 1 - address_y : address_y;

			lcd->quantize_color(format, &TEST_COLORS[i], &expected[physical_offset_y * 2 + physical_offset_x]);
		}
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++)
				success &= read_pixel(lcd, physical_x + x, physical_y + y, &actual[y * 2 + x]);
		}

		test_check("address mode write/read completes", success);
		test_check("address mode maps pixels correctly", success && colors_equal(expected, actual, ARRAY_SIZE(actual)));
	}
}

static void test_window_and_cursor(const struct lcd_controller *lcd) {
	test_category("GRAM window and cursor");
	static const struct lcd_address_mode NORMAL_MODE = { 0 };
	static const struct lcd_color COLORS[] = {
		{ .red = 0x3F, .green = 0x00, .blue = 0x00 },
		{ .red = 0x00, .green = 0x3F, .blue = 0x00 },
		{ .red = 0x00, .green = 0x00, .blue = 0x3F },
		{ .red = 0x3F, .green = 0x3F, .blue = 0x00 },
		{ .red = 0x00, .green = 0x3F, .blue = 0x3F },
	};
	static const struct lcd_color WRAPPED[] = {
		{ .red = 0x3F, .green = 0x3F, .blue = 0x00 },
		{ .red = 0x00, .green = 0x3F, .blue = 0x3F },
		{ .red = 0x00, .green = 0x00, .blue = 0x3F },
	};
	struct lcd_color actual[ARRAY_SIZE(WRAPPED)] = { 0 };
	uint16_t x_start = lcd->width - ARRAY_SIZE(actual);
	uint16_t y_start = lcd->height - ARRAY_SIZE(actual);
	uint16_t test_x = lcd->width / 3;
	uint16_t test_y = lcd->height / 3;
	enum lcd_pixel_format format = select_address_test_format(lcd);

	bool success = lcd->set_pixel_format(format) && lcd->set_address_mode(&NORMAL_MODE) &&
		lcd->set_window(x_start, lcd->width - 1, test_y, test_y) &&
		lcd->write_pixels(COLORS, ARRAY_SIZE(COLORS));
	if (lcd->read_pixels == NULL) {
		test_check("horizontal window wrap write completes", success);
	} else {
		for (uint32_t x = 0; x < ARRAY_SIZE(actual); x++)
			success &= read_pixel(lcd, x_start + x, test_y, &actual[x]);
		test_check("horizontal window wrap completes", success);
		test_check("X1/X2 wrap to the first column",
			success && colors_equal(WRAPPED, actual, ARRAY_SIZE(actual)));
	}

	memset(actual, 0, sizeof(actual));
	success = lcd->set_window(test_x, test_x, y_start, lcd->height - 1) &&
		lcd->write_pixels(COLORS, ARRAY_SIZE(COLORS));
	if (lcd->read_pixels == NULL) {
		test_check("vertical window wrap write completes", success);
	} else {
		for (uint32_t y = 0; y < ARRAY_SIZE(actual); y++)
			success &= read_pixel(lcd, test_x, y_start + y, &actual[y]);
		test_check("vertical window wrap completes", success);
		test_check("Y1/Y2 wrap to the first row",
			success && colors_equal(WRAPPED, actual, ARRAY_SIZE(actual)));
	}

	if (lcd->set_cursor == NULL || lcd->read_pixels == NULL)
		return;
	struct lcd_color baseline[25];
	for (uint32_t i = 0; i < ARRAY_SIZE(baseline); i++)
		baseline[i] = TEST_COLORS[3];
	success = lcd->set_window(lcd->width - 7, lcd->width - 3, test_y, test_y + 4) &&
		lcd->write_pixels(baseline, ARRAY_SIZE(baseline)) &&
		lcd->set_cursor(lcd->width - 5, test_y) && lcd->write_pixels(&TEST_COLORS[0], 1) &&
		read_pixel(lcd, lcd->width - 5, test_y, &actual[0]) &&
		read_pixel(lcd, lcd->width - 6, test_y, &actual[1]);
	test_check("current X write/read completes", success);
	test_check("current X selects only the requested column",
		success && colors_equal(&TEST_COLORS[0], &actual[0], 1) &&
		colors_equal(&TEST_COLORS[3], &actual[1], 1));

	for (uint32_t i = 0; i < 5; i++)
		baseline[i] = TEST_COLORS[3];
	success = lcd->set_window(test_x, test_x, lcd->height - 7, lcd->height - 3) &&
		lcd->write_pixels(baseline, 5) && lcd->set_cursor(test_x, lcd->height - 5) &&
		lcd->write_pixels(&TEST_COLORS[1], 1) && read_pixel(lcd, test_x, lcd->height - 5, &actual[0]) &&
		read_pixel(lcd, test_x, lcd->height - 6, &actual[1]);
	test_check("current Y write/read completes", success);
	test_check("current Y selects only the requested row",
		success && colors_equal(&TEST_COLORS[1], &actual[0], 1) &&
		colors_equal(&TEST_COLORS[3], &actual[1], 1));
}

static void test_bgr_mode(const struct lcd_controller *lcd) {
	test_category("RGB/BGR mode");
	static const struct lcd_address_mode RGB_MODE = { 0 };
	static const struct lcd_address_mode BGR_MODE = { .bgr = true };
	struct lcd_color expected = { 0 };
	struct lcd_color actual = { 0 };
	enum lcd_pixel_format format = select_address_test_format(lcd);
	uint16_t x = lcd->width / 2;
	uint16_t y = lcd->height / 2;
	lcd->quantize_color(format, &TEST_COLORS[3], &expected);

	bool success = lcd->set_pixel_format(format) && lcd->set_address_mode(&BGR_MODE) &&
		lcd->set_window(x, x, y, y) && lcd->write_pixels(&TEST_COLORS[3], 1) && lcd->set_address_mode(&RGB_MODE);
	if (lcd->read_pixels == NULL) {
		test_check("BGR GRAM write completes", success);
		return;
	}
	success &= lcd->set_window(x, x, y, y) && lcd->read_pixels(&actual, 1);
	test_check("BGR GRAM round-trip completes", success);
	test_check("BGR affects the display latch, not stored GRAM data", success && colors_equal(&expected, &actual, 1));
}

static void draw_visible_pattern(const struct lcd_controller *lcd) {
	test_category("Visible test pattern");
	static const struct lcd_address_mode NORMAL_MODE = { 0 };
	static struct lcd_color row[LCD_TEST_MAX_WIDTH];

	test_check("RGB565 and normal addressing selected",
		lcd->width <= LCD_TEST_MAX_WIDTH && lcd->set_pixel_format(LCD_PIXEL_FORMAT_RGB565) &&
		lcd->set_address_mode(&NORMAL_MODE));
	for (uint32_t band = 0; band < ARRAY_SIZE(PATTERN_COLORS); band++) {
		uint16_t y_start = band * lcd->height / ARRAY_SIZE(PATTERN_COLORS);
		uint16_t y_end = (band + 1) * lcd->height / ARRAY_SIZE(PATTERN_COLORS);

		for (uint32_t x = 0; x < lcd->width; x++)
			row[x] = PATTERN_COLORS[band];
		bool success = true;
		for (uint32_t y = y_start; y < y_end && success; y++) {
			success = lcd->set_window(0, lcd->width - 1, y, y) && lcd->write_pixels(row, lcd->width);
		}
		test_check("visible color band written", success);
	}
}

void lcd_test_controller(const struct lcd_controller *lcd) {
	if (lcd->read_pixels == NULL)
		printf("# GRAM readback is unavailable on this LCD transport; running write-path checks\n");

	test_pixel_formats(lcd);
	test_address_modes(lcd);
	test_window_and_cursor(lcd);
	test_bgr_mode(lcd);
	draw_visible_pattern(lcd);
}
