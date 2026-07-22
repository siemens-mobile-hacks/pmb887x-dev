#include <d1601aa.h>
#include <pmb887x.h>
#include <stdbool.h>
#include <stdint.h>

#include "test.h"

#ifndef BOARD_SIEMENS_EL71
#error The DIFv2 LCD test must be built with BOARD=SIEMENS_EL71
#endif

#define DIF_TIMEOUT_MS 100
#define LCD_WIDTH 240U
#define LCD_GRAM_READ_DUMMY_BYTES 4U
#define LCD_GRAM_TEST_BYTES_MAX 4U
#define LCD_DEVICE_CODE_READ_BYTES 8U
/* EL71 uses a fast write profile; JBT6K71 reads require the longer /RD cycle from its AC specification. */
#define LCD_WRITE_TIM1 0x00000400U
#define LCD_WRITE_TIM2 0x02000400U
#define LCD_READ_TIM1 0x00030F00U
#define LCD_READ_TIM2 0x05020601U
#define LCD_ENTRY_MODE_BGR BIT(12)
#define LCD_ENTRY_MODE_TRI BIT(15)
#define LCD_ENTRY_MODE_DFM1 BIT(14)
#define LCD_ENTRY_MODE_DFM0 BIT(13)
#define LCD_ENTRY_MODE_ID1 BIT(5)
#define LCD_ENTRY_MODE_ID0 BIT(4)
#define LCD_ENTRY_MODE_AM BIT(3)

#ifndef LCD_CONTROLLER_TEST
#define LCD_CONTROLLER_TEST 0
#endif

static const struct lcd_register {
	uint16_t index;
	uint16_t value;
} LCD_INITIAL_STATE[] = {
	{ 0x0000, 0x0001 },
	{ 0x0001, 0x0027 },
	{ 0x0002, 0x0200 },
	{ 0x0003, 0x0120 },
	{ 0x0007, 0x4004 },
	{ 0x000D, 0x0011 },
	{ 0x0012, 0x0303 },
	{ 0x0013, 0x0102 },
	{ 0x001C, 0x0000 },
	{ 0x0102, 0x00F6 },
	{ 0x0103, 0x0007 },
	{ 0x0105, 0x0111 },
	{ 0x0300, 0x0200 },
	{ 0x0301, 0x0002 },
	{ 0x0302, 0x0000 },
	{ 0x0303, 0x0300 },
	{ 0x0304, 0x0700 },
	{ 0x0305, 0x0070 },
	{ 0x0402, 0x0000 },
	{ 0x0403, 0x013F },
	{ 0x0406, 0x0000 },
	{ 0x0407, 0x00EF },
	{ 0x0408, 0x0000 },
	{ 0x0409, 0x013F },
	{ 0x0200, 0x00EF },
	{ 0x0201, 0x0000 },
};

static const struct lcd_bsconf_profile {
	const char *name;
	uint32_t value;
	uint16_t color;
} LCD_BSCONF_PROFILES[] = {
	{ "1x8", DIF_CSREG_BSCONF_1x8BIT, 0xF800 },
	{ "2x8", DIF_CSREG_BSCONF_2x8BIT, 0x07E0 },
	{ "3x8", DIF_CSREG_BSCONF_3x8BIT, 0x001F },
	{ "4x8", DIF_CSREG_BSCONF_4x8BIT, 0xFFFF },
	{ "1x9", DIF_CSREG_BSCONF_1x9BIT, 0xFFE0 },
	{ "2x9", DIF_CSREG_BSCONF_2x9BIT, 0x07FF },
	{ "3x9", DIF_CSREG_BSCONF_3x9BIT, 0xF81F },
};

static const struct lcd_bsconf_profile LCD_DIRECT_PROFILES[] = {
	{ "1x8", DIF_CSREG_BSCONF_1x8BIT, 0x1234 },
	{ "1x9", DIF_CSREG_BSCONF_1x9BIT, 0x2345 },
	{ "2x9", DIF_CSREG_BSCONF_2x9BIT, 0x3456 },
	{ "3x9", DIF_CSREG_BSCONF_3x9BIT, 0x4567 },
};

static const struct lcd_packed_bsconf_profile {
	const char *name;
	uint32_t value;
	uint32_t word_bits;
	uint32_t polling_words;
	uint32_t tps_words;
} LCD_PACKED_BSCONF_PROFILES[] = {
	{ "1x8", DIF_CSREG_BSCONF_1x8BIT, 8, 1, 1 },
	{ "2x8", DIF_CSREG_BSCONF_2x8BIT, 8, 2, 2 },
	{ "3x8", DIF_CSREG_BSCONF_3x8BIT, 8, 3, 3 },
	{ "4x8", DIF_CSREG_BSCONF_4x8BIT, 8, 4, 4 },
	{ "1x9", DIF_CSREG_BSCONF_1x9BIT, 9, 1, 1 },
	{ "2x9", DIF_CSREG_BSCONF_2x9BIT, 9, 1, 1 },
	{ "3x9", DIF_CSREG_BSCONF_3x9BIT, 9, 1, 1 },
};

static const struct lcd_pixel_format {
	const char *name;
	uint16_t entry_bits;
	uint32_t bytes_per_pixel;
	uint8_t byte_masks[3];
} LCD_PIXEL_FORMATS[] = {
	{ "8-bit bus / RGB565 in 2 transfers", 0, 2, { 0xFF, 0xFF, 0x00 } },
	{ "8-bit bus / RGB666 8+8+2", LCD_ENTRY_MODE_TRI | LCD_ENTRY_MODE_DFM0, 3, { 0xFF, 0xFF, 0xC0 } },
	{ "8-bit bus / RGB666 2+8+8", LCD_ENTRY_MODE_TRI | LCD_ENTRY_MODE_DFM1, 3, { 0x03, 0xFF, 0xFF } },
	{
		"8-bit bus / RGB666 6+6+6",
		LCD_ENTRY_MODE_TRI | LCD_ENTRY_MODE_DFM1 | LCD_ENTRY_MODE_DFM0,
		3,
		{ 0xFC, 0xFC, 0xFC },
	},
};

static uint32_t lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
static uint32_t lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;

static void reset_data_conversion(void) {
	DIF_PBCCON = 0;
	DIF_BMREG0 = 0x14830820;
	DIF_BMREG1 = 0x2D4920E6;
	DIF_BMREG2 = 0x460F39AC;
	DIF_BMREG3 = 0x5ED55272;
	DIF_BMREG4 = 0x779B6B38;
	DIF_BMREG5 = 0x000003FE;
	DIF_BCSEL0 = 0;
	DIF_BCSEL1 = 0;
	DIF_BCREG = 0;
	DIF_INVERT_BIT = 0;
}

static void set_bmreg_mapping(uint32_t output_bit, uint32_t input_bit) {
	static volatile uint32_t * const REGISTERS[] = {
		&DIF_BMREG0, &DIF_BMREG1, &DIF_BMREG2, &DIF_BMREG3, &DIF_BMREG4, &DIF_BMREG5,
	};
	static const uint32_t SHIFTS[] = {0, 5, 10, 16, 21, 26};
	uint32_t register_index = output_bit / ARRAY_SIZE(SHIFTS);
	uint32_t shift = SHIFTS[output_bit % ARRAY_SIZE(SHIFTS)];
	volatile uint32_t *reg = REGISTERS[register_index];

	*reg = (*reg & ~(GENMASK(4, 0) << shift)) | (input_bit << shift);
}

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, DIF_CLC);
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
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
	// ERRIRQSM is retained while CLC is disabled and may be configured by a previous payload.
	test_eq_u32("ERRIRQSS reset value", 0, DIF_ERRIRQSS);
	test_eq_u32("RIS reset value", 0, DIF_RIS);
	test_eq_u32("IMSC reset value", 0, DIF_IMSC);
	test_eq_u32("MIS reset value", 0, DIF_MIS);
	test_eq_u32("DMAE reset value", 0, DIF_DMAE);
}

static bool dif_wait_idle(void) {
	stopwatch_t started = stopwatch_get();

	while ((DIF_STAT & DIF_STAT_BSY) != 0 && stopwatch_elapsed_ms(started) < DIF_TIMEOUT_MS)
		test_watchdog_serve();

	return (DIF_STAT & DIF_STAT_BSY) == 0;
}

static bool lcd_set_parallel_timings(uint32_t lcdtim1, uint32_t lcdtim2) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_LCDTIM1 = lcdtim1;
	DIF_LCDTIM2 = lcdtim2;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;

	return true;
}

static bool dif_write_word(bool command, uint32_t value) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | (command ? lcd_command_bsconf : lcd_data_bsconf) |
		(command ? DIF_CSREG_CD : 0);
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TXD = value;

	return dif_wait_idle();
}

static bool dif_write_queued_pair(bool command, uint8_t first, uint8_t second) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | (command ? lcd_command_bsconf : lcd_data_bsconf) |
		(command ? DIF_CSREG_CD : 0);
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TXD = first;
	DIF_TXD = second;

	return dif_wait_idle();
}

static bool dif_write_packed(bool command, uint32_t bsconf, uint32_t value, bool use_tps) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | bsconf | (command ? DIF_CSREG_CD : 0);
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TXD = value;
	if (use_tps)
		DIF_TPS_CTRL = 1;

	return dif_wait_idle();
}

static uint32_t pack_alternating_words(
	const struct lcd_packed_bsconf_profile *profile,
	uint32_t word_count,
	uint32_t offset,
	uint16_t first,
	uint16_t second
) {
	uint32_t value = 0;

	for (uint32_t word = 0; word < word_count; word++) {
		uint32_t bus_word = ((offset + word) & 1) == 0 ? first : second;

		value |= bus_word << (word * profile->word_bits);
	}

	return value;
}

static bool dif_write_alternating_stream(
	bool command,
	const struct lcd_packed_bsconf_profile *profile,
	bool use_tps,
	uint16_t first,
	uint16_t second,
	uint32_t *stream_words
) {
	uint32_t words_per_write = use_tps ? profile->tps_words : profile->polling_words;
	uint32_t writes = (words_per_write & 1) == 0 ? 1 : 2;
	bool success = true;

	for (uint32_t write = 0; write < writes; write++) {
		uint32_t offset = write * words_per_write;
		uint32_t value = pack_alternating_words(profile, words_per_write, offset, first, second);

		success &= dif_write_packed(command, profile->value, value, use_tps);
	}
	*stream_words = words_per_write * writes;

	return success;
}

static bool dif_write_alternating_row(
	const struct lcd_packed_bsconf_profile *profile,
	bool use_tps,
	uint16_t first,
	uint16_t second
) {
	uint32_t words_per_write = use_tps ? profile->tps_words : profile->polling_words;
	bool success = true;

	for (uint32_t offset = 0; offset < LCD_WIDTH * 2; offset += words_per_write) {
		uint32_t value = pack_alternating_words(profile, words_per_write, offset, first, second);

		success &= dif_write_packed(false, profile->value, value, use_tps);
		if ((offset & 0xFF) == 0)
			test_watchdog_serve();
	}

	return success;
}

static bool lcd_write_command(uint16_t index) {
	return dif_write_word(true, index >> 8) && dif_write_word(true, index & 0xFF);
}

static bool lcd_write_queued_command(uint16_t index) {
	return dif_write_queued_pair(true, index >> 8, index & 0xFF);
}

static bool dif_write_pbc_pixel_packet(uint16_t color) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | lcd_data_bsconf;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TPS_CTRL = 4;
	DIF_TXD = 0x12;
	DIF_TXD = color >> 8;
	DIF_TXD = 0x34;
	DIF_TXD = color & 0xFF;

	return dif_wait_idle();
}

static bool lcd_write_register(uint16_t index, uint16_t value) {
	return lcd_write_command(index) && dif_write_word(false, value >> 8) && dif_write_word(false, value & 0xFF);
}

static bool lcd_write_queued_register(uint16_t index, uint16_t value) {
	if (!dif_wait_idle())
		return false;
	/* Change CD immediately after queuing both command bytes. */
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_CD | lcd_command_bsconf;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TXD = index >> 8;
	DIF_TXD = index & 0xFF;
	DIF_CSREG = DIF_CSREG_CS1 | lcd_data_bsconf;
	DIF_TXD = value >> 8;
	DIF_TXD = value & 0xFF;

	return dif_wait_idle();
}

static bool lcd_set_gram_address(uint16_t x, uint16_t y) {
	return lcd_write_register(0x0200, x) && lcd_write_register(0x0201, y);
}

static bool lcd_set_gram_window(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end) {
	return lcd_write_register(0x0406, x_start) &&
		lcd_write_register(0x0407, x_end) &&
		lcd_write_register(0x0408, y_start) &&
		lcd_write_register(0x0409, y_end);
}

static bool dif_read_bytes_selected(uint32_t chip_select, bool data, uint8_t *buffer, uint32_t size) {
	uint32_t previous_lcdtim1 = DIF_LCDTIM1;
	uint32_t previous_lcdtim2 = DIF_LCDTIM2;

	if (!lcd_set_parallel_timings(LCD_READ_TIM1, LCD_READ_TIM2))
		return false;
	DIF_RUNCTRL = 0;
	DIF_RXFIFO_CFG = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	DIF_CSREG = chip_select | (data ? lcd_data_bsconf : lcd_command_bsconf) |
		(data ? 0 : DIF_CSREG_CD);
	DIF_STARTLCDRD = 0;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_STARTLCDRD = DIF_STARTLCDRD_STARTREAD | ((size - 1) << DIF_STARTLCDRD_READBYTES_SHIFT);

	for (uint32_t i = 0; i < size; i++) {
		stopwatch_t started = stopwatch_get();

		while (DIF_RXFFS_STAT == 0 && stopwatch_elapsed_ms(started) < DIF_TIMEOUT_MS)
			test_watchdog_serve();
		if (DIF_RXFFS_STAT == 0) {
			DIF_RUNCTRL = 0;
			DIF_STARTLCDRD = 0;
			DIF_LCDTIM1 = previous_lcdtim1;
			DIF_LCDTIM2 = previous_lcdtim2;
			DIF_RUNCTRL = DIF_RUNCTRL_RUN;
			return false;
		}
		buffer[i] = DIF_RXD;
	}

	bool idle = dif_wait_idle();
	DIF_STARTLCDRD = 0;

	return idle && lcd_set_parallel_timings(previous_lcdtim1, previous_lcdtim2);
}

static bool dif_read_bytes(bool data, uint8_t *buffer, uint32_t size) {
	return dif_read_bytes_selected(DIF_CSREG_CS1, data, buffer, size);
}

static void lcd_configure_dif_gpio(void) {
	uint32_t control = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_DIF_CD) = control;
	GPIO_PIN(GPIO_DIF_CS1) = control;
	GPIO_PIN(GPIO_DIF_WR) = control;
	GPIO_PIN(GPIO_DIF_RD) = control;

	GPIO_PIN(GPIO_DIF_D0) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT;
	uint32_t data = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_PDPU_PULLDOWN;
	GPIO_PIN(GPIO_DIF_D1) = data;
	GPIO_PIN(GPIO_DIF_D2) = data;
	GPIO_PIN(GPIO_DIF_D3) = data;
	GPIO_PIN(GPIO_DIF_D4) = data;
	GPIO_PIN(GPIO_DIF_D5) = data;
	GPIO_PIN(GPIO_DIF_D6) = data;
	GPIO_PIN(GPIO_DIF_D7) = data;
}

static void lcd_init_gpio(void) {
	GPIO_CLC = 1U << MOD_CLC_RMC_SHIFT;
	lcd_configure_dif_gpio();

	GPIO_PIN(GPIO_DIF_VD) = GPIO_PS_MANUAL | GPIO_DIR_IN;
	gpio_init_output(
		GPIO_DIF_RESET1,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
}

static void lcd_init_dif(void) {
	DIF_RUNCTRL = 0;
	DIF_CON = 0;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_PARALLEL;
	DIF_TXFIFO_CFG = DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC;
	DIF_RXFIFO_CFG = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	DIF_LCDTIM1 = LCD_WRITE_TIM1;
	DIF_LCDTIM2 = LCD_WRITE_TIM2;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_CD | DIF_CSREG_BSCONF_OFF;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static void lcd_enable_panel_power(void) {
	gpio_init_output(
		GPIO_LED_FL_EN,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	i2c_init();
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x06, 0x0D);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x12);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0C, 0x07);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x07, 0x09);
	stopwatch_msleep_wd(1);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x12);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x08, 0x01);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x1A);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x08, 0x03);
	stopwatch_msleep_wd(1);
}

static bool lcd_sync_parallel_interface(void) {
	gpio_init_output(
		GPIO_DIF_CD,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_DIF_RD,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);

	bool success = true;
	for (uint32_t i = 0; i < 4; i++)
		success &= dif_write_word(false, 0);

	gpio_set(GPIO_DIF_CD, true);
	gpio_set(GPIO_DIF_RD, false);
	GPIO_PIN(GPIO_DIF_CD) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_DIF_RD) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT;

	return success;
}

static bool lcd_program_controller_queued(void) {
	bool success = true;

	success &= lcd_write_queued_register(0x001D, 0x0005);
	stopwatch_msleep_wd(1);

	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_INITIAL_STATE); i++)
		success &= lcd_write_queued_register(LCD_INITIAL_STATE[i].index, LCD_INITIAL_STATE[i].value);

	success &= lcd_write_queued_register(0x0100, 0xC010);
	stopwatch_msleep_wd(30);
	success &= lcd_write_queued_register(0x0101, 0x0001);

	return success;
}

static bool lcd_init_controller(void) {
	bool success = true;

	for (uint32_t i = 0; i < 3; i++) {
		success &= lcd_write_command(0x0000);
		stopwatch_msleep_wd(1);
	}
	success &= lcd_write_register(0x05FF, 0x0000);
	success &= lcd_sync_parallel_interface();
	success &= lcd_write_register(0x001D, 0x0005);
	stopwatch_msleep_wd(1);

	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_INITIAL_STATE); i++)
		success &= lcd_write_register(LCD_INITIAL_STATE[i].index, LCD_INITIAL_STATE[i].value);

	success &= lcd_write_register(0x0100, 0xC010);
	stopwatch_msleep_wd(30);
	success &= lcd_write_register(0x0101, 0x0001);
	success &= lcd_write_register(0x0100, 0xF7FE);

	return success;
}

static void lcd_reset_controller(void) {
	gpio_set(GPIO_DIF_RESET1, false);
	stopwatch_msleep_wd(10);
	gpio_set(GPIO_DIF_RESET1, true);
	stopwatch_msleep_wd(120);
}

static bool lcd_reset_and_init_controller(void) {
	lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	lcd_reset_controller();

	return lcd_init_controller();
}

static bool lcd_init_queued_polling_sequence(void) {
	uint8_t id[5] = { 0 };

	lcd_command_bsconf = DIF_CSREG_BSCONF_OFF;
	lcd_data_bsconf = DIF_CSREG_BSCONF_OFF;
	lcd_reset_controller();
	bool success = dif_write_word(true, 0x04) && dif_read_bytes(true, id, 4);
	success &= dif_write_word(true, 0xD3) && dif_read_bytes(true, id, 5);
	for (uint32_t i = 0; i < 3; i++) {
		success &= dif_write_word(true, 0x00);
		stopwatch_msleep_wd(1);
	}
	success &= lcd_write_queued_register(0x001D, 0x0005);
	success &= lcd_program_controller_queued();
	success &= lcd_write_queued_register(0x0100, 0xF7FE);
	lcd_command_bsconf = DIF_CSREG_BSCONF_2x9BIT;
	lcd_data_bsconf = DIF_CSREG_BSCONF_2x9BIT;

	return success;
}

static void lcd_enable_backlight(void) {
	i2c_smbus_write_byte(
		D1601AA_I2C_ADDR,
		D1601AA_LED_CONTROL,
		D1601AA_LED2_EN | D1601AA_LIGHT_PWM1_EN
	);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM1, 0x50);
}

static bool lcd_draw_solid(uint32_t bsconf, uint16_t y, uint16_t height, uint16_t color) {
	lcd_data_bsconf = bsconf;
	bool success = lcd_write_register(0x0200, 0x00EF);
	success &= lcd_write_register(0x0201, y);
	success &= lcd_write_command(0x0202);

	for (uint32_t pixel = 0; pixel < LCD_WIDTH * height; pixel++) {
		success &= dif_write_word(false, color >> 8);
		success &= dif_write_word(false, color & 0xFF);
		if ((pixel & 0xFF) == 0)
			test_watchdog_serve();
	}

	return success;
}

static bool lcd_draw_solid_queued(uint16_t y, uint16_t height, uint16_t color) {
	bool success = lcd_write_queued_register(0x0200, 0x00EF);
	success &= lcd_write_queued_register(0x0201, y);
	success &= lcd_write_queued_command(0x0202);

	for (uint32_t pixel = 0; pixel < LCD_WIDTH * height; pixel++) {
		success &= dif_write_queued_pair(false, color >> 8, color & 0xFF);
		if ((pixel & 0xFF) == 0)
			test_watchdog_serve();
	}

	return success;
}

static bool lcd_prepare_gram(uint16_t y) {
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;

	return lcd_write_register(0x0200, 0x00EF) &&
		lcd_write_register(0x0201, y) &&
		lcd_write_command(0x0202);
}

static bool lcd_write_pixels(uint32_t bsconf, uint16_t height, uint16_t color) {
	lcd_data_bsconf = bsconf;
	bool success = true;

	for (uint32_t pixel = 0; pixel < LCD_WIDTH * height; pixel++) {
		success &= dif_write_word(false, color >> 8);
		success &= dif_write_word(false, color & 0xFF);
		if ((pixel & 0xFF) == 0)
			test_watchdog_serve();
	}

	return success;
}

static bool lcd_read_gram(uint32_t bsconf, uint16_t y, uint8_t *data, uint32_t size) {
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	bool success = lcd_write_register(0x0200, 0x00EF) &&
		lcd_write_register(0x0201, y) &&
		lcd_write_command(0x0202);
	lcd_data_bsconf = bsconf;

	return success && dif_read_bytes(true, data, size);
}

static bool lcd_read_gram_bytes(uint16_t y, uint8_t *data, uint32_t size) {
	uint8_t gram[LCD_GRAM_READ_DUMMY_BYTES + LCD_GRAM_TEST_BYTES_MAX] = { 0 };

	if (size > LCD_GRAM_TEST_BYTES_MAX ||
		!lcd_read_gram(DIF_CSREG_BSCONF_1x8BIT, y, gram, LCD_GRAM_READ_DUMMY_BYTES + size))
		return false;
	for (uint32_t i = 0; i < size; i++)
		data[i] = gram[LCD_GRAM_READ_DUMMY_BYTES + i];

	return true;
}

static bool lcd_read_pixel_1x8(uint16_t y, uint16_t *pixel) {
	uint8_t gram[8] = { 0 };

	lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	if (!lcd_read_gram(DIF_CSREG_BSCONF_1x8BIT, y, gram, sizeof(gram)))
		return false;
	*pixel = gram[4] << 8 | gram[5];

	return true;
}

static bool lcd_read_pixel_xy_1x8(uint16_t x, uint16_t y, uint16_t *pixel) {
	uint8_t gram[LCD_GRAM_READ_DUMMY_BYTES + 2] = { 0 };
	lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	if (!lcd_set_gram_window(x, x, y, y) || !lcd_set_gram_address(x, y) || !lcd_write_command(0x0202) ||
		!dif_read_bytes(true, gram, sizeof(gram)))
		return false;
	*pixel = gram[LCD_GRAM_READ_DUMMY_BYTES] << 8 | gram[LCD_GRAM_READ_DUMMY_BYTES + 1];

	return true;
}

static bool lcd_write_rgb565_sequence(const uint16_t *colors, uint32_t count) {
	bool success = true;

	for (uint32_t i = 0; i < count; i++) {
		success &= dif_write_word(false, colors[i] >> 8);
		success &= dif_write_word(false, colors[i]);
	}

	return success;
}

static bool lcd_fill_gram_window(
	uint16_t x_start,
	uint16_t x_end,
	uint16_t y_start,
	uint16_t y_end,
	uint16_t color
) {
	lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	bool success = lcd_write_register(0x0003, LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0) &&
		lcd_set_gram_window(x_start, x_end, y_start, y_end) &&
		lcd_set_gram_address(x_start, y_start) &&
		lcd_write_command(0x0202);
	uint32_t pixel_count = (x_end - x_start + 1) * (y_end - y_start + 1);

	for (uint32_t i = 0; i < pixel_count; i++) {
		success &= dif_write_word(false, color >> 8);
		success &= dif_write_word(false, color);
	}

	return success;
}

static bool lcd_read_horizontal_span(uint16_t x_start, uint16_t y, uint16_t *pixels, uint32_t count) {
	bool success = true;

	for (uint32_t i = 0; i < count; i++)
		success &= lcd_read_pixel_xy_1x8(x_start + i, y, &pixels[i]);

	return success;
}

static bool lcd_read_vertical_span(uint16_t x, uint16_t y_start, uint16_t *pixels, uint32_t count) {
	bool success = true;

	for (uint32_t i = 0; i < count; i++)
		success &= lcd_read_pixel_xy_1x8(x, y_start + i, &pixels[i]);

	return success;
}

enum dif_conversion {
	DIF_CONVERSION_BM,
	DIF_CONVERSION_BC,
	DIF_CONVERSION_PBC,
	DIF_CONVERSION_INVERT,
};

static void configure_conversion(enum dif_conversion conversion) {
	DIF_RUNCTRL = 0;
	reset_data_conversion();
	if (conversion == DIF_CONVERSION_BM) {
		set_bmreg_mapping(0, 1);
		set_bmreg_mapping(1, 0);
	} else if (conversion == DIF_CONVERSION_BC) {
		DIF_BCSEL0 = 1;
		DIF_BCREG = 1;
	} else if (conversion == DIF_CONVERSION_PBC) {
		DIF_CON = DIF_CON_BM_8;
		DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	} else {
		DIF_INVERT_BIT = 1;
	}
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static void disable_conversion(void) {
	DIF_RUNCTRL = 0;
	reset_data_conversion();
	DIF_CON = 0;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static void configure_pbc_data_conversion(void);

static void configure_read_conversion(enum dif_conversion conversion) {
	if (conversion == DIF_CONVERSION_PBC) {
		configure_pbc_data_conversion();
	} else {
		configure_conversion(conversion);
	}
	if (conversion == DIF_CONVERSION_BM) {
		DIF_RUNCTRL = 0;
		reset_data_conversion();
		set_bmreg_mapping(0, 3);
		set_bmreg_mapping(3, 0);
		DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	}
}

static bool dif_read_bytes_with_conversion(
	enum dif_conversion conversion,
	bool data,
	uint8_t *buffer,
	uint32_t size
) {
	configure_read_conversion(conversion);
	bool success = dif_read_bytes(data, buffer, size);
	disable_conversion();

	return success;
}

static void configure_pbc_data_conversion(void) {
	DIF_RUNCTRL = 0;
	reset_data_conversion();
	DIF_CON = DIF_CON_BM_16;
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	DIF_BMREG0 = 0x56934A30;
	DIF_BMREG1 = 0x2D4922F6;
	DIF_BMREG2 = 0x040F39AC;
	DIF_BMREG3 = 0x1CC51062;
	DIF_BMREG4 = 0x779B6B38;
	DIF_BMREG5 = 0x000003FE;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static bool probe_pbc_lcd_data(uint16_t y, uint16_t color, uint16_t *pixel) {
	const uint16_t BASELINE_COLOR = 0x001F;

	if (!lcd_reset_and_init_controller() ||
		!lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, y, 1, BASELINE_COLOR) ||
		!lcd_prepare_gram(y))
		return false;
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	configure_pbc_data_conversion();
	bool success = dif_write_pbc_pixel_packet(color);
	disable_conversion();
	success &= lcd_read_pixel_1x8(y, pixel);

	return success;
}

static bool lcd_write_pbc_gram_command(void) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	reset_data_conversion();
	DIF_CON = DIF_CON_BM_16;
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_CD | DIF_CSREG_BSCONF_2x8BIT;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TPS_CTRL = 2;
	DIF_TXD = 0x0202;
	DIF_TXD = 0x0303;

	return dif_wait_idle();
}

static bool lcd_write_gram_command_with_conversion(enum dif_conversion conversion) {
	bool success;

	if (conversion == DIF_CONVERSION_PBC) {
		success = lcd_write_pbc_gram_command();
	} else {
		DIF_RUNCTRL = 0;
		reset_data_conversion();
		uint16_t command = 0x0202;

		if (conversion == DIF_CONVERSION_BM) {
			set_bmreg_mapping(0, 1);
			set_bmreg_mapping(1, 0);
		} else if (conversion == DIF_CONVERSION_BC) {
			DIF_BCSEL0 = 1;
			command = 0x0303;
		} else {
			DIF_INVERT_BIT = 1;
			command = 0x0303;
		}
		DIF_RUNCTRL = DIF_RUNCTRL_RUN;
		success = lcd_write_command(command);
	}
	disable_conversion();

	return success;
}

static bool probe_lcd_command_conversion(enum dif_conversion conversion, uint16_t y, uint16_t *pixel) {
	const uint16_t BASELINE_COLOR = 0x001F;
	const uint16_t COMMAND_COLOR = 0x1234;

	if (!lcd_reset_and_init_controller() ||
		!lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, y, 1, BASELINE_COLOR))
		return false;
	lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	bool success = lcd_write_register(0x0200, 0x00EF) && lcd_write_register(0x0201, y);
	success &= lcd_write_gram_command_with_conversion(conversion);
	success &= lcd_write_pixels(DIF_CSREG_BSCONF_1x8BIT, 1, COMMAND_COLOR);
	success &= lcd_read_pixel_1x8(y, pixel);

	return success;
}

static int32_t find_pixel(const uint8_t *data, uint32_t size, uint16_t pixel) {
	for (uint32_t i = 0; i + 1 < size; i++) {
		if (data[i] == (pixel >> 8) && data[i + 1] == (pixel & 0xFF))
			return i;
	}

	return -1;
}

static bool lcd_read_device_code(uint8_t code[LCD_DEVICE_CODE_READ_BYTES]) {
	return lcd_write_command(0x0000) && dif_read_bytes(true, code, LCD_DEVICE_CODE_READ_BYTES);
}

static uint32_t encode_pixel(
	const struct lcd_pixel_format *format,
	uint32_t rgb666,
	uint8_t encoded[3]
) {
	uint8_t red = (rgb666 >> 12) & 0x3F;
	uint8_t green = (rgb666 >> 6) & 0x3F;
	uint8_t blue = rgb666 & 0x3F;

	if (format->entry_bits == 0) {
		encoded[0] = (red & 0x3E) << 2 | green >> 3;
		encoded[1] = green << 5 | blue >> 1;
	} else if (format->entry_bits == (LCD_ENTRY_MODE_TRI | LCD_ENTRY_MODE_DFM0)) {
		encoded[0] = red << 2 | green >> 4;
		encoded[1] = green << 4 | blue >> 2;
		encoded[2] = blue << 6;
	} else if (format->entry_bits == (LCD_ENTRY_MODE_TRI | LCD_ENTRY_MODE_DFM1)) {
		encoded[0] = red >> 4;
		encoded[1] = red << 4 | green >> 2;
		encoded[2] = green << 6 | blue;
	} else {
		encoded[0] = red << 2;
		encoded[1] = green << 2;
		encoded[2] = blue << 2;
	}

	return format->bytes_per_pixel;
}

static int32_t find_masked_bytes(
	const uint8_t *data,
	uint32_t data_size,
	const uint8_t *expected,
	const uint8_t *masks,
	uint32_t expected_size
) {
	for (uint32_t offset = 0; offset + expected_size <= data_size; offset++) {
		bool match = true;

		for (uint32_t i = 0; i < expected_size; i++) {
			if ((data[offset + i] & masks[i]) != expected[i]) {
				match = false;
				break;
			}
		}
		if (match)
			return offset;
	}

	return -1;
}

static bool run_entry_mode(uint32_t id, bool am, uint16_t expected[4], uint16_t actual[4]) {
	static const uint16_t COLORS[] = { 0xF800, 0x07E0, 0x001F, 0xFFFF };
	const uint16_t X_START = 8;
	const uint16_t X_END = 9;
	const uint16_t Y_START = 24;
	const uint16_t Y_END = 25;
	bool horizontal_increment = (id & 1) != 0;
	bool vertical_increment = (id & 2) != 0;
	uint16_t entry_mode = LCD_ENTRY_MODE_BGR | (id << 4) | (am ? LCD_ENTRY_MODE_AM : 0);
	uint16_t start_x = horizontal_increment ? X_START : X_END;
	uint16_t start_y = vertical_increment ? Y_START : Y_END;
	bool success = lcd_fill_gram_window(X_START, X_END, Y_START, Y_END, 0x39E7) &&
		lcd_write_register(0x0003, entry_mode) &&
		lcd_set_gram_window(X_START, X_END, Y_START, Y_END) &&
		lcd_set_gram_address(start_x, start_y) &&
		lcd_write_command(0x0202);

	for (uint32_t i = 0; i < ARRAY_SIZE(COLORS); i++) {
		success &= dif_write_word(false, COLORS[i] >> 8);
		success &= dif_write_word(false, COLORS[i]);
		uint32_t primary = i & 1;
		uint32_t secondary = i >> 1;
		uint32_t x_step = am ? secondary : primary;
		uint32_t y_step = am ? primary : secondary;
		uint32_t x = horizontal_increment ? x_step : 1 - x_step;
		uint32_t y = vertical_increment ? y_step : 1 - y_step;

		expected[y * 2 + x] = COLORS[i];
	}

	success &= lcd_write_register(0x0003, LCD_ENTRY_MODE_BGR | LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0);
	for (uint32_t y = 0; y < 2; y++) {
		for (uint32_t x = 0; x < 2; x++)
			success &= lcd_read_pixel_xy_1x8(X_START + x, Y_START + y, &actual[y * 2 + x]);
	}

	return success;
}

static bool run_pixel_format(const struct lcd_pixel_format *format, int32_t *pixel_offset) {
	static const uint32_t COLORS[] = { 0x2B554, 0x12AAB };
	const struct lcd_pixel_format *read_format = &LCD_PIXEL_FORMATS[3];
	const uint16_t X_START = 16;
	const uint16_t Y = 32;
	uint8_t write_data[6] = { 0 };
	uint8_t expected[6] = { 0 };
	uint8_t masks[6] = { 0 };
	uint8_t gram[24] = { 0 };
	uint32_t write_size = 0;
	uint32_t expected_size = 0;
	uint16_t write_entry_mode = LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0 | format->entry_bits;
	uint16_t read_entry_mode = LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0 | read_format->entry_bits;

	for (uint32_t pixel = 0; pixel < ARRAY_SIZE(COLORS); pixel++) {
		uint32_t encoded_size = encode_pixel(format, COLORS[pixel], &write_data[write_size]);
		write_size += encoded_size;
		encoded_size = encode_pixel(read_format, COLORS[pixel], &expected[expected_size]);
		expected_size += encoded_size;
	}
	for (uint32_t i = 0; i < expected_size; i++) {
		masks[i] = read_format->byte_masks[i % read_format->bytes_per_pixel];
		expected[i] &= masks[i];
	}

	bool success = lcd_fill_gram_window(X_START, X_START + ARRAY_SIZE(COLORS) - 1, Y, Y, 0x0000) &&
		lcd_write_register(0x0003, write_entry_mode) &&
		lcd_set_gram_window(X_START, X_START + ARRAY_SIZE(COLORS) - 1, Y, Y) &&
		lcd_set_gram_address(X_START, Y) && lcd_write_command(0x0202);
	for (uint32_t i = 0; i < write_size; i++)
		success &= dif_write_word(false, write_data[i]);
	success &= lcd_write_register(0x0003, read_entry_mode) &&
		lcd_set_gram_address(X_START, Y) && lcd_write_command(0x0202) &&
		dif_read_bytes(true, gram, sizeof(gram));
	uint32_t invalid_pixel_size = read_format->bytes_per_pixel;
	*pixel_offset = find_masked_bytes(
		gram,
		sizeof(gram),
		&expected[invalid_pixel_size],
		&masks[invalid_pixel_size],
		read_format->bytes_per_pixel
	);
	if (*pixel_offset < 0) {
		printf("# unmatched GRAM bytes:");
		for (uint32_t i = 0; i < sizeof(gram); i++)
			printf(" %02X", gram[i]);
		printf("\n");
	}

	return success;
}

static void run_lcd_controller_tests(void) {
	test_category("JBT6K71 device code");
	uint8_t initialized_code[LCD_DEVICE_CODE_READ_BYTES] = { 0 };
	test_check("controller initialization before device code read", lcd_reset_and_init_controller());
	test_check("initialized device code read completes", lcd_read_device_code(initialized_code));
	printf("# index 0000 RS=1 read:");
	for (uint32_t i = 0; i < sizeof(initialized_code); i++)
		printf(" %02X", initialized_code[i]);
	printf("\n");
	bool repeated_code = true;
	for (uint32_t i = 2; i < sizeof(initialized_code); i += 2)
		repeated_code &= initialized_code[i] == initialized_code[0] && initialized_code[i + 1] == initialized_code[1];
	test_check("continuous reads repeat the same device code", repeated_code);
	test_eq_u32("device code matches the documented 71xx family", 0x71, initialized_code[0]);
	test_eq_u32("EL71 LCD reports device code 7114", 0x7114,
		(initialized_code[0] << 8) | initialized_code[1]);

	test_category("GRAM window X1/X2/Y1/Y2 registers");
	enum { BASELINE_COLOR = 0x39E7 };
	static const uint16_t HORIZONTAL_COLORS[] = { 0xF800, 0x07E0 };
	static const uint16_t HORIZONTAL_EXPECTED[] = {
		BASELINE_COLOR, 0x07E0, BASELINE_COLOR, 0xF800, BASELINE_COLOR,
	};
	uint16_t horizontal_actual[ARRAY_SIZE(HORIZONTAL_EXPECTED)] = { 0 };
	test_check("controller reset before window register tests", lcd_reset_and_init_controller());
	test_check("X1/X2 baseline fill completes", lcd_fill_gram_window(0xE8, 0xEC, 40, 40, BASELINE_COLOR));
	test_check("X1/X2 bounded write completes",
		lcd_write_register(0x0003, LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0) &&
		lcd_set_gram_window(0xE9, 0xEB, 40, 40) && lcd_set_gram_address(0xEB, 40) &&
		lcd_write_command(0x0202) &&
		lcd_write_rgb565_sequence(HORIZONTAL_COLORS, ARRAY_SIZE(HORIZONTAL_COLORS)));
	test_check("X1/X2 result reads complete",
		lcd_read_horizontal_span(0xE8, 40, horizontal_actual, ARRAY_SIZE(horizontal_actual)));
	test_eq_memory("X1/X2 contain writes and wrap from X2 to X1",
		HORIZONTAL_EXPECTED, horizontal_actual, sizeof(horizontal_actual));

	static const uint16_t VERTICAL_COLORS[] = { 0x001F, 0xFFE0 };
	static const uint16_t VERTICAL_EXPECTED[] = {
		BASELINE_COLOR, 0xFFE0, BASELINE_COLOR, 0x001F, BASELINE_COLOR,
	};
	uint16_t vertical_actual[ARRAY_SIZE(VERTICAL_EXPECTED)] = { 0 };
	test_check("Y1/Y2 baseline fill completes", lcd_fill_gram_window(30, 30, 0xFF, 0x103, BASELINE_COLOR));
	test_check("Y1/Y2 bounded write completes",
		lcd_write_register(0x0003, LCD_ENTRY_MODE_ID1 | LCD_ENTRY_MODE_ID0 | LCD_ENTRY_MODE_AM) &&
		lcd_set_gram_window(30, 30, 0x100, 0x102) && lcd_set_gram_address(30, 0x102) &&
		lcd_write_command(0x0202) &&
		lcd_write_rgb565_sequence(VERTICAL_COLORS, ARRAY_SIZE(VERTICAL_COLORS)));
	test_check("Y1/Y2 result reads complete",
		lcd_read_vertical_span(30, 0xFF, vertical_actual, ARRAY_SIZE(vertical_actual)));
	test_eq_memory("Y1/Y2 contain writes and wrap from Y2 to Y1",
		VERTICAL_EXPECTED, vertical_actual, sizeof(vertical_actual));

	test_category("GRAM current X/Y address registers");
	static const uint16_t CURRENT_X_EXPECTED[] = {
		BASELINE_COLOR, BASELINE_COLOR, 0xF81F, BASELINE_COLOR, BASELINE_COLOR,
	};
	uint16_t current_x_actual[ARRAY_SIZE(CURRENT_X_EXPECTED)] = { 0 };
	test_check("current X baseline fill completes", lcd_fill_gram_window(0xD9, 0xDD, 60, 60, BASELINE_COLOR));
	test_check("current X selects the requested column",
		lcd_set_gram_window(0xD9, 0xDD, 60, 60) && lcd_set_gram_address(0xDB, 60) &&
		lcd_write_command(0x0202) && dif_write_word(false, 0xF8) && dif_write_word(false, 0x1F));
	test_check("current X result reads complete",
		lcd_read_horizontal_span(0xD9, 60, current_x_actual, ARRAY_SIZE(current_x_actual)));
	test_eq_memory("register 0200 sets only the requested X address",
		CURRENT_X_EXPECTED, current_x_actual, sizeof(current_x_actual));

	static const uint16_t CURRENT_Y_EXPECTED[] = {
		BASELINE_COLOR, BASELINE_COLOR, 0x07FF, BASELINE_COLOR, BASELINE_COLOR,
	};
	uint16_t current_y_actual[ARRAY_SIZE(CURRENT_Y_EXPECTED)] = { 0 };
	test_check("current Y baseline fill completes", lcd_fill_gram_window(50, 50, 0x109, 0x10D, BASELINE_COLOR));
	test_check("current Y selects the requested row",
		lcd_set_gram_window(50, 50, 0x109, 0x10D) && lcd_set_gram_address(50, 0x10B) &&
		lcd_write_command(0x0202) && dif_write_word(false, 0x07) && dif_write_word(false, 0xFF));
	test_check("current Y result reads complete",
		lcd_read_vertical_span(50, 0x109, current_y_actual, ARRAY_SIZE(current_y_actual)));
	test_eq_memory("register 0201 sets only the requested Y address",
		CURRENT_Y_EXPECTED, current_y_actual, sizeof(current_y_actual));

	test_category("GRAM entry addressing ID1/ID0/AM");
	test_check("controller reset before entry mode matrix", lcd_reset_and_init_controller());
	uint32_t entry_mode_coverage = 0;
	for (uint32_t am = 0; am < 2; am++) {
		for (uint32_t id = 0; id < 4; id++) {
			uint16_t expected[4] = { 0 };
			uint16_t actual[ARRAY_SIZE(expected)] = { 0 };

			printf("# entry mode: ID1/ID0=%u%u AM=%u\n", (unsigned int) (id >> 1),
				(unsigned int) (id & 1), (unsigned int) am);
			test_check("entry mode GRAM transaction completes", run_entry_mode(id, am != 0, expected, actual));
			test_eq_memory("entry mode follows the datasheet address route",
				expected, actual, sizeof(expected));
			entry_mode_coverage |= BIT(am * 4 + id);
		}
	}
	test_eq_u32("ID1/ID0/AM matrix covers all eight combinations", 0xFF, entry_mode_coverage);

	test_category("8-bit MPU pixel formats");
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_PIXEL_FORMATS); i++) {
		const struct lcd_pixel_format *format = &LCD_PIXEL_FORMATS[i];
		int32_t pixel_offset = -1;

		printf("# pixel format: %s\n", format->name);
		test_check("controller reset before pixel format", lcd_reset_and_init_controller());
		test_check("pixel format GRAM round-trip completes", run_pixel_format(format, &pixel_offset));
		printf("# pixel format data offset: %d\n", (int) pixel_offset);
		test_check("pixel format preserves the valid RGB pixel after the invalid first read", pixel_offset >= 0);
	}

	test_category("BGR output routing");
	uint16_t bgr_off[2] = { 0 };
	uint16_t bgr_on[ARRAY_SIZE(bgr_off)] = { 0 };
	test_check("controller reset before visible BGR pattern", lcd_reset_and_init_controller());
	test_check("visible red BGR stripe completes",
		lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, 64, 16, 0xF800));
	test_check("visible blue BGR stripe completes",
		lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, 80, 16, 0x001F));
	test_check("BGR-off GRAM reads complete",
		lcd_read_pixel_xy_1x8(0x00EF, 64, &bgr_off[0]) &&
		lcd_read_pixel_xy_1x8(0x00EF, 80, &bgr_off[1]));
	test_check("BGR output swap enables", lcd_write_register(0x0003, 0x0120 | LCD_ENTRY_MODE_BGR));
	test_check("BGR-on GRAM reads complete",
		lcd_read_pixel_xy_1x8(0x00EF, 64, &bgr_on[0]) &&
		lcd_read_pixel_xy_1x8(0x00EF, 80, &bgr_on[1]));
	test_eq_memory("BGR changes panel output without changing GRAM", bgr_off, bgr_on, sizeof(bgr_off));
}

int main(void) {
	test_start(LCD_CONTROLLER_TEST ? "JBT6K71 EL71 LCD controller test" : "DIFv2 EL71 LCD test");
	test_reset_values();
	test_module_id("module ID", 0xF043C000, DIF_ID);
	lcd_init_gpio();
	lcd_init_dif();
	lcd_enable_panel_power();
	if (LCD_CONTROLLER_TEST) {
		lcd_enable_backlight();
		run_lcd_controller_tests();
		return test_finish();
	}

	uint16_t probe_pixel = 0;
	test_category("TXD ordering across CD changes");
	test_check("queued polling initialization transfers complete", lcd_init_queued_polling_sequence());
	lcd_enable_backlight();
	test_check("queued 2x9 GRAM write completes",
		lcd_draw_solid_queued(0, 1, 0x1234));
	test_check("GRAM read after queued write completes", lcd_read_pixel_1x8(0, &probe_pixel));
	test_eq_u32("later CSREG CD does not reclassify earlier TXD", 0x1234, probe_pixel);

	test_category("Direct polling command BSCONF writes");
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_DIRECT_PROFILES); i++) {
		const struct lcd_bsconf_profile *profile = &LCD_DIRECT_PROFILES[i];
		uint16_t pixel = 0;

		printf("# command BSCONF %s\n", profile->name);
		test_check("controller reset before command BSCONF", lcd_reset_and_init_controller());
		lcd_command_bsconf = profile->value;
		lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
		test_check("command BSCONF stripe write completes",
			lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, 0, 16, profile->color));
		test_check("command BSCONF stripe read completes", lcd_read_pixel_1x8(0, &pixel));
		test_eq_u32("command BSCONF selects the correct GRAM address", profile->color, pixel);
	}

	test_category("Direct polling data BSCONF writes");
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_DIRECT_PROFILES); i++) {
		const struct lcd_bsconf_profile *profile = &LCD_DIRECT_PROFILES[i];
		uint16_t pixel = 0;

		printf("# data BSCONF %s\n", profile->name);
		test_check("controller reset before data BSCONF", lcd_reset_and_init_controller());
		lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
		test_check("data BSCONF stripe write completes", lcd_draw_solid(profile->value, 0, 16, profile->color));
		test_check("data BSCONF stripe read completes", lcd_read_pixel_1x8(0, &pixel));
		test_eq_u32("data BSCONF writes the expected RGB565 pixel", profile->color, pixel);
	}

	for (uint32_t tps = 0; tps < 2; tps++) {
		bool use_tps = tps != 0;

		test_category(use_tps ? "TPS-packed command BSCONF writes" : "Polling-packed command BSCONF writes");
		for (uint32_t i = 0; i < ARRAY_SIZE(LCD_PACKED_BSCONF_PROFILES); i++) {
			const struct lcd_packed_bsconf_profile *profile = &LCD_PACKED_BSCONF_PROFILES[i];
			uint16_t command_word = profile->word_bits == 9 ? 0x102 : 0x02;
			uint32_t stream_words = 0;
			uint16_t pixel = 0;

			printf("# %s command BSCONF %s\n", use_tps ? "TPS" : "polling", profile->name);
			test_check("controller reset before packed command", lcd_reset_and_init_controller());
			test_check("packed command baseline write completes",
				lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, 0, 1, 0x001F));
			lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
			lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
			test_check("packed command GRAM address write completes",
				lcd_write_register(0x0200, 0x00EF) && lcd_write_register(0x0201, 0));
			test_check("packed GRAM command completes", dif_write_alternating_stream(
				true,
				profile,
				use_tps,
				command_word,
				command_word,
				&stream_words
			));
			test_check("packed command emits complete instruction pairs", (stream_words & 1) == 0);
			lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
			lcd_data_bsconf = DIF_CSREG_BSCONF_1x8BIT;
			test_check("row write after packed command completes",
				lcd_write_pixels(DIF_CSREG_BSCONF_1x8BIT, 1, 0x1234));
			test_check("pixel read after packed command completes", lcd_read_pixel_1x8(0, &pixel));
			test_eq_u32("packed BSCONF command selects GRAM", 0x1234, pixel);
		}

		test_category(use_tps ? "TPS-packed data BSCONF writes" : "Polling-packed data BSCONF writes");
		for (uint32_t i = 0; i < ARRAY_SIZE(LCD_PACKED_BSCONF_PROFILES); i++) {
			const struct lcd_packed_bsconf_profile *profile = &LCD_PACKED_BSCONF_PROFILES[i];
			uint16_t first = profile->word_bits == 9 ? 0x112 : 0x12;
			uint16_t second = profile->word_bits == 9 ? 0x134 : 0x34;
			uint8_t expected[LCD_GRAM_TEST_BYTES_MAX] = { 0 };
			uint8_t actual[sizeof(expected)] = { 0 };
			uint32_t stream_words = LCD_GRAM_TEST_BYTES_MAX;

			printf("# %s data BSCONF %s\n", use_tps ? "TPS" : "polling", profile->name);
			test_check("controller reset before packed data", lcd_reset_and_init_controller());
			test_check("packed data prepares GRAM", lcd_prepare_gram(0));
			test_check("packed GRAM row completes", dif_write_alternating_row(
				profile,
				use_tps,
				first,
				second
			));
			for (uint32_t word = 0; word < stream_words; word++)
				expected[word] = (word & 1) == 0 ? 0x12 : 0x34;
			lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
			test_check("packed GRAM data read completes", lcd_read_gram_bytes(0, actual, stream_words));
			test_eq_memory("packed BSCONF data preserves every bus byte", expected, actual, stream_words);
		}
	}

	test_eq_u32("direct polling does not use TPS_CTRL", 0, DIF_TPS_CTRL);
	test_eq_u32("direct writes drain TX FIFO", 0, DIF_TXFFS_STAT);
	test_eq_u32("direct writes have no FIFO errors", 0, DIF_ERRIRQSS &
		(DIF_ERRIRQSS_TXFOFL | DIF_ERRIRQSS_TXUFL));

	test_category("Command reads with BSCONF");
	uint8_t reference_id[2] = { 0 };
	test_check("controller reset before command reads", lcd_reset_and_init_controller());
	lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
	test_check("reference command read completes", dif_read_bytes(false, reference_id, sizeof(reference_id)));
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_BSCONF_PROFILES); i++) {
		const struct lcd_bsconf_profile *profile = &LCD_BSCONF_PROFILES[i];
		uint8_t id[2] = { 0 };

		printf("# command read BSCONF %s\n", profile->name);
		test_check("controller reset before command BSCONF read", lcd_reset_and_init_controller());
		lcd_command_bsconf = profile->value;
		test_check("command BSCONF read completes", dif_read_bytes(false, id, sizeof(id)));
		test_eq_memory("command BSCONF read matches reference", reference_id, id, sizeof(id));
	}
	static const struct conversion_profile {
		const char *name;
		enum dif_conversion conversion;
	} CONVERSION_PROFILES[] = {
		{ "BMREG", DIF_CONVERSION_BM },
		{ "BCREG", DIF_CONVERSION_BC },
		{ "PBCCON", DIF_CONVERSION_PBC },
		{ "INVERT_BIT", DIF_CONVERSION_INVERT },
	};
	test_category("CD=1 command writes with data conversion enabled");
	for (uint32_t i = 0; i < ARRAY_SIZE(CONVERSION_PROFILES); i++) {
		const struct conversion_profile *profile = &CONVERSION_PROFILES[i];
		uint16_t pixel = 0;

		printf("# command write with %s\n", profile->name);
		test_check("converted CD=1 GRAM command completes",
			probe_lcd_command_conversion(profile->conversion, 20, &pixel));
		test_eq_u32("converted CD=1 command selects GRAM", 0x1234, pixel);
	}
	test_category("CD=1 command reads with data conversion enabled");
	for (uint32_t i = 0; i < ARRAY_SIZE(CONVERSION_PROFILES); i++) {
		const struct conversion_profile *profile = &CONVERSION_PROFILES[i];
		uint8_t control[sizeof(reference_id)] = { 0 };
		uint8_t converted[sizeof(reference_id)] = { 0 };

		printf("# command read with %s\n", profile->name);
		test_check("controller reset before CD=1 control read", lcd_reset_and_init_controller());
		test_check("CD=1 control read completes", dif_read_bytes(false, control, sizeof(control)));
		test_check("controller reset before converted CD=1 read", lcd_reset_and_init_controller());
		test_check("converted CD=1 read completes",
			dif_read_bytes_with_conversion(profile->conversion, false, converted, sizeof(converted)));
		test_eq_memory("CD=1 read bypasses data conversion", control, converted, sizeof(converted));
	}

	test_category("GRAM reads with BSCONF");
	for (uint32_t i = 0; i < ARRAY_SIZE(LCD_BSCONF_PROFILES); i++) {
		const struct lcd_bsconf_profile *profile = &LCD_BSCONF_PROFILES[i];
		uint8_t gram[16] = { 0 };

		test_check("controller reset before GRAM BSCONF", lcd_reset_and_init_controller());
		lcd_command_bsconf = DIF_CSREG_BSCONF_1x8BIT;
		test_check("GRAM reference stripe write completes",
			lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, 0, 1, profile->color));
		test_check("GRAM BSCONF read completes", lcd_read_gram(profile->value, 0, gram, sizeof(gram)));
		int32_t offset = find_pixel(gram, sizeof(gram), profile->color);
		printf("# GRAM BSCONF %s: pixel offset=%d\n", profile->name, (int) offset);
		test_check("GRAM BSCONF read contains expected RGB565 pixel", offset >= 0);
	}
	test_eq_u32("reads have no FIFO errors", 0, DIF_ERRIRQSS &
		(DIF_ERRIRQSS_RXFUFL | DIF_ERRIRQSS_RXFOFL));

	test_category("CD=0 GRAM reads with data conversion enabled");
	const uint16_t RX_TEST_Y = 21;
	const uint16_t RX_TEST_COLOR = 0xF800;
	for (uint32_t i = 0; i < ARRAY_SIZE(CONVERSION_PROFILES); i++) {
		const struct conversion_profile *profile = &CONVERSION_PROFILES[i];
		uint8_t control[8] = { 0 };
		uint8_t converted[sizeof(control)] = { 0 };

		printf("# GRAM read with %s\n", profile->name);
		test_check("controller reset before CD=0 control read", lcd_reset_and_init_controller());
		test_check("CD=0 control stripe write completes",
			lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, RX_TEST_Y, 1, RX_TEST_COLOR));
		test_check("CD=0 control read prepares GRAM", lcd_prepare_gram(RX_TEST_Y));
		test_check("CD=0 control read completes", dif_read_bytes(true, control, sizeof(control)));
		test_check("controller reset before converted CD=0 read", lcd_reset_and_init_controller());
		test_check("converted CD=0 reference stripe write completes",
			lcd_draw_solid(DIF_CSREG_BSCONF_1x8BIT, RX_TEST_Y, 1, RX_TEST_COLOR));
		test_check("converted CD=0 read prepares GRAM", lcd_prepare_gram(RX_TEST_Y));
		test_check("converted CD=0 read completes",
			dif_read_bytes_with_conversion(profile->conversion, true, converted, sizeof(converted)));
		test_eq_memory("CD=0 read bypasses data conversion", control, converted, sizeof(converted));
	}

	test_category("CD=0 data conversion with direct polling BSCONF");
	static const struct write_conversion_profile {
		const char *name;
		uint16_t input;
		uint16_t expected;
	} WRITE_CONVERSION_PROFILES[] = {
		{ "BMREG", 0xF801, 0xF802 },
		{ "BCREG", 0xF800, 0xF901 },
		{ "INVERT_BIT zero-to-one", 0xF800, 0xF901 },
		{ "INVERT_BIT one-to-zero", 0xF901, 0xF800 },
	};
	for (uint32_t bs = 0; bs < ARRAY_SIZE(LCD_DIRECT_PROFILES); bs++) {
		const struct lcd_bsconf_profile *bsconf = &LCD_DIRECT_PROFILES[bs];

		for (uint32_t conversion = 0; conversion < ARRAY_SIZE(WRITE_CONVERSION_PROFILES); conversion++) {
			const struct write_conversion_profile *profile = &WRITE_CONVERSION_PROFILES[conversion];
			uint16_t pixel = 0;

			printf("# conversion %s with BSCONF %s\n", profile->name, bsconf->name);
			test_check("controller reset before converted write", lcd_reset_and_init_controller());
			test_check("converted write prepares GRAM", lcd_prepare_gram(0));
			DIF_RUNCTRL = 0;
			if (conversion == 0) {
				set_bmreg_mapping(0, 1);
				set_bmreg_mapping(1, 0);
			} else if (conversion == 1) {
				DIF_BCSEL0 = 1;
				DIF_BCREG = 1;
			} else {
				DIF_INVERT_BIT = 1;
			}
			DIF_RUNCTRL = DIF_RUNCTRL_RUN;
			test_check("converted pixel write completes", lcd_write_pixels(bsconf->value, 1, profile->input));
			DIF_RUNCTRL = 0;
			reset_data_conversion();
			DIF_RUNCTRL = DIF_RUNCTRL_RUN;
			test_check("converted pixel read completes", lcd_read_pixel_1x8(0, &pixel));
			test_eq_u32("converted RGB565 pixel matches", profile->expected, pixel);
		}
	}
	uint16_t pbc_pixel = 0;
	test_check("CD=0 PBCCON pixel write completes", probe_pbc_lcd_data(22, 0x07E0, &pbc_pixel));
	test_eq_u32("CD=0 PBCCON consumes the raw pixel packet", 0x001F, pbc_pixel);

	return test_finish();
}
