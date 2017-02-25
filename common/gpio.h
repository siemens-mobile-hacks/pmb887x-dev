#pragma once

#include <pmb8876.h>

#define PMB8876_GPIO_BASE		0xF4300020

// shift
#define PMB8876_GPIO_IS			0
#define PMB8876_GPIO_OS			4
#define PMB8876_GPIO_PS			8
#define PMB8876_GPIO_DATA		9
#define PMB8876_GPIO_DIR		10
#define PMB8876_GPIO_PPEN		12
#define PMB8876_GPIO_PDPU		13
#define PMB8876_GPIO_ENAQ		15

// mask
#define PMB8876_GPIO_IS_MASK		7
#define PMB8876_GPIO_OS_MASK		7
#define PMB8876_GPIO_PS_MASK		1
#define PMB8876_GPIO_DATA_MASK		1
#define PMB8876_GPIO_DIR_MASK		1
#define PMB8876_GPIO_PPEN_MASK		1
#define PMB8876_GPIO_PDPU_MASK		3
#define PMB8876_GPIO_ENAQ_MASK		1

// Port Select
#define PMB8876_GPIO_PS_ALT		0
#define PMB8876_GPIO_PS_MANUAL	1

// DIR
#define PMB8876_GPIO_DIR_IN		0
#define PMB8876_GPIO_DIR_OUT	1

// DIR
#define PMB8876_GPIO_DATA_LOW		0
#define PMB8876_GPIO_DATA_HIGH		1

// IS / OS
#define PMB8876_GPIO_NO_ALT		0
#define PMB8876_GPIO_ALT0		1
#define PMB8876_GPIO_ALT1		2
#define PMB8876_GPIO_ALT2		3
#define PMB8876_GPIO_ALT3		4
#define PMB8876_GPIO_ALT4		5
#define PMB8876_GPIO_ALT5		6
#define PMB8876_GPIO_ALT6		7

// PDPU
#define PMB8876_GPIO_PDPU_NONE			0
#define PMB8876_GPIO_PDPU_PULLUP		1
#define PMB8876_GPIO_PDPU_PULLDOWN		2

// PPEN
#define PMB8876_GPIO_PPEN_PUSHPULL		0
#define PMB8876_GPIO_PPEN_OPENDRAIN		1

// ENAQ
#define PMB8876_GPIO_ENAQ_ENAQ			1
#define PMB8876_GPIO_ENAQ_NO_ENAQ		0

// GPIO PIN N => reg
#define PMB8876_GPIO_PIN(n) (PMB8876_GPIO_BASE + n * 4)

#define PMB8876_GPIO(is, os, ps, dir, data, ppen, pdpu, enaq) \
	( \
		(PMB8876_GPIO_ ## is		<< PMB8876_GPIO_IS)		| \
		(PMB8876_GPIO_ ## os		<< PMB8876_GPIO_OS)		| \
		(PMB8876_GPIO_PS_ ## ps		<< PMB8876_GPIO_PS)		| \
		(PMB8876_GPIO_DIR_ ## dir	<< PMB8876_GPIO_DIR)	| \
		(PMB8876_GPIO_DATA_ ## data	<< PMB8876_GPIO_DATA)	| \
		(PMB8876_GPIO_PPEN_ ## ppen	<< PMB8876_GPIO_PPEN)	| \
		(PMB8876_GPIO_PDPU_ ## pdpu	<< PMB8876_GPIO_PDPU)	| \
		(PMB8876_GPIO_ENAQ_ ## enaq	<< PMB8876_GPIO_ENAQ)	  \
	)

#define pmb8876_gpio_reg_set_bit(value, v, shift, mask)			(value) = (((value) & ~(mask << shift)) | ((v & mask) << shift))
#define pmb8876_gpio_reg_get_bit(value, shift, mask)			(((value) >> shift) & mask)

// write reg
#define pmb8876_gpio_reg_set_is(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_IS,   PMB8876_GPIO_IS_MASK)
#define pmb8876_gpio_reg_set_os(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_OS,   PMB8876_GPIO_OS_MASK)
#define pmb8876_gpio_reg_set_ps(reg, v)				pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PS,   PMB8876_GPIO_PS_MASK)
#define pmb8876_gpio_reg_set_dir(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_DIR,  PMB8876_GPIO_DIR_MASK)
#define pmb8876_gpio_reg_set_data(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_DATA, PMB8876_GPIO_DATA_MASK)
#define pmb8876_gpio_reg_set_ppen(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PPEN, PMB8876_GPIO_PPEN_MASK)
#define pmb8876_gpio_reg_set_pdpu(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_PDPU, PMB8876_GPIO_PDPU_MASK)
#define pmb8876_gpio_reg_set_enaq(reg, v)			pmb8876_gpio_reg_set_bit(reg, v, PMB8876_GPIO_ENAQ, PMB8876_GPIO_ENAQ_MASK)

// read reg
#define pmb8876_gpio_reg_get_is(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_IS,   PMB8876_GPIO_IS_MASK)
#define pmb8876_gpio_reg_get_os(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_OS,   PMB8876_GPIO_OS_MASK)
#define pmb8876_gpio_reg_get_ps(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PS,   PMB8876_GPIO_PS_MASK)
#define pmb8876_gpio_reg_get_dir(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_DIR,  PMB8876_GPIO_DIR_MASK)
#define pmb8876_gpio_reg_get_data(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_DATA, PMB8876_GPIO_DATA_MASK)
#define pmb8876_gpio_reg_get_ppen(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PPEN, PMB8876_GPIO_PPEN_MASK)
#define pmb8876_gpio_reg_get_pdpu(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_PDPU, PMB8876_GPIO_PDPU_MASK)
#define pmb8876_gpio_reg_get_enaq(reg)				pmb8876_gpio_reg_get_bit(reg, PMB8876_GPIO_ENAQ, PMB8876_GPIO_ENAQ_MASK)
