#include <pmb887x.h>

#include "lcd-board.h"
#include "test.h"

#if !defined(BOARD_SIEMENS_CX75)
#error No DIFv1 LCD power implementation for this board
#endif

#define D1094EC_LCD_POWER_REG 0x10
#define D1094EC_SUPPLY_ENABLE_REG 0x07
#define D1094EC_LIGHT_PWM1_REG 0x12
#define D1094EC_LIGHT_PWM2_REG 0x13
#define D1094EC_LED_CONTROL_REG 0x14
#define D1094EC_LED_CONTROL_LIGHT_MASK 0x38
#define D1094EC_LCD_BACKLIGHT_CONTROL 0x2C

void lcd_board_initialize_pmic(void) {
	static const struct pmic_register_value {
		uint8_t reg;
		uint8_t value;
	} INITIAL_VALUES[] = {
		{ 0x03, 0x0A },
		{ 0x04, 0x40 },
		{ 0x09, 0xB7 },
		{ 0x0B, 0x11 },
		{ 0x06, 0x2F },
		{ 0x07, 0x09 },
		{ 0x08, 0x00 },
		{ 0x0C, 0x06 },
		{ 0x0D, 0x00 },
		{ 0x0E, 0x10 },
		{ D1094EC_LCD_POWER_REG, 0x20 },
		{ D1094EC_LIGHT_PWM1_REG, 0x00 },
		{ D1094EC_LIGHT_PWM2_REG, 0x00 },
		{ D1094EC_LED_CONTROL_REG, 0x00 },
	};

	i2c_init();
	test_id_u32("D1094EC identification", 0x94, i2c_smbus_read_byte(0x31, 0x00));
	for (uint32_t i = 0; i < ARRAY_SIZE(INITIAL_VALUES); i++)
		i2c_smbus_write_byte(0x31, INITIAL_VALUES[i].reg, INITIAL_VALUES[i].value);
	test_check("stock CX75 PMIC configuration sent", true);
}

void lcd_board_enable_panel_power(void) {
	uint8_t power_control = i2c_smbus_read_byte(0x31, D1094EC_LCD_POWER_REG);

	/* The CX75 firmware changes D1094EC register 0x10 from 0x20 to 0x28 before LCD initialization. */
	power_control |= BIT(3);
	i2c_smbus_write_byte(0x31, D1094EC_LCD_POWER_REG, power_control);
	test_eq_u32(
		"PMIC enables CX75 LCD power",
		power_control,
		i2c_smbus_read_byte(0x31, D1094EC_LCD_POWER_REG)
	);
}

void lcd_board_enable_vboost(void) {
	uint8_t supply_enable = i2c_smbus_read_byte(0x31, D1094EC_SUPPLY_ENABLE_REG);

	/* CX75 illumination enables VBOOST through D1094EC supply 0x0E (register 0x07 bit 2). */
	supply_enable |= BIT(2);
	i2c_smbus_write_byte(0x31, D1094EC_SUPPLY_ENABLE_REG, supply_enable);
	test_eq_u32(
		"PMIC enables CX75 LCD VBOOST",
		supply_enable,
		i2c_smbus_read_byte(0x31, D1094EC_SUPPLY_ENABLE_REG)
	);
}

void lcd_board_enable_backlight(void) {
	uint8_t control = i2c_smbus_read_byte(0x31, D1094EC_LED_CONTROL_REG);

	control &= ~D1094EC_LED_CONTROL_LIGHT_MASK;
	control |= D1094EC_LCD_BACKLIGHT_CONTROL;
	i2c_smbus_write_byte(0x31, D1094EC_LIGHT_PWM1_REG, 0x50);
	i2c_smbus_write_byte(0x31, D1094EC_LED_CONTROL_REG, control);
	test_eq_u32(
		"PMIC enables CX75 display LIGHT",
		control,
		i2c_smbus_read_byte(0x31, D1094EC_LED_CONTROL_REG)
	);
}
