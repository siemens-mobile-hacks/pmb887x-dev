#include <pmb887x.h>
#include <pmic/PASIC.h>

#include "lcd-board.h"

#if !defined(BOARD_SIEMENS_CX75)
#error No DIFv1 LCD power implementation for this board
#endif

#define LCD_BACKLIGHT_LEVEL 0x50
#define CX75_SUPPLY_ENABLE_2_INITIAL 0x09

bool lcd_board_initialize_light(void) {
	i2c_init();
	if (!i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2, CX75_SUPPLY_ENABLE_2_INITIAL))
		return false;
	if (!i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_ENABLE,
		PASIC_LIGHT_ENABLE_LED2_EN | PASIC_LIGHT_ENABLE_LED3_EN
	))
		return false;

	return i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, 0);
}

bool lcd_board_enable_vboost(void) {
	return i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_SUPPLY_ENABLE_2,
		CX75_SUPPLY_ENABLE_2_INITIAL | PASIC_SUPPLY_ENABLE_2_VBOOST_EN
	);
}

bool lcd_board_enable_backlight(void) {
	if (!i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, LCD_BACKLIGHT_LEVEL))
		return false;

	return i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_CONTROL,
		PASIC_LIGHT_CONTROL_LED2_EN | PASIC_LIGHT_CONTROL_PWM1_EN | PASIC_LIGHT_CONTROL_MASTER_EN
	);
}
