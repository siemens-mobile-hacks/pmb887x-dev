#include <pmb887x.h>
#include <pmic/PASIC.h>

#include "lcd-board.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_CX75)
#error No DIFv1 LCD power implementation for this board
#endif

void lcd_board_initialize_pmic(void) {
	static const struct pmic_register_value {
		uint8_t reg;
		uint8_t value;
	} INITIAL_VALUES[] = {
		{ PASIC_IRQ_MASK_1, 0x0A },
		{ PASIC_IRQ_MASK_2, 0x40 },
		{ PASIC_SUPPLY_CONTROL_1, 0xB7 },
		{ PASIC_RF_VOLTAGE, 0x11 },
		{ PASIC_SUPPLY_ENABLE_1, 0x2F },
		{ PASIC_SUPPLY_ENABLE_2, 0x09 },
		{ PASIC_RF_ENABLE, 0x00 },
		{ PASIC_LIGHT_ENABLE, PASIC_LIGHT_ENABLE_LED2_EN | PASIC_LIGHT_ENABLE_LED3_EN },
		{ 0x0D, 0x00 },
		{ PASIC_POWER, 0x10 },
		{ PASIC_CHARGE_CONTROL, 0x20 },
		{ PASIC_LIGHT_PWM1, 0x00 },
		{ PASIC_LIGHT_PWM2, 0x00 },
		{ PASIC_LIGHT_CONTROL, 0x00 },
	};

	i2c_init();
	test_id_u32(
		"D1094EC identification",
		0x94,
		i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_IDENTIFICATION)
	);
	for (uint32_t i = 0; i < ARRAY_SIZE(INITIAL_VALUES); i++)
		i2c_smbus_write_byte(PASIC_I2C_ADDR, INITIAL_VALUES[i].reg, INITIAL_VALUES[i].value);
	test_check("stock CX75 PMIC configuration sent", true);
}

void lcd_board_enable_panel_power(void) {
	uint8_t charge_control = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_CHARGE_CONTROL);

	/* The CX75 firmware enables charging immediately before LCD initialization. */
	charge_control |= PASIC_CHARGE_CONTROL_CHARGE_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_CHARGE_CONTROL, charge_control);
	test_eq_u32(
		"PMIC applies stock CX75 charger state",
		charge_control,
		i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_CHARGE_CONTROL)
	);
}

void lcd_board_enable_vboost(void) {
	uint8_t supply_enable = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2);

	/* CX75 illumination enables VBOOST through D1094EC supply 0x0E (register 0x07 bit 2). */
	supply_enable |= PASIC_SUPPLY_ENABLE_2_VBOOST_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2, supply_enable);
	test_eq_u32(
		"PMIC enables CX75 LCD VBOOST",
		supply_enable,
		i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2)
	);
}

void lcd_board_enable_backlight(void) {
	uint8_t control = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL);

	control &= ~(PASIC_LIGHT_CONTROL_PWM1_EN |
		PASIC_LIGHT_CONTROL_PWM2_EN | PASIC_LIGHT_CONTROL_MASTER_EN);
	control |= PASIC_LIGHT_CONTROL_LED2_EN |
		PASIC_LIGHT_CONTROL_PWM1_EN | PASIC_LIGHT_CONTROL_MASTER_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, 0x50);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, control);
	test_eq_u32(
		"PMIC enables CX75 display LIGHT",
		control,
		i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL)
	);
}
