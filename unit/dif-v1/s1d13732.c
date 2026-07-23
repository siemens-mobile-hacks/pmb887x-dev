#include <pmb887x.h>

#include "s1d13732.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_CX75)
#error The S1D13732 transport currently requires BOARD=siemens-cx75
#endif

#define S1D13732_WRITE_REGISTER 0x0000U
#define S1D13732_READ_REGISTER 0x4000U
#define S1D13732_READ_REGISTER_DATA 0x8000U
#define S1D13732_DISPLAY_DATA_BYPASS 0xD000U
#define S1D13732_DISPLAY_COMMAND 0x3000U
#define S1D13732_DISPLAY_DATA 0x1000U

#define S1D13732_DETECT_PATTERN_C0 BIT(0)
#define S1D13732_DETECT_PATTERN_E0 BIT(1)
#define S1D13732_DETECT_PATTERN_C1 BIT(2)

static uint16_t dif_exchange(uint16_t value) {
	DIF_TB = value;
	while ((DIF_CON & DIF_CON_BSY) != 0)
		;

	return DIF_RB;
}

static void dif_write_only(uint16_t value) {
	DIF_TB = value;
	while ((DIF_CON & DIF_CON_BSY) != 0)
		;
}

static void select_bridge(bool selected) {
	gpio_set(GPIO_CIF_CS, !selected);
}

static void select_display(bool selected) {
	gpio_set(GPIO_DISP_CS1, !selected);
}

static void set_bridge_command(bool command) {
	gpio_set(GPIO_CIF_RS, !command);
}

static void s1d13732_write_command(uint16_t command) {
	set_bridge_command(true);
	dif_write_only(command);
}

static uint16_t s1d13732_read_command(uint16_t command) {
	set_bridge_command(true);
	return dif_exchange(command);
}

static void s1d13732_write_data(uint16_t data) {
	set_bridge_command(false);
	dif_write_only(data);
}

static void s1d13732_write_display_frame(uint16_t frame) {
	/* gimmick_display_cmd()/data(): pulse S1D CS, then leave both host controls inactive. */
	select_bridge(false);
	select_bridge(true);
	set_bridge_command(true);
	dif_write_only(frame);
	set_bridge_command(false);
	select_bridge(false);
}

void s1d13732_init(void) {
	GPIO_CLC = 1U << MOD_CLC_RMC_SHIFT;
	DIF_CLC = 1U << MOD_CLC_RMC_SHIFT;
	gpio_init_output(
		GPIO_CIF_RESET,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CIF_RS,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CIF_CS,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_DISP_CS1,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CIF_CS_DISPLAY,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_CLKOUT0,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	gpio_init_output(
		GPIO_DISPLAY_RESET,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);

	GPIO_PIN(GPIO_CIF_MRST) = GPIO_PS_ALT | GPIO_IS_ALT3;
	GPIO_PIN(GPIO_CIF_CLK) = GPIO_PS_ALT | GPIO_OS_ALT0;
	GPIO_PIN(GPIO_CIF_MTSR) = GPIO_PS_ALT | GPIO_OS_ALT0;
	GPIO_PIN(GPIO_CLK32) = GPIO_PS_ALT | GPIO_OS_ALT1;

	DIF_BR = 0;
	/* DIFv1.cfg defines these fields only in programming mode (EN=0). */
	DIF_CON = 0;
	DIF_CON =
		DIF_CON_MS_MASTER |
		DIF_CON_HB_MSB |
		DIF_CON_PH_1 |
		DIF_CON_BM_16;
	DIF_CON |= DIF_CON_EN;

	/* FUN_a0a8af8c(): reset S1D13732 while CLK32 is supplied. */
	PLL_CON2 |= PLL_CON2_CLK32_EN;
	stopwatch_usleep_wd(255);
	gpio_set(GPIO_CIF_RESET, true);
	stopwatch_usleep_wd(15);
	gpio_set(GPIO_CIF_RESET, false);
	stopwatch_usleep_wd(255);
	gpio_set(GPIO_CIF_RESET, true);
	PLL_CON2 &= ~PLL_CON2_CLK32_EN;
}

static bool sample_display_detect_pattern(
	struct s1d13732_display_detect_result *result,
	uint16_t data_bus_pattern,
	uint8_t pattern_bit
) {
	s1d13732_write_register(0x0060, data_bus_pattern);
	stopwatch_usleep_wd(2000);
	result->sampled_patterns |= pattern_bit;
	if (!gpio_get(GPIO_DISP_CS1))
		return false;

	result->high_patterns |= pattern_bit;
	return true;
}

struct s1d13732_display_detect_result s1d13732_detect_display(void) {
	struct s1d13732_display_detect_result result = { 0 };

	/* gimmick_begin_display_detection(): run the S1D PLL from the external 32.768 kHz clock. */
	PLL_CON2 |= PLL_CON2_CLK32_EN;
	s1d13732_write_register(0x0004, 0x00C0);
	s1d13732_write_register(0x0014, 0x1211);
	s1d13732_write_register(0x000C, 0x1027);
	s1d13732_write_register(0x000E, 0x2840);
	s1d13732_write_register(0x0010, 0x0103);
	s1d13732_write_register(0x0018, 0x0000);
	s1d13732_write_register(0x0012, 0x0000);
	stopwatch_usleep_wd(25000);

	uint16_t saved_interface_config = s1d13732_read_register(0x0014);
	s1d13732_write_register(0x0014, 0x0000);
	uint16_t saved_display_detect_config = s1d13732_read_register(0x0202);
	s1d13732_write_register(0x0202, 0x0000);

	/* The CPU releases DISP_CS1 while S1D presents the probe patterns on FPDAT[7:0]. */
	gpio_init_input(GPIO_DISP_CS1, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, GPIO_ENAQ_OFF);
	s1d13732_write_register(0x0060, 0x00C0);
	if (sample_display_detect_pattern(&result, 0x00C0, S1D13732_DETECT_PATTERN_C0)) {
		result.controller_type = LCD_CONTROLLER_SSD1286;
	} else if (sample_display_detect_pattern(&result, 0x00E0, S1D13732_DETECT_PATTERN_E0)) {
		result.controller_type = LCD_CONTROLLER_LS020;
	} else if (sample_display_detect_pattern(&result, 0x00C1, S1D13732_DETECT_PATTERN_C1)) {
		result.controller_type = LCD_CONTROLLER_PCF8882;
	}
	gpio_init_output(
		GPIO_DISP_CS1,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		true,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);

	/* gimmick_end_display_detection(): restore the normal panel path and stop CLK32. */
	s1d13732_write_register(0x003C, s1d13732_read_register(0x003C));
	s1d13732_write_register(0x0060, 0x0000);
	s1d13732_write_register(0x0202, saved_display_detect_config);
	s1d13732_write_register(0x0014, saved_interface_config);
	s1d13732_write_register(0x0012, 0x0101);
	PLL_CON2 &= ~PLL_CON2_CLK32_EN;

	return result;
}

void s1d13732_enable_memory_interface(void) {
	/* gimmick_enable_runtime_blocks() from the CX75 firmware. */
	PLL_CON2 |= PLL_CON2_CLK32_EN;
	s1d13732_write_register(0x0004, 0x00C0);
	s1d13732_write_register(0x0014, 0x1211);
	s1d13732_write_register(0x0014, s1d13732_read_register(0x0014) & ~BIT(12));
	s1d13732_write_register(0x0180, s1d13732_read_register(0x0180) | BIT(11));
	s1d13732_write_register(0x000C, 0x1027);
	s1d13732_write_register(0x000E, 0x1E48);
	s1d13732_write_register(0x0010, 0x0103);
	s1d13732_write_register(0x0018, 0x0000);
	s1d13732_write_register(0x0014, s1d13732_read_register(0x0014) | BIT(0));
	s1d13732_write_register(0x0012, 0x0000);
	/* The close S1D13719 specification allows up to 100 ms for PLL lock. */
	stopwatch_usleep_wd(150000);
	s1d13732_write_register(0x0014, s1d13732_read_register(0x0014) & ~BIT(0));
	stopwatch_usleep_wd(25000);
	s1d13732_write_register(0x0300, 0x0003);
	s1d13732_write_register(0x0110, s1d13732_read_register(0x0110) & ~BIT(0));
	s1d13732_write_register(0x0114, 0x0001);
	s1d13732_write_register(0x0100, 0x0009);
	s1d13732_write_register(0x0110, 0x0409);
}

void s1d13732_select_display(bool selected) {
	select_display(selected);
	gpio_set(GPIO_CLKOUT0, !selected);
}

void s1d13732_write_register(uint16_t reg, uint16_t value) {
	select_bridge(true);
	s1d13732_write_command(S1D13732_WRITE_REGISTER);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(reg);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(value);
	select_bridge(false);
}

uint16_t s1d13732_read_register(uint16_t reg) {
	select_bridge(true);
	s1d13732_write_command(S1D13732_READ_REGISTER);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(reg);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(0);
	select_bridge(false);
	select_bridge(true);
	uint16_t value = s1d13732_read_command(S1D13732_READ_REGISTER_DATA);
	select_bridge(false);

	return value;
}

void s1d13732_write_memory16(uint32_t address, const uint16_t *data, uint32_t count) {
	if (count == 0)
		return;

	s1d13732_begin_memory_write16(address);
	for (uint32_t i = 0; i < count; i++)
		s1d13732_write_memory_word16(data[i]);
	s1d13732_end_memory_write16();
}

void s1d13732_begin_memory_write16(uint32_t address) {
	/* REG[0022h] latches the full address, so set REG[0024h] first. */
	s1d13732_write_register(0x0024, (address >> 16) & 0x0007);
	s1d13732_write_register(0x0022, (uint16_t) address & 0xFFFE);

	select_bridge(true);
	s1d13732_write_command(S1D13732_WRITE_REGISTER);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(0x0028);
	select_bridge(false);
	select_bridge(true);
	set_bridge_command(false);
}

void s1d13732_write_memory_word16(uint16_t data) {
	dif_write_only(data);
}

void s1d13732_end_memory_write16(void) {
	select_bridge(false);
}

void s1d13732_read_memory16(uint32_t address, uint16_t *data, uint32_t count) {
	if (count == 0)
		return;

	/* REG[0022h] latches the full address, so set REG[0024h] first. */
	s1d13732_write_register(0x0024, (address >> 16) & 0x0007);
	s1d13732_write_register(0x0022, (uint16_t) address | 1U);

	select_bridge(true);
	s1d13732_write_command(S1D13732_READ_REGISTER);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(0x0028);
	select_bridge(false);
	select_bridge(true);
	s1d13732_write_data(0x0000);
	select_bridge(false);

	if (count > 1) {
		select_bridge(true);
		set_bridge_command(false);
		for (uint32_t i = 0; i < count - 1; i++)
			data[i] = dif_exchange(0x0000);
		select_bridge(false);
	}

	select_bridge(true);
	data[count - 1] = s1d13732_read_command(S1D13732_READ_REGISTER_DATA);
	select_bridge(false);
}

void s1d13732_reset_display(void) {
	gpio_set(GPIO_DISPLAY_RESET, true);
	stopwatch_usleep_wd(150);
	gpio_set(GPIO_DISPLAY_RESET, false);
	stopwatch_usleep_wd(150);
	gpio_set(GPIO_DISPLAY_RESET, true);
	stopwatch_usleep_wd(5000);
}

void s1d13732_write_display_command(uint8_t command) {
	s1d13732_write_display_frame(S1D13732_DISPLAY_COMMAND | ((uint16_t) command << 4));
}

void s1d13732_write_display_data(uint8_t data) {
	s1d13732_write_display_frame(S1D13732_DISPLAY_DATA | ((uint16_t) data << 4));
}

void s1d13732_begin_display_data_stream(void) {
	/*
	 * D000 switches the S1D13732 from its framed command protocol to the raw
	 * panel data path used by the CX75 firmware for RGB565 framebuffer words.
	 */
	s1d13732_write_display_frame(S1D13732_DISPLAY_DATA_BYPASS);
}

void s1d13732_write_display_word(uint16_t data) {
	dif_write_only(data);
}

void s1d13732_end_display_data_stream(void) {
	/* pcf8882_finish_draw_job() repeats D000 with S1D CS inactive to flush the raw job. */
	set_bridge_command(true);
	dif_write_only(S1D13732_DISPLAY_DATA_BYPASS);
	set_bridge_command(false);
	/* The firmware releases the panel path before releasing the S1D CS. */
	s1d13732_select_display(false);
	select_bridge(false);
}
