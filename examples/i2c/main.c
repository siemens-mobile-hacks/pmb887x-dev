#include <pmb887x.h>
#include <pmic/PASIC.h>
#include <printf.h>

static void dump_all_regs(void) {
	printf("Dump all PASIC registers...\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(PASIC_I2C_ADDR, i);
		printf("%02X: %02X\n", i, v);
		wdt_serve();
	}
}

static void test_vibra(void) {
	printf("Testing i2c vibra...\n");
	
	for (int i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_VIBRA, i);
		stopwatch_msleep_wd(30);
	}
	
	for (int i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_VIBRA, i);
		stopwatch_msleep_wd(30);
	}
}

static void test_pickoff_sound(void) {
	printf("Testing pickoff sound...\n");
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_AMPLIFIER_GAIN_2, 0x24);
	i2c_smbus_write_byte(
		PASIC_I2C_ADDR,
		PASIC_TONE_CONTROL,
		0x1F << PASIC_TONE_CONTROL_DURATION_SHIFT | 3 << PASIC_TONE_CONTROL_MODULATION_SHIFT
	);
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_MONO_CONTROL, 0x5F);
}

static void test_backlight(void) {
	uint32_t lcd_control = 0;
	
	// Даём питалово
	lcd_control |= PASIC_LIGHT_CONTROL_LED2_EN;
	
	// ================= DISPLAY =================
	printf("Testing i2c LCD backlight...\n");
	
	lcd_control |= PASIC_LIGHT_CONTROL_PWM1_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, lcd_control);
	
	for (int i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, i);
		stopwatch_msleep_wd(30);
	}
	
	for (int i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM1, i);
		stopwatch_msleep_wd(30);
	}
	lcd_control &= ~PASIC_LIGHT_CONTROL_PWM1_EN;
	
	// ================= KEYBOARD =================
	printf("Testing i2c keyboard backlight...\n");
	
	lcd_control |= PASIC_LIGHT_CONTROL_PWM2_EN;
	i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_CONTROL, lcd_control);
	
	for (int i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM2, i);
		stopwatch_msleep_wd(30);
	}
	
	for (int i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_LIGHT_PWM2, i);
		stopwatch_msleep_wd(30);
	}
	lcd_control &= ~PASIC_LIGHT_CONTROL_PWM2_EN;
	
	// ================= FLASH LIGHT =================
	printf("Testing gpio flash light...\n");
	
	for (int i = 0; i < 8; ++i) {
		gpio_set(GPIO_LED_FL_EN, true);
		stopwatch_msleep_wd(500);
		
		gpio_set(GPIO_LED_FL_EN, false);
		stopwatch_msleep_wd(500);
	}
}

int main(void) {
	wdt_init();
	i2c_init();
	
	gpio_init_output(GPIO_LED_FL_EN, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_PUSHPULL, GPIO_PDPU_NONE, false);
	
	dump_all_regs();
	test_vibra();
	test_backlight();
	test_pickoff_sound();

	printf("Done!\n");
	
	return 0;
}
