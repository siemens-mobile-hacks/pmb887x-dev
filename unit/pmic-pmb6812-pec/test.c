#include <pmb887x.h>
#include <pmic/PMB6812.h>

#include "i2c.h"
#include "test.h"

#ifdef BOARD_HAS_PMIC_PMB6812

#ifdef PMB8875

#define I2C_V1_TIMEOUT_MS 100

static bool i2c_v1_status_wait(uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((I2C_SYSCON & mask) == 0 && stopwatch_elapsed_ms(start) < I2C_V1_TIMEOUT_MS)
		test_watchdog_serve();
	return (I2C_SYSCON & mask) != 0;
}

static bool i2c_v1_status_clear_wait(uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((I2C_SYSCON & mask) != 0 && stopwatch_elapsed_ms(start) < I2C_V1_TIMEOUT_MS)
		test_watchdog_serve();
	return (I2C_SYSCON & mask) == 0;
}

static void test_i2c_v1_data_nack(void) {
	const uint8_t value = 1 << PMB6812_LEDCTRL1_LEDPWM_SHIFT;
	const uint32_t bad_command = ((uint32_t) PMB6812_I2C_ADDR << 1) | ((uint32_t) PMB6812_LEDCTRL1 << 8) |
		((uint32_t) value << 16) | ((uint32_t) 0xD1 << 24);
	const uint32_t restore_command = ((uint32_t) PMB6812_I2C_ADDR << 1) |
		((uint32_t) PMB6812_LEDCTRL1 << 8) | ((uint32_t) 0x20 << 24);

	test_category("I2C v1 data NACK");
	I2C_SYSCON = I2C_SYSCON_MOD_MASTER | (3 << I2C_SYSCON_CI_SHIFT);
	I2C_RTB = bad_command;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETBUM;

	test_check("bad PEC raises data interrupt", i2c_v1_status_wait(I2C_SYSCON_IRQD));
	test_check("data-byte NACK is reported in LRB", (I2C_SYSCON & I2C_SYSCON_LRB) != 0);
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQD;
	test_check("data-byte NACK ends the transmission", i2c_v1_status_wait(I2C_SYSCON_IRQE));
	test_eq_u32("NACK end holds the bus until STOP", I2C_SYSCON_BB | I2C_SYSCON_BUM,
		I2C_SYSCON & (I2C_SYSCON_BB | I2C_SYSCON_BUM));
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRBUM;
	test_check("STOP releases the physical bus", i2c_v1_status_clear_wait(I2C_SYSCON_BB));
	(void) I2C_RTB;
	// PMB8875 keeps LRB latched across RTB accesses until another frame is acknowledged.
	test_check("RTB read preserves LRB", (I2C_SYSCON & I2C_SYSCON_LRB) != 0);
	I2C_RTB = restore_command;
	test_check("RTB write preserves LRB", (I2C_SYSCON & I2C_SYSCON_LRB) != 0);
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQE;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETBUM;
	test_check("valid PEC raises data interrupt", i2c_v1_status_wait(I2C_SYSCON_IRQD));
	test_check("ACK clears LRB", (I2C_SYSCON & I2C_SYSCON_LRB) == 0);
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRBUM | I2C_WHBSYSCON_CLRIRQD;
	test_check("valid command STOP releases the bus", i2c_v1_status_clear_wait(I2C_SYSCON_BB));
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQE;
}

#endif

static bool command_write(uint8_t value, uint8_t pec) {
	const uint8_t command[] = { PMB6812_LEDCTRL1, value, pec };

	return i2c_write(PMB6812_I2C_ADDR, command, ARRAY_SIZE(command));
}

int main(void) {
	uint8_t confirmation = 0;
	uint8_t value = 1 << PMB6812_LEDCTRL1_LEDPWM_SHIFT;
	uint8_t public_command[] = { PMB6812_LEDCTRL1, value };

	test_start("PMB6812 PEC");
	i2c_init();

	test_check("command write is acknowledged", command_write(value, 0x2E));
	test_check("command confirmation read completes", i2c_read(PMB6812_I2C_ADDR, &confirmation, 1));
	test_eq_u32("accepted command returns its PEC", 0x2E, confirmation);
	test_check("restore write is acknowledged", command_write(0, 0x20));
	test_check("restore confirmation read completes", i2c_read(PMB6812_I2C_ADDR, &confirmation, 1));
	test_eq_u32("restore command returns its PEC", 0x20, confirmation);

	test_check("command with bad PEC is rejected", !command_write(value, 0xD1));
	test_check("rejected command confirmation read completes", i2c_read(PMB6812_I2C_ADDR, &confirmation, 1));
	test_eq_u32("rejected command returns inverted PEC", 0x2E, confirmation);
	test_check("public PEC write is acknowledged",
		i2c_smbus_write_pec(PMB6812_I2C_ADDR, public_command, ARRAY_SIZE(public_command)));
	test_check("public PEC confirmation read completes", i2c_read(PMB6812_I2C_ADDR, &confirmation, 1));
	test_eq_u32("public PEC write calculates the expected CRC", 0x2E, confirmation);
	public_command[1] = 0;
	test_check("public confirmed PEC write verifies confirmation",
		i2c_smbus_write_pec_confirmed(PMB6812_I2C_ADDR, public_command, ARRAY_SIZE(public_command)));

#ifdef PMB8875
	test_i2c_v1_data_nack();
#endif

	return test_finish();
}

#else

int main(void) {
	test_start("PMB6812 PEC");
	test_skip("PMB6812 PEC", "unsupported PMIC");
	return test_finish();
}

#endif
