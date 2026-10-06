#include <pmb887x.h>
#include <pmic/PMB6812.h>

#include "i2c.h"
#include "pmb681x.h"

bool pmb681x_reg_write(uint8_t reg, uint8_t value) {
	const uint8_t command[] = { reg, value };
	uint8_t expected = i2c_smbus_pec_calculate(PMB6812_I2C_ADDR, command, ARRAY_SIZE(command));
	uint8_t confirmation;

	if (!i2c_smbus_write_pec(PMB6812_I2C_ADDR, command, ARRAY_SIZE(command)))
		return false;
	return i2c_read(PMB6812_I2C_ADDR, &confirmation, 1) && confirmation == expected;
}
