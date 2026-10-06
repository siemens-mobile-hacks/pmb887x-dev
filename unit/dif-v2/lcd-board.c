#include <pmb887x.h>

#include "lcd-board.h"

#if defined(BOARD_SIEMENS_E71) || defined(BOARD_SIEMENS_EL71)
#include <pmic/PASIC.h>

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
	uint8_t light_control = 0;

	i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, &light_control);
	light_control |= PASIC_LIGHT_CONTROL_LED2_EN |
		PASIC_LIGHT_CONTROL_PWM1_EN | PASIC_LIGHT_CONTROL_MASTER_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, LCD_BACKLIGHT_LEVEL);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, light_control);
}

#elif defined(BOARD_LG_KE970)

void lcd_board_initialize_light(void) {
	GPIO_PIN(GPIO_LCD_BACKLIGHT) = GPIO_OS_ALT2 | GPIO_DATA_LOW | GPIO_PS_ALT | GPIO_DIR_IN | GPIO_PPEN_PUSHPULL | GPIO_PDPU_NONE | GPIO_ENAQ_OFF;

	CAPCOM_CLC(CAPCOM1) = 0x0100;
}

void lcd_board_enable_backlight(void) {
	CAPCOM_OUT(CAPCOM1) |= CAPCOM_OUT_O3;

	for (int i = 0; i < 20; i++) { // brightness is 0..31 - 0 = max, 31 = off, wrapping around
		stopwatch_usleep_wd(100);
		CAPCOM_OUT(CAPCOM1) &= ~(CAPCOM_OUT_O3);
		stopwatch_usleep_wd(100);
		CAPCOM_OUT(CAPCOM1) |= CAPCOM_OUT_O3;
	}
}

#else
#error No LCD power implementation for this board
#endif
