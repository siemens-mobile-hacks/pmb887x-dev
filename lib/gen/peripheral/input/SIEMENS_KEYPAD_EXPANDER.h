#pragma once

#include <bitops.h> // IWYU pragma: export

// SIEMENS_KEYPAD_EXPANDER
// Siemens keypad expander: a COP8TAC5XLQ8 MCU running a Siemens mask ROM (SK65 D3401)
#define	SIEMENS_KEYPAD_EXPANDER_I2C_ADDR				0x51

/* Key event queue, 16 bytes terminated by a zero byte */
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS					0x20
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS_CODE				GENMASK(6, 0)	 // Key code: ((PX_IN + 2) & 7) << 4 | (PX_OUT + 1), PX_OUT + 1 = 9 for a key to GND
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS_CODE_SHIFT		0
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS_PRESSED			BIT(7)			 // Event kind
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS_PRESSED_RELEASED	0x0
#define	SIEMENS_KEYPAD_EXPANDER_EVENTS_PRESSED_PRESSED	0x80

/* Expander status */
#define	SIEMENS_KEYPAD_EXPANDER_STATUS					0xD0
#define	SIEMENS_KEYPAD_EXPANDER_STATUS_EVENTS			BIT(0)			 // Key events are queued
#define	SIEMENS_KEYPAD_EXPANDER_STATUS_EVENTS_EMPTY		0x0
#define	SIEMENS_KEYPAD_EXPANDER_STATUS_EVENTS_PENDING	0x1

/* Read by the firmware when it resynchronizes the key state */
#define	SIEMENS_KEYPAD_EXPANDER_REG_F0					0xF0
