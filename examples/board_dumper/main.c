#include <pmb887x.h>
#include <printf.h>

#if defined(BOARD_HAS_PMIC_PMB6812)
#include <pmic/PMB6812.h>
#define PMIC_I2C_ADDR PMB6812_I2C_ADDR
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/PASIC.h>
#define PMIC_I2C_ADDR PASIC_I2C_ADDR
#else
#error "unsupported PMIC for this board"
#endif

static void dump_cpu() {
	printf("SCU_UID0=%08X\n", SCU_UID0);
	printf("SCU_UID1=%08X\n", SCU_UID1);
	printf("SCU_UID2=%08X\n", SCU_UID2);
	printf("SCU_MANID=%08X\n", SCU_MANID);
	printf("SCU_CHIPID=%08X\n", SCU_CHIPID);
	printf("---------------------\n");
}

static void dump_brom() {
	printf("BROM: ");
	for (int i = 0; i < 32 * 1024; ++i) {
		printf("%02X", MMIO8(0x00400000 + i));
		wdt_serve();
	}
	printf("\n");
}

static void dump_pmic(void) {
	i2c_init();
	printf("PMIC:");
	for (int i = 0; i <= 0xFF; ++i) {
		uint8_t v = 0;
		i2c_smbus_read_byte(PMIC_I2C_ADDR, i, &v);
		printf(" 0x%02X,", v);
		wdt_serve();
	}
	printf("\n");
	printf("---------------------\n");
}

static void dump_hwid(void) {
	gpio_init_input(GPIO_PIPESTAT0, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_PIPESTAT1, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_PIPESTAT2, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_TRACEPKT0, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_TRACEPKT1, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_TRACEPKT2, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_TRACEPKT3, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);

	printf("GPIO_PIPESTAT0=%d (HW_DET_MOB_TYPE1)\n", gpio_get(GPIO_PIPESTAT0));
	printf("GPIO_PIPESTAT1=%d (HW_DET_MOB_TYPE2)\n", gpio_get(GPIO_PIPESTAT1));
	printf("GPIO_PIPESTAT2=%d (HW_DET_MOB_TYPE3)\n", gpio_get(GPIO_PIPESTAT2));
	printf("GPIO_TRACEPKT0=%d (HW_DET_MOB_TYPE4)\n", gpio_get(GPIO_TRACEPKT0));
	printf("GPIO_TRACEPKT1=%d (HW_DET_MOB_TYPE5 / HW_DET_BLUETOOTH)\n", gpio_get(GPIO_TRACEPKT1));
	printf("GPIO_TRACEPKT2=%d (HW_DET_RF_TYPE)\n", gpio_get(GPIO_TRACEPKT2));
	printf("GPIO_TRACEPKT3=%d (HW_DET_BAND_SEL)\n", gpio_get(GPIO_TRACEPKT3));
	printf("---------------------\n");
}

int main(void) {
	wdt_init();
	printf("---------------------\n");
	dump_hwid();
	dump_pmic();
	dump_cpu();
	if (SCU_CHIPID != 0x00001B10 && SCU_CHIPID != 0x00001B11 && SCU_CHIPID != 0x00001A05)
		dump_brom();
	while (1);
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
