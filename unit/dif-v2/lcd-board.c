#include <pmb887x.h>
#include <pmic/PASIC.h>

#include "lcd-board.h"

#if !defined(BOARD_SIEMENS_E71) && !defined(BOARD_SIEMENS_EL71)
#error No LCD power implementation for this board
#endif

#define LCD_BACKLIGHT_LEVEL 0x50

void lcd_board_initialize_light(void) {
	i2c_init();
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_ENABLE,
		PASIC_LIGHT_ENABLE_LED1_EN | PASIC_LIGHT_ENABLE_LED2_EN | PASIC_LIGHT_ENABLE_LED3_EN
	);
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_CONTROL,
		PASIC_LIGHT_CONTROL_LED1_EN | PASIC_LIGHT_CONTROL_PWM1_EN |
			PASIC_LIGHT_CONTROL_PWM2_EN | PASIC_LIGHT_CONTROL_MASTER_EN
	);
}

void lcd_board_enable_backlight(void) {
	uint8_t light_control = i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL);

	light_control |= PASIC_LIGHT_CONTROL_LED2_EN |
		PASIC_LIGHT_CONTROL_PWM1_EN | PASIC_LIGHT_CONTROL_MASTER_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, LCD_BACKLIGHT_LEVEL);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, light_control);
}
