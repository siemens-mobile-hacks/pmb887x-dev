#pragma once

#define D1601AA_I2C_ADDR		0x31 // Dialog/Twigo

// D1601AA regs
#define D1601AA_FAULT_REASON	0x05 /* HW fault reason */
#define D1601AA_LIGHT_PWM1		0x12 /* display backlight level 0x00...0x64 */
#define D1601AA_LIGHT_PWM2		0x13 /* keyboard backlight level 0x00...0x64 */
#define D1601AA_LED_CONTROL		0x14 /* led control */
#define D1601AA_VIBRA			0x47 /* vibra level 0x00...0x64 */
#define D1601AA_RF_REG			0x58 /* RF REG */

// D1601AA_FAULT_REASON values (read)
#define D1601AA_FAULT_UNDEFINED						0
#define D1601AA_FAULT_NO_REASON_STORED				1
#define D1601AA_FAULT_UNDERVOLTAGE_VBATT			2
#define D1601AA_FAULT_UNDERVOLTAGE_REG_3			3
#define D1601AA_FAULT_UNDERVOLTAGE_REG_2a			4
#define D1601AA_FAULT_UNDERVOLTAGE_REG_1			5
#define D1601AA_FAULT_SHUTDOWN_BY_REGISTER			6
#define D1601AA_FAULT_WATCHDOG_MIN_TIME				7
#define D1601AA_FAULT_WATCHDOG_MAX_TIME				8
#define D1601AA_FAULT_OVERVOLTAGE_VBATT				9

// D1601AA_RF_REG values
#define D1601AA_VRF1			(1 << 0) // <-- на EL71 тут висит VDD_RF1
#define D1601AA_VRF2			(1 << 1) // <-- на EL71 тут висит VDD_RF2
#define D1601AA_VRF3			(1 << 2) // <-- на EL71 тут висит VDD_BT

// D1601AA_LED_CONTROL values
#define D1601AA_LED1_EN			(1 << 1) // <-- на E71 тут SLI led, на EL71 не подключен
#define D1601AA_LED2_EN			(1 << 2) // <-- Сюда подключен EXTBOOST_EN, что бы работала вспышка и подсветки нужно включить его
#define D1601AA_LIGHT_PWM1_EN	(1 << 3) // <-- Подсветка дисплея
#define D1601AA_LIGHT_PWM2_EN	(1 << 4) // <-- Подсветка клавиатуры
