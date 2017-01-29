#pragma once

#include "pmb8876_regs.h"

#define REG(addr)			(*( unsigned int *) (addr))
#define PMB8876_EXPORT		__attribute__ ((visibility ("default")))
#define __IRQ				__attribute__((interrupt))

#define PMB8876_IRQ(n) (REG(0xf2800030 + ((n) * 4)))

enum { 
	ARM_CPU_MODE_USR = 0x10, ARM_CPU_MODE_FIQ = 0x11, ARM_CPU_MODE_IRQ = 0x12, ARM_CPU_MODE_SVC = 0x13, 
	ARM_CPU_MODE_ABT = 0x17, ARM_CPU_MODE_UND = 0x1b, ARM_CPU_MODE_SYS = 0x1f 
};

#define __mrc(coproc, opcode1, CRn, CRm, opcode2)\
  ({unsigned int rd; __asm__(\
    "MRC p" #coproc ", " #opcode1 ", %0, c" #CRn ", c" #CRm ", " #opcode2 \
    :"=r"(rd)); rd; })

// PLL
#define PLL_OSC 0xF45000A0
#define PLL_CON0 0xF45000A4
#define PLL_CON1 0xF45000A8
#define PLL_CON2 0xF45000AC

#define CPUID_ID        0
#define CPUID_CACHETYPE 1
#define CPUID_TCM       2
#define CPUID_TLBTYPE   3
#define CPUID_MPUIR     4
#define CPUID_MPIDR     5
#define CPUID_REVIDR    6

#define TCMTR_FORMAT_MASK       0xe0000000U
#define __stringify_1(x)        #x
#define __stringify(x)          __stringify_1(x)
#define read_cpuid(reg)                                                 \
         ({                                                              \
                 unsigned int __val;                                     \
                 asm("mrc        p15, 0, %0, c0, c0, " __stringify(reg)  \
                     : "=r" (__val)                                      \
                     :                                                   \
                     : "cc");                                            \
                 __val;                                                  \
         })
 

// USART
#define PMB8876_USART0_BASE   0xf1000000
#define PMB8876_USART0_CLC    PMB8876_USART0_BASE
#define PMB8876_USART0_BG     (PMB8876_USART0_BASE + 0x14)
#define PMB8876_USART0_FDV    (PMB8876_USART0_BASE + 0x18)
#define PMB8876_USART0_TXB    (PMB8876_USART0_BASE + 0x20)
#define PMB8876_USART0_RXB    (PMB8876_USART0_BASE + 0x24)
#define PMB8876_USART0_FCSTAT (PMB8876_USART0_BASE + 0x68)
#define PMB8876_USART0_ICR    (PMB8876_USART0_BASE + 0x70)

// STM
#define STM_4    0xF4B00020

extern unsigned int _cpu_vectors;

struct watchdog {
	unsigned int addr;
	unsigned int time;
} __g_watchdog;

enum {
	UART_SPEED_57600 = 0x001901d8, 
	UART_SPEED_115200 = 0x000c01d8, 
	UART_SPEED_230400 = 0x000501b4, 
	UART_SPEED_460800 = 0x00000092, 
	UART_SPEED_614400 = 0x000000c3, 
	UART_SPEED_921600 = 0x00000127, 
	UART_SPEED_1228800 = 0x0000018a, 
	UART_SPEED_1600000 = 0x00000000, 
	UART_SPEED_1500000 = 0x000001d0
};

void init_sdram();
void enable_irq(int flag);
void enable_fiq(int flag);
void disable_first_whatchdog();
void set_einit(char flag);
void dump_cpsr();

unsigned int get_cpsr();
unsigned int set_cpsr(volatile unsigned int r);

// watchdog
void init_watchdog();
void switch_watchdog();
void serve_watchdog();

// utils
char to_hex(unsigned char b);
void hexdump(void *data, unsigned int len);
const char *itoa(unsigned int val, unsigned int base);
void exec_address(unsigned int addr);

// UART
void pmb8876_serial_set_speed(unsigned int speed);
void pmb8876_serial_putc(char c);
char pmb8876_serial_getc();
void pmb8876_serial_print(const char *data);

// CPU
unsigned int get_cpu_freq();
const char *get_cpu_mode(unsigned int cpsr);


// EABI
unsigned int __udivmodsi4(unsigned int num, unsigned int den, unsigned int * rem_p);
signed int __aeabi_idiv(signed int num, signed int den);
unsigned int __aeabi_uidiv(unsigned int num, unsigned int den);

/////////////////

typedef union { /* 32 bits */
   struct {
      unsigned int DISR:1;              /*  = [0..0] = 0x00000001 */
      unsigned int DISS:1;              /*  = [1..1] = 0x00000002 */
      unsigned int SPEN:1;              /*  = [2..2] = 0x00000004 */
      unsigned int EDIS:1;              /*  = [3..3] = 0x00000008 */
      unsigned int SBWE:1;              /*  = [4..4] = 0x00000010 */
      unsigned int FSOE:1;              /*  = [5..5] = 0x00000020 */
      unsigned int _bit6:1;             /* 0 = [6..6] = 0x00000040 */
      unsigned int _bit7:1;             /* 0 = [7..7] = 0x00000080 */
      unsigned int RMC:3;               /*  = [8..10] = 0x00000700 */
      unsigned int _bit11:1;            /* 0 = [11..11] = 0x00000800 */
      unsigned int _bit12:1;            /* 0 = [12..12] = 0x00001000 */
      unsigned int _bit13:1;            /* 0 = [13..13] = 0x00002000 */
      unsigned int _bit14:1;            /* 0 = [14..14] = 0x00004000 */
      unsigned int _bit15:1;            /* 0 = [15..15] = 0x00008000 */
      unsigned int _bit16:1;            /* 0 = [16..16] = 0x00010000 */
      unsigned int _bit17:1;            /* 0 = [17..17] = 0x00020000 */
      unsigned int _bit18:1;            /* 0 = [18..18] = 0x00040000 */
      unsigned int _bit19:1;            /* 0 = [19..19] = 0x00080000 */
      unsigned int _bit20:1;            /* 0 = [20..20] = 0x00100000 */
      unsigned int _bit21:1;            /* 0 = [21..21] = 0x00200000 */
      unsigned int _bit22:1;            /* 0 = [22..22] = 0x00400000 */
      unsigned int _bit23:1;            /* 0 = [23..23] = 0x00800000 */
      unsigned int _bit24:1;            /* 0 = [24..24] = 0x01000000 */
      unsigned int _bit25:1;            /* 0 = [25..25] = 0x02000000 */
      unsigned int _bit26:1;            /* 0 = [26..26] = 0x04000000 */
      unsigned int _bit27:1;            /* 0 = [27..27] = 0x08000000 */
      unsigned int _bit28:1;            /* 0 = [28..28] = 0x10000000 */
      unsigned int _bit29:1;            /* 0 = [29..29] = 0x20000000 */
      unsigned int _bit30:1;            /* 0 = [30..30] = 0x40000000 */
      unsigned int _bit31:1;            /* 0 = [31..31] = 0x80000000 */
   } bits;
   unsigned long reg; 
} __attribute__((aligned(4))) STM_CLC_t;

typedef union { /* 32 bits */
   struct {
      unsigned int CMP0EN:1;            /*  = [0..0] = 0x00000001 */
      unsigned int CMP0IR:1;            /*  = [1..1] = 0x00000002 */
      unsigned int CMP0OS:1;            /*  = [2..2] = 0x00000004 */
      unsigned int _bit3:1;             /* 0 = [3..3] = 0x00000008 */
      unsigned int CMP1EN:1;            /*  = [4..4] = 0x00000010 */
      unsigned int CMP1IR:1;            /*  = [5..5] = 0x00000020 */
      unsigned int CMP1OS:1;            /*  = [6..6] = 0x00000040 */
      unsigned int _bit7:1;             /* 0 = [7..7] = 0x00000080 */
      unsigned int _bit8:1;             /* 0 = [8..8] = 0x00000100 */
      unsigned int _bit9:1;             /* 0 = [9..9] = 0x00000200 */
      unsigned int _bit10:1;            /* 0 = [10..10] = 0x00000400 */
      unsigned int _bit11:1;            /* 0 = [11..11] = 0x00000800 */
      unsigned int _bit12:1;            /* 0 = [12..12] = 0x00001000 */
      unsigned int _bit13:1;            /* 0 = [13..13] = 0x00002000 */
      unsigned int _bit14:1;            /* 0 = [14..14] = 0x00004000 */
      unsigned int _bit15:1;            /* 0 = [15..15] = 0x00008000 */
      unsigned int _bit16:1;            /* 0 = [16..16] = 0x00010000 */
      unsigned int _bit17:1;            /* 0 = [17..17] = 0x00020000 */
      unsigned int _bit18:1;            /* 0 = [18..18] = 0x00040000 */
      unsigned int _bit19:1;            /* 0 = [19..19] = 0x00080000 */
      unsigned int _bit20:1;            /* 0 = [20..20] = 0x00100000 */
      unsigned int _bit21:1;            /* 0 = [21..21] = 0x00200000 */
      unsigned int _bit22:1;            /* 0 = [22..22] = 0x00400000 */
      unsigned int _bit23:1;            /* 0 = [23..23] = 0x00800000 */
      unsigned int _bit24:1;            /* 0 = [24..24] = 0x01000000 */
      unsigned int _bit25:1;            /* 0 = [25..25] = 0x02000000 */
      unsigned int _bit26:1;            /* 0 = [26..26] = 0x04000000 */
      unsigned int _bit27:1;            /* 0 = [27..27] = 0x08000000 */
      unsigned int _bit28:1;            /* 0 = [28..28] = 0x10000000 */
      unsigned int _bit29:1;            /* 0 = [29..29] = 0x20000000 */
      unsigned int _bit30:1;            /* 0 = [30..30] = 0x40000000 */
      unsigned int _bit31:1;            /* 0 = [31..31] = 0x80000000 */
   } bits;
   unsigned long reg; 
} __attribute__((aligned(4))) STM_ICR_t;