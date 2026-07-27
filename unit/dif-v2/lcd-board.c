#include <pmb887x.h>
#include <pmic/PASIC.h>

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
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1, 0x0D);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_RF_VOLTAGE, 0x12);
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_ENABLE,
		PASIC_LIGHT_ENABLE_LED1_EN | PASIC_LIGHT_ENABLE_LED2_EN | PASIC_LIGHT_ENABLE_LED3_EN
	);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_2, 0x09);
	stopwatch_usleep_wd(1000);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_RF_VOLTAGE, 0x12);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_RF_ENABLE, PASIC_RF_ENABLE_VRF1_EN);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_RF_VOLTAGE, 0x1A);
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_RF_ENABLE,
		PASIC_RF_ENABLE_VRF1_EN | PASIC_RF_ENABLE_VRF2_EN
	);
	stopwatch_usleep_wd(1000);
}

void lcd_board_enable_backlight(void) {
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_LIGHT_CONTROL,
		PASIC_LIGHT_CONTROL_LED2_EN | PASIC_LIGHT_CONTROL_PWM1_EN
	);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, 0x50);
}
