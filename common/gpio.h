#pragma once

#define GPIO_IS			0
#define GPIO_OS			4
#define GPIO_PS			8
#define GPIO_DATA		9
#define GPIO_DIR		10
#define GPIO_PPEN		12
#define GPIO_PDPU		13
#define GPIO_ENAQ		15

#define GPIO_IS_MASK		7
#define GPIO_OS_MASK		7
#define GPIO_PS_MASK		1
#define GPIO_DATA_MASK		1
#define GPIO_DIR_MASK		1
#define GPIO_PPEN_MASK		1
#define GPIO_PDPU_MASK		3
#define GPIO_ENAQ_MASK		1

// Port Select
#define GPIO_PS_ALT		0
#define GPIO_PS_NONE	1

// DIR
#define GPIO_DIR_IN		0
#define GPIO_DIR_OUT	1

// IS / OS
#define GPIO_ALT_NONE	0
#define GPIO_ALT0		1
#define GPIO_ALT1		2
#define GPIO_ALT2		3
#define GPIO_ALT3		4
#define GPIO_ALT4		5
#define GPIO_ALT5		6
#define GPIO_ALT6		7

// PDPU
#define GPIO_PDPU_NONE			0
#define GPIO_PDPU_PULLUP		1
#define GPIO_PDPU_PULLDOWN		2

// PPEN
#define GPIO_PPEN_PUSHPULL		0
#define GPIO_PPEN_OPENDRAIN		1

// GPIO PIN N => ADDR
#define GPIO_PIN(n) (0xF4300020 + n * 4)

#define gpio_configure_pin(addr, is, os, ps, data, dir, ppen, pdpu, enaq) \
			REG(addr) =  \
				((is	&	GPIO_IS_MASK)		<<	GPIO_IS)	| \
				((os	&	GPIO_OS_MASK)		<<	GPIO_OS)	| \
				((ps	&	GPIO_PS_MASK)		<<	GPIO_PS)	| \
				((data	&	GPIO_DATA_MASK)		<<	GPIO_DATA)	| \
				((dir	&	GPIO_DIR_MASK)		<<	GPIO_DIR)	| \
				((ppen	&	GPIO_PPEN_MASK)		<<	GPIO_PPEN)	| \
				((pdpu	&	GPIO_PDPU_MASK)		<<	GPIO_PDPU)	| \
				((enaq	&	GPIO_ENAQ_MASK)		<<	GPIO_ENAQ);

// write
#define gpio_set_is(addr, v)		SET_BIT(REG(addr), v, GPIO_IS,   GPIO_IS_MASK)
#define gpio_set_os(addr, v)		SET_BIT(REG(addr), v, GPIO_OS,   GPIO_OS_MASK)
#define gpio_set_ps(addr, v)		SET_BIT(REG(addr), v, GPIO_PS,   GPIO_PS_MASK)
#define gpio_set_dir(addr, v)		SET_BIT(REG(addr), v, GPIO_DIR,  GPIO_DIR_MASK)
#define gpio_set_data(addr, v)		SET_BIT(REG(addr), v, GPIO_DATA, GPIO_DATA_MASK)
#define gpio_set_ppen(addr, v)		SET_BIT(REG(addr), v, GPIO_PPEN, GPIO_PPEN_MASK)
#define gpio_set_pdpu(addr, v)		SET_BIT(REG(addr), v, GPIO_PDPU, GPIO_PDPU_MASK)
#define gpio_set_enaq(addr, v)		SET_BIT(REG(addr), v, GPIO_ENAQ, GPIO_ENAQ_MASK)

// read
#define gpio_get_is(addr)				GET_BIT(REG(addr), GPIO_IS,   GPIO_IS_MASK)
#define gpio_get_os(addr)				GET_BIT(REG(addr), GPIO_OS,   GPIO_OS_MASK)
#define gpio_get_ps(addr)				GET_BIT(REG(addr), GPIO_PS,   GPIO_PS_MASK)
#define gpio_get_dir(addr)				GET_BIT(REG(addr), GPIO_DIR,  GPIO_DIR_MASK)
#define gpio_get_data(addr)				GET_BIT(REG(addr), GPIO_DATA, GPIO_DATA_MASK)
#define gpio_get_ppen(addr)				GET_BIT(REG(addr), GPIO_PPEN, GPIO_PPEN_MASK)
#define gpio_get_PDPU(addr)				GET_BIT(REG(addr), GPIO_PDPU, GPIO_PDPU_MASK)
#define gpio_get_enaq(addr)				GET_BIT(REG(addr), GPIO_ENAQ, GPIO_ENAQ_MASK)

/*
my $IS		= $value & 7;
my $OS		= ($value >> 4) & 7;
my $PS		= ($value >> 8) & 1;
my $DATA	= ($value >> 9) & 1;
my $DIR		= ($value >> 10) & 1;
my $PPEN	= ($value >> 12) & 1;
my $PDPU	= ($value >> 13) & 3;
my $ENAQ	= ($value >> 15) & 1;
*/