#include <pmb887x.h>
#include <pmic/PASIC.h>

#include "lcd-board.h"

#if !defined(BOARD_SIEMENS_CX75)
#error No DIFv1 LCD power implementation for this board
#endif

#define LCD_BACKLIGHT_LEVEL 0x50

void lcd_board_initialize_light(void) {
	i2c_init();
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_ENABLE,
		PASIC_LIGHT_ENABLE_LED2_EN | PASIC_LIGHT_ENABLE_LED3_EN
	);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, 0);
}

void lcd_board_enable_vboost(void) {
	uint8_t supply_enable = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2);

	supply_enable |= PASIC_SUPPLY_ENABLE_2_VBOOST_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2, supply_enable);
}

void lcd_board_enable_backlight(void) {
	uint8_t control = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL);

	control |= PASIC_LIGHT_CONTROL_LED2_EN |
		PASIC_LIGHT_CONTROL_PWM1_EN | PASIC_LIGHT_CONTROL_MASTER_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, LCD_BACKLIGHT_LEVEL);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, control);
}
