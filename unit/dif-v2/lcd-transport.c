#include <pmb887x.h>

#include "lcd/lcd-transport.h"
#include "test.h"

#define DIF_TIMEOUT_MS 100
#define LCD_WRITE_TIM1 0x00000400U
#define LCD_WRITE_TIM2 0x02000400U
#define LCD_READ_TIM1 0x00030F00U
#define LCD_READ_TIM2 0x05020601U

static bool dif_wait_idle(void) {
	stopwatch_t started = stopwatch_get();

	while ((DIF_STAT & DIF_STAT_BSY) != 0 && stopwatch_elapsed_ms(started) < DIF_TIMEOUT_MS)
		test_watchdog_serve();

	return (DIF_STAT & DIF_STAT_BSY) == 0;
}

static bool lcd_transport_set_timings(uint32_t lcdtim1, uint32_t lcdtim2) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_LCDTIM1 = lcdtim1;
	DIF_LCDTIM2 = lcdtim2;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;

	return true;
}

static bool lcd_transport_write_byte(bool command, uint8_t value) {
	if (!dif_wait_idle())
		return false;
	DIF_RUNCTRL = 0;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_BSCONF_1x8BIT | (command ? DIF_CSREG_CD : 0);
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_TXD = value;

	return dif_wait_idle();
}

void lcd_transport_init(void) {
	GPIO_CLC = 1U << MOD_CLC_RMC_SHIFT;
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

	DIF_RUNCTRL = 0;
	DIF_CON = 0;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_PARALLEL;
	DIF_TXFIFO_CFG = DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC;
	DIF_RXFIFO_CFG = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	DIF_LCDTIM1 = LCD_WRITE_TIM1;
	DIF_LCDTIM2 = LCD_WRITE_TIM2;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_CD | DIF_CSREG_BSCONF_1x8BIT;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

void lcd_transport_reset_controller(void) {
	gpio_set(GPIO_DIF_RESET1, false);
	stopwatch_usleep_wd(10000);
	gpio_set(GPIO_DIF_RESET1, true);
	stopwatch_usleep_wd(10000);
}

bool lcd_transport_sync_parallel_interface(void) {
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

	uint8_t zeros[4] = { 0 };
	bool success = lcd_transport_write_data(zeros, sizeof(zeros));
	gpio_set(GPIO_DIF_CD, true);
	gpio_set(GPIO_DIF_RD, false);
	GPIO_PIN(GPIO_DIF_CD) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_DIF_RD) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT;

	return success;
}

bool lcd_transport_write_command(uint8_t command) {
	return lcd_transport_write_byte(true, command);
}

bool lcd_transport_write_data(const uint8_t *data, uint32_t size) {
	bool success = true;

	for (uint32_t i = 0; i < size; i++)
		success &= lcd_transport_write_byte(false, data[i]);

	return success;
}

bool lcd_transport_read_data(uint8_t *data, uint32_t size) {
	if (size == 0)
		return true;

	uint32_t previous_lcdtim1 = DIF_LCDTIM1;
	uint32_t previous_lcdtim2 = DIF_LCDTIM2;

	if (!lcd_transport_set_timings(LCD_READ_TIM1, LCD_READ_TIM2))
		return false;
	DIF_RUNCTRL = 0;
	DIF_RXFIFO_CFG = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	DIF_CSREG = DIF_CSREG_CS1 | DIF_CSREG_BSCONF_1x8BIT;
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
		data[i] = DIF_RXD;
	}

	bool idle = dif_wait_idle();
	DIF_STARTLCDRD = 0;

	return idle && lcd_transport_set_timings(previous_lcdtim1, previous_lcdtim2);
}
