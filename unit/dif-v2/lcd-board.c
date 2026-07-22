#include <d1601aa.h>
#include <pmb887x.h>

#include "lcd-board.h"

#if !defined(BOARD_SIEMENS_E71) && !defined(BOARD_SIEMENS_EL71)
#error No LCD power implementation for this board
#endif

void lcd_board_enable_panel_power(void) {
	gpio_init_output(
		GPIO_LED_FL_EN,
		GPIO_OS_NONE,
		GPIO_PS_MANUAL,
		false,
		GPIO_PPEN_PUSHPULL,
		GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF
	);
	i2c_init();
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x06, 0x0D);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x12);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0C, 0x07);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x07, 0x09);
	stopwatch_usleep_wd(1000);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x12);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x08, 0x01);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x0B, 0x1A);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x08, 0x03);
	stopwatch_usleep_wd(1000);
}

void lcd_board_enable_backlight(void) {
	i2c_smbus_write_byte(
		D1601AA_I2C_ADDR,
		D1601AA_LED_CONTROL,
		D1601AA_LED2_EN | D1601AA_LIGHT_PWM1_EN
	);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM1, 0x50);
}
