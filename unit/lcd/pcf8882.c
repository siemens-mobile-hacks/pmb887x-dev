#include <pmb887x.h>

#include "lcd-controller.h"
#include "lcd-transport.h"
#include "test.h"

#define PCF8882_CMD_SLPOUT 0x11U
#define PCF8882_CMD_DISPON 0x29U
#define PCF8882_CMD_RDDIDIF 0x04U
#define PCF8882_CMD_RDID1 0xDAU
#define PCF8882_CMD_RDID2 0xDBU
#define PCF8882_CMD_RDID3 0xDCU
#define PCF8882_CMD_CASET 0x2AU
#define PCF8882_CMD_PASET 0x2BU
#define PCF8882_CMD_RAMWR 0x2CU
#define PCF8882_CMD_MADCTL 0x36U
#define PCF8882_CMD_COLMOD 0x3AU
#define PCF8882_CMD_GAMSET 0x26U
#define PCF8882_CMD_VENDOR_C9 0xC9U
#define PCF8882_CMD_VENDOR_D2 0xD2U
#define PCF8882_CMD_VENDOR_D6 0xD6U

#define PCF8882_MADCTL_REVERSE_Y BIT(7)
#define PCF8882_MADCTL_REVERSE_X BIT(6)
#define PCF8882_MADCTL_SWAP_AXES BIT(5)
#define PCF8882_MADCTL_BGR BIT(3)

#define PCF8882_COLMOD_RGB565 0x05U
#define PCF8882_WIDTH 132U
#define PCF8882_HEIGHT 176U

static bool pcf8882_write_command_data(uint8_t command, const uint8_t *data, uint32_t size) {
	return lcd_transport_write_command(command) && lcd_transport_write_data(data, size);
}

static bool pcf8882_set_pixel_format(enum lcd_pixel_format format) {
	if (format != LCD_PIXEL_FORMAT_RGB565)
		return false;

	uint8_t value = PCF8882_COLMOD_RGB565;
	return pcf8882_write_command_data(PCF8882_CMD_COLMOD, &value, 1);
}

static bool pcf8882_set_address_mode(const struct lcd_address_mode *mode) {
	uint8_t value = (mode->reverse_y ? PCF8882_MADCTL_REVERSE_Y : 0) |
		(mode->reverse_x ? PCF8882_MADCTL_REVERSE_X : 0) |
		(mode->swap_axes ? PCF8882_MADCTL_SWAP_AXES : 0) |
		(mode->bgr ? PCF8882_MADCTL_BGR : 0);

	return pcf8882_write_command_data(PCF8882_CMD_MADCTL, &value, 1);
}

static bool pcf8882_set_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	if (x_end >= PCF8882_WIDTH || y_end >= PCF8882_HEIGHT)
		return false;

	uint8_t columns[] = { x_start, x_end };
	uint8_t pages[] = { y_start, y_end };

	return pcf8882_write_command_data(PCF8882_CMD_CASET, columns, sizeof(columns)) &&
		pcf8882_write_command_data(PCF8882_CMD_PASET, pages, sizeof(pages));
}

static bool pcf8882_write_pixels(const struct lcd_color *colors, uint32_t count) {
	if (!lcd_transport_write_command(PCF8882_CMD_RAMWR) || !lcd_transport_begin_data_stream())
		return false;

	bool success = true;
	for (uint32_t i = 0; i < count && success; i++) {
		uint16_t pixel = ((uint16_t)(colors[i].red >> 1) << 11) |
			((uint16_t)colors[i].green << 5) | (colors[i].blue >> 1);

		success = lcd_transport_write_data_word(pixel);
	}

	return lcd_transport_end_data_stream() && success;
}

static void pcf8882_quantize_color(
	enum lcd_pixel_format format,
	const struct lcd_color *input,
	struct lcd_color *output
) {
	(void) format;
	*output = *input;
	output->red = (input->red & 0x3E) | (input->red >> 5);
	output->blue = (input->blue & 0x3E) | (input->blue >> 5);
}

static bool pcf8882_initialize(void) {
	static const struct lcd_address_mode DEFAULT_MODE = { 0 };
	static const uint8_t GAMMA = 4;
	static const uint8_t ZERO = 0;
	static const uint8_t ONE = 1;
	static const uint8_t WHITE_RGB666[] = { 0xFF, 0xFF, 0xFF };

	/* Exact PCF8882 wake-up order used by the CX75 firmware. */
	bool success = pcf8882_write_command_data(PCF8882_CMD_VENDOR_D2, &ZERO, 1) &&
		pcf8882_write_command_data(PCF8882_CMD_VENDOR_C9, &ZERO, 1) &&
		pcf8882_write_command_data(PCF8882_CMD_VENDOR_D6, &ZERO, 1) &&
		lcd_transport_write_command(PCF8882_CMD_RAMWR);
	for (uint32_t y = 0; y < PCF8882_HEIGHT && success; y++) {
		for (uint32_t x = 0; x < PCF8882_WIDTH && success; x++)
			success = lcd_transport_write_data(WHITE_RGB666, sizeof(WHITE_RGB666));
		test_watchdog_serve();
	}
	success = success && pcf8882_write_command_data(PCF8882_CMD_GAMSET, &GAMMA, 1) &&
		lcd_transport_write_command(PCF8882_CMD_SLPOUT);
	stopwatch_usleep_wd(270000);
	success = success && pcf8882_write_command_data(PCF8882_CMD_VENDOR_D2, &ONE, 1);
	stopwatch_usleep_wd(30000);
	success = success && lcd_transport_write_command(PCF8882_CMD_DISPON);

	return success &&
		pcf8882_set_pixel_format(LCD_PIXEL_FORMAT_RGB565) &&
		pcf8882_set_address_mode(&DEFAULT_MODE) &&
		pcf8882_set_window(0, PCF8882_WIDTH - 1, 0, PCF8882_HEIGHT - 1);
}

static bool pcf8882_probe(uint32_t *id) {
	uint8_t combined[4] = { 0 };
	uint8_t separate[3][2] = { 0 };
	static const uint8_t COMMANDS[] = {
		PCF8882_CMD_RDID1,
		PCF8882_CMD_RDID2,
		PCF8882_CMD_RDID3,
	};

	if (!lcd_transport_write_command(PCF8882_CMD_RDDIDIF) ||
		!lcd_transport_read_data(combined, sizeof(combined)))
		return false;
	for (uint32_t i = 0; i < ARRAY_SIZE(COMMANDS); i++) {
		if (!lcd_transport_write_command(COMMANDS[i]) ||
			!lcd_transport_read_data(separate[i], sizeof(separate[i])))
			return false;
	}
	printf(
		"# PCF8882 ID raw: 04=%02X %02X %02X %02X DA=%02X %02X DB=%02X %02X DC=%02X %02X\n",
		combined[0],
		combined[1],
		combined[2],
		combined[3],
		separate[0][0],
		separate[0][1],
		separate[1][0],
		separate[1][1],
		separate[2][0],
		separate[2][1]
	);
	/* PCF8833-compatible controllers report the Philips manufacturer ID 0x45. */
	if (combined[1] == 0x45 || separate[0][1] == 0x45) {
		*id = 0x8882;
		return true;
	}
	*id = ((uint32_t)combined[1] << 16) | ((uint32_t)combined[2] << 8) | combined[3];
	return false;
}

const struct lcd_controller lcd_controller_pcf8882 = {
	.name = "PCF8882",
	.type = LCD_CONTROLLER_PCF8882,
	.id = 0x8882,
	.width = PCF8882_WIDTH,
	.height = PCF8882_HEIGHT,
	.pixel_formats = BIT(LCD_PIXEL_FORMAT_RGB565),
	.reset_settle_ms = 0,
	.gram_write_command = { PCF8882_CMD_RAMWR },
	.gram_write_command_size = 1,
	.swap_axes_changes_gram_order = true,
	.reverse_x_mirrors_coordinates = true,
	.reverse_y_mirrors_coordinates = true,
	.probe = pcf8882_probe,
	.initialize = pcf8882_initialize,
	.quantize_color = pcf8882_quantize_color,
	.set_pixel_format = pcf8882_set_pixel_format,
	.set_address_mode = pcf8882_set_address_mode,
	.set_window = pcf8882_set_window,
	.write_pixels = pcf8882_write_pixels,
};
