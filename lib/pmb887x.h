#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef CPU_OSC_FREQ
#define CPU_OSC_FREQ	26000000
#endif

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define PMB8876_SYSTEM_FREQ 26000000

#define MMIO8(addr)			(*(volatile uint8_t *)(addr))
#define MMIO16(addr)		(*(volatile uint16_t *)(addr))
#define MMIO32(addr)		(*(volatile uint32_t *)(addr))
#define MMIO64(addr)		(*(volatile uint64_t *)(addr))

#define BITS_PER_LONG		32

#define BIT(n)				(1 << (n))

#define __AC(X,Y)			(X##Y)
#define _AC(X,Y)			__AC(X,Y)
#define _AT(T,X)			((T)(X))
#define UL(x)				(_AC(x, UL))
#define ULL(x)				(_AC(x, ULL))

#define GENMASK(h, l) \
	(((~UL(0)) - (UL(1) << (l)) + 1) & \
	 (~UL(0) >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~ULL(0)) - (ULL(1) << (l)) + 1) & \
	 (~ULL(0) >> (BITS_PER_LONG_LONG - 1 - (h))))

#define REG_BYTE(addr)		MMIO8(addr)
#define REG_SHORT(addr)		MMIO16(addr)
#define REG(addr)			MMIO32(addr)

#define SET_BIT(value, v, shift, mask)			(value) = (((value) & ~(mask << shift)) | ((v & mask) << shift))
#define GET_BIT(value, shift, mask)				(((value) >> shift) & mask)

#define __IRQ __attribute__((interrupt))

#include "gen/board.h"
#include "gen/cpu.h"

#include "printf.h"
#include "usart.h"
#include "wdt.h"
#include "gpio.h"
#include "i2c.h"
#include "cpu.h"
#include "stopwatch.h"

// CPU Vectors
__IRQ void reset_handler(void);
__IRQ void undef_handler(void);
__IRQ void swi_handler(void);
__IRQ void prefetch_abort_handler(void);
__IRQ void data_abort_handler(void);
__IRQ void reserved_handler(void);
__IRQ void irq_handler(void);
__IRQ void fiq_handler(void);
