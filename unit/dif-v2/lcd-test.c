#include <pmb887x.h>
#include <string.h>

#include "lcd/lcd-controller.h"
#include "lcd/lcd-transport.h"
#include "lcd-board.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_E71) && !defined(BOARD_SIEMENS_EL71)
#error The DIFv2 LCD test currently requires BOARD=siemens-e71 or BOARD=siemens-el71
#endif

static const struct lcd_color TEST_COLORS[] = {
	{ .red = 0x3F, .green = 0x00, .blue = 0x00 },
	{ .red = 0x00, .green = 0x3F, .blue = 0x00 },
	{ .red = 0x00, .green = 0x00, .blue = 0x3F },
	{ .red = 0x2A, .green = 0x15, .blue = 0x0A },
};

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, DIF_CLC);
	DIF_CLC = 1U << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0xF043C000, DIF_ID);
	test_eq_u32("RUNCTRL reset value", 0, DIF_RUNCTRL);
	test_eq_u32("CON reset value", 0, DIF_CON);
	test_eq_u32("PERREG reset value", 0, DIF_PERREG);
	test_eq_u32("CSREG reset value", 0, DIF_CSREG);
	test_eq_u32("LCDTIM1 reset value", 0, DIF_LCDTIM1);
	test_eq_u32("LCDTIM2 reset value", 0, DIF_LCDTIM2);
	test_eq_u32("STARTLCDRD reset value", 0, DIF_STARTLCDRD);
	test_eq_u32("STAT reset value", 0, DIF_STAT);
	test_eq_u32("COEFF_REG1 reset value", 0, DIF_COEFF_REG1);
	test_eq_u32("COEFF_REG2 reset value", 0, DIF_COEFF_REG2);
	test_eq_u32("COEFF_REG3 reset value", 0, DIF_COEFF_REG3);
	test_eq_u32("OFFSET reset value", 0, DIF_OFFSET);
	test_eq_u32("PBCCON reset value", 0, DIF_PBCCON);
	test_eq_u32("BMREG0 identity mapping", 0x14830820, DIF_BMREG0);
	test_eq_u32("BMREG1 identity mapping", 0x2D4920E6, DIF_BMREG1);
	test_eq_u32("BMREG2 identity mapping", 0x460F39AC, DIF_BMREG2);
	test_eq_u32("BMREG3 identity mapping", 0x5ED55272, DIF_BMREG3);
	test_eq_u32("BMREG4 identity mapping", 0x779B6B38, DIF_BMREG4);
	test_eq_u32("BMREG5 identity mapping", 0x000003FE, DIF_BMREG5);
	test_eq_u32("BCSEL0 reset value", 0, DIF_BCSEL0);
	test_eq_u32("BCSEL1 reset value", 0, DIF_BCSEL1);
	test_eq_u32("BCREG reset value", 0, DIF_BCREG);
	test_eq_u32("INVERT_BIT reset value", 0, DIF_INVERT_BIT);
	test_eq_u32("SYNC_CONFIG reset value", 0, DIF_SYNC_CONFIG);
	test_eq_u32("SYNC_COUNT reset value", 0, DIF_SYNC_COUNT);
	test_eq_u32("BR reset value", 0, DIF_BR);
	test_eq_u32("FDIV reset value", 0, DIF_FDIV);
	test_eq_u32("RXFIFO_CFG reset value", DIF_RXFIFO_CFG_RXBS_4_WORD, DIF_RXFIFO_CFG);
	test_eq_u32("RPS_STAT reset value", 0, DIF_RPS_STAT);
	test_eq_u32("RXFFS_STAT reset value", 0, DIF_RXFFS_STAT);
	test_eq_u32("TXFIFO_CFG reset value", DIF_TXFIFO_CFG_TXBS_8_WORD, DIF_TXFIFO_CFG);
	test_eq_u32("TPS_CTRL reset value", 0, DIF_TPS_CTRL);
	test_eq_u32("TXFFS_STAT reset value", 0, DIF_TXFFS_STAT);
	// ERRIRQSM may be retained while CLC is disabled and configured by a previous payload.
	test_eq_u32("ERRIRQSS reset value", 0, DIF_ERRIRQSS);
	test_eq_u32("RIS reset value", 0, DIF_RIS);
	test_eq_u32("IMSC reset value", 0, DIF_IMSC);
	test_eq_u32("MIS reset value", 0, DIF_MIS);
	test_eq_u32("DMAE reset value", 0, DIF_DMAE);
}

static bool colors_equal(const struct lcd_color *expected, const struct lcd_color *actual, uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		if (expected[i].red != actual[i].red || expected[i].green != actual[i].green ||
			 expected[i].blue != actual[i].blue)
			return false;
	}
	return true;
}

static bool read_pixel(const struct lcd_controller *lcd, uint16_t x, uint16_t y, struct lcd_color *color) {
	return lcd->set_window(x, x, y, y) && lcd->read_pixels(color, 1);
}

static void test_pixel_formats(const struct lcd_controller *lcd) {
	test_category("GRAM pixel formats");
	static const struct pixel_format_profile {
		enum lcd_pixel_format format;
		const char *name;
	} FORMATS[] = {
		{ LCD_PIXEL_FORMAT_RGB565, "RGB565 / 2 transfers" },
		{ LCD_PIXEL_FORMAT_RGB666_8_8_2, "RGB666 / 8+8+2" },
		{ LCD_PIXEL_FORMAT_RGB666_2_8_8, "RGB666 / 2+8+8" },
		{ LCD_PIXEL_FORMAT_RGB666, "RGB666 / 6+6+6" },
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(FORMATS); i++) {
		if ((lcd->pixel_formats & BIT(FORMATS[i].format)) == 0)
			continue;
		struct lcd_color expected[ARRAY_SIZE(TEST_COLORS)];
		struct lcd_color actual[ARRAY_SIZE(TEST_COLORS)] = { 0 };
		for (uint32_t color = 0; color < ARRAY_SIZE(TEST_COLORS); color++)
			lcd->quantize_color(FORMATS[i].format, &TEST_COLORS[color], &expected[color]);
		printf("# pixel format: %s\n", FORMATS[i].name);
		bool success = lcd->set_pixel_format(FORMATS[i].format) &&
			lcd->set_window(20, 23, 20, 20) && lcd->write_pixels(TEST_COLORS, ARRAY_SIZE(TEST_COLORS)) &&
			lcd->set_window(20, 23, 20, 20) && lcd->read_pixels(actual, ARRAY_SIZE(actual));

		test_check("pixel format GRAM round-trip completes", success);
		test_check("pixel format GRAM values match",
			success && colors_equal(expected, actual, ARRAY_SIZE(actual)));
	}
}

static void test_address_modes(const struct lcd_controller *lcd) {
	test_category("GRAM address mode matrix");
	static const struct lcd_address_mode NORMAL_MODE = { 0 };

	test_check("RGB666 selected for address tests", lcd->set_pixel_format(LCD_PIXEL_FORMAT_RGB666));
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

			expected[physical_offset_y * 2 + physical_offset_x] = TEST_COLORS[i];
		}
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++)
				success &= read_pixel(lcd, physical_x + x, physical_y + y, &actual[y * 2 + x]);
		}

		test_check("address mode write/read completes", success);
		bool matches = success && colors_equal(expected, actual, ARRAY_SIZE(actual));
		test_check("address mode maps pixels correctly", matches);
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

	bool success = lcd->set_pixel_format(LCD_PIXEL_FORMAT_RGB666) && lcd->set_address_mode(&NORMAL_MODE) &&
		lcd->set_window(0xE9, 0xEB, 70, 70) && lcd->write_pixels(COLORS, ARRAY_SIZE(COLORS));
	for (uint32_t x = 0; x < ARRAY_SIZE(actual); x++)
		success &= read_pixel(lcd, 0xE9 + x, 70, &actual[x]);
	test_check("horizontal window wrap completes", success);
	test_check("X1/X2 wrap to the first column",
		success && colors_equal(WRAPPED, actual, ARRAY_SIZE(actual)));

	memset(actual, 0, sizeof(actual));
	success = lcd->set_window(75, 75, 0x100, 0x102) && lcd->write_pixels(COLORS, ARRAY_SIZE(COLORS));
	for (uint32_t y = 0; y < ARRAY_SIZE(actual); y++)
		success &= read_pixel(lcd, 75, 0x100 + y, &actual[y]);
	test_check("vertical window wrap completes", success);
	test_check("Y1/Y2 wrap to the first row",
		success && colors_equal(WRAPPED, actual, ARRAY_SIZE(actual)));

	if (lcd->set_cursor == NULL)
		return;
	struct lcd_color baseline[25];
	for (uint32_t i = 0; i < ARRAY_SIZE(baseline); i++)
		baseline[i] = TEST_COLORS[3];
	success = lcd->set_window(0xD9, 0xDD, 80, 84) && lcd->write_pixels(baseline, ARRAY_SIZE(baseline)) &&
		lcd->set_cursor(0xDB, 80) && lcd->write_pixels(&TEST_COLORS[0], 1) &&
		read_pixel(lcd, 0xDB, 80, &actual[0]) && read_pixel(lcd, 0xDA, 80, &actual[1]);
	test_check("current X write/read completes", success);
	test_check("current X selects only the requested column",
		success && colors_equal(&TEST_COLORS[0], &actual[0], 1) &&
		colors_equal(&TEST_COLORS[3], &actual[1], 1));

	for (uint32_t i = 0; i < 5; i++)
		baseline[i] = TEST_COLORS[3];
	success = lcd->set_window(80, 80, 0x109, 0x10D) && lcd->write_pixels(baseline, 5) &&
		lcd->set_cursor(80, 0x10B) && lcd->write_pixels(&TEST_COLORS[1], 1) &&
		read_pixel(lcd, 80, 0x10B, &actual[0]) && read_pixel(lcd, 80, 0x10A, &actual[1]);
	test_check("current Y write/read completes", success);
	test_check("current Y selects only the requested row",
		success && colors_equal(&TEST_COLORS[1], &actual[0], 1) &&
		colors_equal(&TEST_COLORS[3], &actual[1], 1));
}

static void test_bgr_mode(const struct lcd_controller *lcd) {
	test_category("RGB/BGR mode");
	static const struct lcd_address_mode RGB_MODE = { 0 };
	static const struct lcd_address_mode BGR_MODE = { .bgr = true };
	struct lcd_color actual = { 0 };

	bool success = lcd->set_address_mode(&BGR_MODE) && lcd->set_window(60, 60, 60, 60) &&
		lcd->write_pixels(&TEST_COLORS[3], 1) && lcd->set_address_mode(&RGB_MODE) &&
		lcd->set_window(60, 60, 60, 60) && lcd->read_pixels(&actual, 1);
	test_check("BGR GRAM round-trip completes", success);
	test_check("BGR affects the display latch, not stored GRAM data",
		success && colors_equal(&TEST_COLORS[3], &actual, 1));
}

static void draw_visible_pattern(const struct lcd_controller *lcd) {
	test_category("Visible test pattern");
	static const struct lcd_address_mode NORMAL_MODE = { 0 };
	static const struct lcd_color COLORS[] = {
		{ .red = 0x3F, .green = 0x00, .blue = 0x00 },
		{ .red = 0x00, .green = 0x3F, .blue = 0x00 },
		{ .red = 0x00, .green = 0x00, .blue = 0x3F },
		{ .red = 0x3F, .green = 0x3F, .blue = 0x3F },
		{ .red = 0x3F, .green = 0x3F, .blue = 0x00 },
		{ .red = 0x00, .green = 0x3F, .blue = 0x3F },
		{ .red = 0x3F, .green = 0x00, .blue = 0x3F },
		{ .red = 0x00, .green = 0x00, .blue = 0x00 },
	};
	struct lcd_color row[240];

	test_check("RGB565 and normal addressing selected",
		lcd->set_pixel_format(LCD_PIXEL_FORMAT_RGB565) && lcd->set_address_mode(&NORMAL_MODE));
	for (uint32_t band = 0; band < ARRAY_SIZE(COLORS); band++) {
		for (uint32_t x = 0; x < lcd->width; x++)
			row[x] = COLORS[band];
		bool success = true;
		for (uint32_t y = 0; y < 40 && success; y++) {
			uint16_t row_y = band * 40 + y;

			success &= lcd->set_window(0, lcd->width - 1, row_y, row_y) &&
				lcd->write_pixels(row, lcd->width);
		}
		test_check("visible color band written", success);
	}
}

int main(void) {
	test_start("DIFv2 LCD controller");
	test_reset_values();

	lcd_transport_init();
	lcd_board_enable_panel_power();

	test_category("Controller detection and initialization");
	uint32_t detected_id = UINT32_MAX;
	const struct lcd_controller *lcd = lcd_controller_detect(&detected_id);
	test_check("supported LCD controller detected", lcd != NULL);
	if (lcd == NULL)
		return test_finish();
	test_eq_u32("controller ID matches selected backend", lcd->id, detected_id);
	lcd_controller_reset(lcd);
	test_check("controller initializes", lcd->initialize());
	lcd_board_enable_backlight();

	test_pixel_formats(lcd);
	test_address_modes(lcd);
	test_window_and_cursor(lcd);
	test_bgr_mode(lcd);
	draw_visible_pattern(lcd);

	return test_finish();
}
