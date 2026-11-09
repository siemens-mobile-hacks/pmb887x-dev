#include <pmb887x.h>
#include <pmic/D1094XX.h>

#include "i2c.h"
#include "test.h"

#ifdef BOARD_PLATFORM_SIEMENS

int main(void) {
	uint8_t identification = 0;
	uint8_t original = 0;
	uint8_t written;
	uint8_t readback = 0;

	test_start("D1094xx PMIC");
	i2c_init();

	test_check("identification read completes",
		i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_IDENTIFICATION, &identification));
	uint8_t model = (identification & D1094XX_IDENTIFICATION_MODEL) >> D1094XX_IDENTIFICATION_MODEL_SHIFT;
	uint8_t revision = (identification & D1094XX_IDENTIFICATION_REVISION) >> D1094XX_IDENTIFICATION_REVISION_SHIFT;
	const char *vendor = (identification & D1094XX_IDENTIFICATION_VENDOR) != 0 ? "Dialog" : "ST";
	printf("# identification=%02X vendor=%s model=%u revision=%u\n", identification, vendor, model, revision);
	test_check("identification resembles the D1094xx family", model >= 4 && model <= 6);

	test_check("LIGHT_PWM1 read completes",
		i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_LIGHT_PWM1, &original));
	written = original == 0 ? 1 : 0;
	test_check("LIGHT_PWM1 write completes",
		i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_LIGHT_PWM1, written));
	test_check("LIGHT_PWM1 readback completes",
		i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_LIGHT_PWM1, &readback));
	test_eq_u32("LIGHT_PWM1 reads back the written value", written, readback);
	test_check("LIGHT_PWM1 restore completes",
		i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_LIGHT_PWM1, original));
	test_check("LIGHT_PWM1 restored readback completes",
		i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_LIGHT_PWM1, &readback));
	test_eq_u32("LIGHT_PWM1 restores its original value", original, readback);

	return test_finish();
}

#else

int main(void) {
	test_start("D1094xx PMIC");
	test_skip("D1094xx PMIC", "unsupported PMIC");
	return test_finish();
}

#endif
