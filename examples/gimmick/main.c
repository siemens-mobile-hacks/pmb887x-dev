#include "gen/board_siemens_cx75.h"
#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

static void dif_write(uint16_t data) {
	DIF_TB = data;
	while ((DIF_CON & DIF_CON_BSY) != 0)
		wdt_serve();
}

static uint16_t dif_read(uint16_t data) {
	DIF_TB = data;
	while ((DIF_CON & DIF_CON_BSY) != 0)
		wdt_serve();
	return DIF_RB;
}

/*

// CMD: RS=0, CS=0
// ARGx: RS=1, CS=0

TYPE	CMD		ARG0		ARG1
WRITE	0x0000	<REG_ID>	<VALUE>		write reg value
WRITE	0x4000	<REG_ID>	0x0000		select reg for read
READ	0x8000							read current reg

WRITE	0x3xx0							xx - LCD command (bypass)
WRITE	0x1xx0							xx - LCD data (bypass)

*/

static uint16_t gimmick_write_reg(uint16_t reg, uint16_t value) {
	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, false);
	dif_write(0x0000);

	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, true);
	dif_write(reg);

	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, true);
	dif_write(value);

	gpio_set(GPIO_CIF_CS, true);
	gpio_set(GPIO_CIF_RS, true);

	return value;
}

static uint16_t gimmick_read_reg(uint16_t reg) {
	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, false);
	dif_write(0x4000);

	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, true);
	dif_write(reg);

	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, true);
	dif_write(0);

	gpio_set(GPIO_CIF_CS, false);
	gpio_set(GPIO_CIF_RS, false);
	uint16_t value = dif_read(0x8000);

	gpio_set(GPIO_CIF_CS, true);
	gpio_set(GPIO_CIF_RS, true);

	return value;
}

static void sacc_write(uint16_t msg_id, uint16_t len, uint8_t *data) {
	gpio_set(GPIO_AAC_CS, false);
	dif_read(0x0000);
	dif_read(0x00AA);
	dif_read(len + 4);
	dif_read(msg_id);

	uint16_t *data16 = (uint16_t *) data;
	for (int i = 0; i < (len / 2 * 2); i += 2) {
		dif_read(data16[i / 2]);
	}

	if ((len % 2) != 0) {
		dif_read(data[len - 1]);
	}

	for (int i = 0; i < 100; i++) {
		printf("%04X\n", dif_read(0));
	}

	gpio_set(GPIO_AAC_CS, true);
}

int main(void) {
	i2c_init();
	wdt_init();
	stopwatch_init();

	PLL_CON2 &= ~PLL_CON2_CLK32_EN;

	// ACC codec
	GPIO_PIN(GPIO_AAC_INT) = GPIO_PS_MANUAL | GPIO_DIR_IN;
	GPIO_PIN(GPIO_AAC_CS) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_AAC_RESET) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_AAC_CLK32) = GPIO_PS_ALT | GPIO_OS_ALT3 | GPIO_DATA_HIGH;

	// gimmick
	GPIO_PIN(GPIO_DISPLAY_RESET) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_CIF_CS_DISPLAY) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_CIF_RESET) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_CIF_RS) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_LOW;
	GPIO_PIN(GPIO_DISP_CS1) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_CIF_CS) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;

	GPIO_PIN(GPIO_CIF_MRST) = GPIO_PS_ALT | GPIO_IS_ALT3;
	GPIO_PIN(GPIO_CIF_CLK) = GPIO_PS_ALT | GPIO_OS_ALT0;
	GPIO_PIN(GPIO_CIF_MTSR) = GPIO_PS_ALT | GPIO_OS_ALT0;

	DIF_CLC = 0x100;
	DIF_BR = 0;
	DIF_CON = DIF_CON_MS_MASTER | DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_BM_16 | DIF_CON_EN;

	stopwatch_msleep(1);
	gpio_set(GPIO_CIF_RESET, true);
	gpio_set(GPIO_AAC_RESET, true);
	stopwatch_msleep(1);

	PLL_CON2 |= PLL_CON2_CLK32_EN;

	printf("REG: %04X = %04X\n", 0x0000, gimmick_read_reg(0x0000));
	printf("REG: %04X = %04X\n", 0x0014, gimmick_read_reg(0x0014));
	printf("REG: %04X = %04X\n", 0x0202, gimmick_read_reg(0x0202));

	DIF_CON &= ~DIF_CON_EN;
	while ((DIF_CON & DIF_CON_EN) != 0);

	DIF_CON = DIF_CON_MS_MASTER | DIF_CON_HB_MSB | DIF_CON_BM_16 | DIF_CON_EN;

	uint8_t bootcode[2048] = {};
	sacc_write(0x80, 2044, bootcode);

	while (true) {
		if (gpio_get(GPIO_AAC_INT)) {
			printf("AAC_INT: %d\n", gpio_get(GPIO_AAC_INT));
		}
	}

	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}

__IRQ void irq_handler(void) {
	printf("irq_handler\n");
	while (true);
}
