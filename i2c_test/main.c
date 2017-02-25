#include <pmb8876.h>
#include <i2c.h>
#include <d1601aa_i2c.h>

#include "main.h"

void _start() {
	init_sdram();
	enable_irq(0);
	enable_fiq(0);
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	init_watchdog();
	
	int i;
	void **vectors = (void **) 0;
	for (i = 0; i < 8; ++i)
		vectors[i] = (void *) (&_cpu_vectors)[i];
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = loop;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = loop;
	vectors[14] = loop;
	vectors[15] = loop;
	
	i2c_init();
	
	test_vibra();
	test_backlight();
	test_pickoff_sound();
	
	pmb8876_serial_print("Done!\n");
	
	while (1);
}

void mdelay(unsigned int popugays) {
	popugays <<= 6;
	while (popugays--)
		serve_watchdog();
}

void test_vibra() {
	int ret, i;
	
	pmb8876_serial_print("Testing i2c vibra...\n");
	
	for (i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_VIBRA, i);
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_VIBRA, i);
		mdelay(30);
	}
}

void test_pickoff_sound() {
	int ret;
	
	pmb8876_serial_print("Testing pickoff sound...\n");
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x44, 0x24);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x46, 0x5F);
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0x42, 0x5F);
}

void test_backlight() {
	int ret, lcd_control = 0, i;
	
	// Отключаем вспышку, что бы после подачи VBOOT (D1601AA_LED2_EN) она не включилась
	REG(GPIO_LED_FL_EN) = PMB8876_GPIO(NO_ALT,	NO_ALT,	MANUAL,	OUT,	LOW,	PUSHPULL,	NONE,	NO_ENAQ);
	
	// Даём питалово
	lcd_control |= D1601AA_LED2_EN;
	
	// ================= DISPLAY =================
	pmb8876_serial_print("Testing i2c LCD backlight...\n");
	
	lcd_control |= D1601AA_LIGHT_PWM1_EN;
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LED_CONTROL, lcd_control);
	
	for (i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM1, i);
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM1, i);
		mdelay(30);
	}
	lcd_control &= ~D1601AA_LIGHT_PWM1_EN;
	
	// ================= KEYBOARD =================
	pmb8876_serial_print("Testing i2c keyboard backlight...\n");
	
	lcd_control |= D1601AA_LIGHT_PWM2_EN;
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LED_CONTROL, lcd_control);
	
	for (i = 0; i < 0x64; ++i) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM2, i);
		mdelay(30);
	}
	
	for (i = 0x64; i-- > 0; ) {
		i2c_smbus_write_byte(D1601AA_I2C_ADDR, D1601AA_LIGHT_PWM2, i);
		mdelay(30);
	}
	lcd_control &= ~D1601AA_LIGHT_PWM2_EN;
	
	// ================= FLASH LIGHT =================
	pmb8876_serial_print("Testing gpio flash light...\n");
	
	for (i = 0; i < 8; ++i) {
		REG(GPIO_LED_FL_EN) = PMB8876_GPIO(NO_ALT,	NO_ALT,	MANUAL,	OUT,	HIGH,	PUSHPULL,	NONE,	NO_ENAQ);
		mdelay(500);
		REG(GPIO_LED_FL_EN) = PMB8876_GPIO(NO_ALT,	NO_ALT,	MANUAL,	OUT,	LOW,	PUSHPULL,	NONE,	NO_ENAQ);
		mdelay(500);
	}
}

void __IRQ reset_addr() {
	pmb8876_serial_print("\n***** reset_addr! *****\n");
}
void __IRQ undef_addr() {
	pmb8876_serial_print("\n***** undef_addr! *****\n");
}
void __IRQ prefetch_addr() {
	pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	while (1);
}
void __IRQ abort_addr() {
	pmb8876_serial_print("\n***** abort_addr! *****\n");
	while (1);
}
void __IRQ loop() {
	while (1);
}
