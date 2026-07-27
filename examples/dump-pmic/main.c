#include <pmb887x.h>
#include <pmic/D1094XX.h>
#include <printf.h>

static const char *pmic_model_name(uint32_t model, uint32_t revision) {
	if (model == 4) {
		switch (revision) {
		case 2:
			return "D1094EC";
		case 3:
			return "D1094BB";
		case 15:
			return "D1094DB";
		default:
			return "D1094xx";
		}
	}
	if (model == 5)
		return "D1094ED";
	if (model == 6)
		return "D1601AA";
	return "unknown";
}

static uint32_t pmic_family(uint32_t model, bool is_dialog) {
	if (model < 4 || model > 6)
		return 15;
	return (model - 4) * 2 + !is_dialog;
}

static void print_pmic_info(void) {
	uint32_t id = i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_IDENTIFICATION);
	uint32_t model = (id & D1094XX_IDENTIFICATION_MODEL) >> D1094XX_IDENTIFICATION_MODEL_SHIFT;
	uint32_t revision = (id & D1094XX_IDENTIFICATION_REVISION) >> D1094XX_IDENTIFICATION_REVISION_SHIFT;
	bool is_dialog = (id & D1094XX_IDENTIFICATION_VENDOR) == D1094XX_IDENTIFICATION_VENDOR_DIALOG;

	printf(
		"PMIC 0x%02X: ID=0x%02X, model=%s, vendor=%s, family=%u, revision=%u\n",
		D1094XX_I2C_ADDR,
		id,
		pmic_model_name(model, revision),
		is_dialog ? "Dialog Mozart" : "ST Twigo4",
		pmic_family(model, is_dialog),
		revision
	);
}

static void dump_all_regs(void) {
	print_pmic_info();
	printf("PMIC registers on 0x31\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(D1094XX_I2C_ADDR, i);
		printf("0x%02X, ", v);
		wdt_serve();
	}
	printf("\ndone!\n");
	
	printf("Dialog on 0x03\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(0x03, i);
		printf("0x%02X, ", v);
		wdt_serve();
	}
	printf("\ndone!\n");

	printf("Dialog on 0x8\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(0x8, i);
		printf("0x%02X, ", v);
		wdt_serve();
	}
	printf("\ndone!\n");
}

int main(void) {
	wdt_init();
	i2c_init();
	
	gpio_init_output(GPIO_LED_FL_EN, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_PUSHPULL, GPIO_PDPU_NONE, false);
	
	dump_all_regs();
	
	return 0;
}
