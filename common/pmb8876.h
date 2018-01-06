
#ifndef __PMB8876_H__
#define __PMB8876_H__

#include "pmb8876_regs.h"
#include "gpio.h"

#define REG(addr)								(*(volatile unsigned int *) (addr))
#define REG_BYTE(addr)							(*(volatile unsigned char *) (addr))
#define REG_SHORT(addr)							(*(volatile unsigned short *) (addr))

#define SET_BIT(value, v, shift, mask)			(value) = (((value) & ~(mask << shift)) | ((v & mask) << shift))
#define GET_BIT(value, shift, mask)				(((value) >> shift) & mask)

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

enum {
	UART_SPEED_12000 = 0x003f00f2, 
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
void init_watchdog_noinit();
void init_watchdog();
void switch_watchdog();
void serve_watchdog();

// utils
char to_hex(unsigned char b);
void hexdump(void *data, unsigned int len);
void hexnum(void *d, unsigned int len);
const char *itoa(unsigned int val, unsigned int base);
void exec_address(unsigned int addr);

// UART
void pmb8876_serial_set_speed(unsigned int speed);
void pmb8876_serial_putc(char c);
char pmb8876_serial_getc();
void pmb8876_serial_print(const char *data);
char pmb8876_serial_has_byte();

// CPU
unsigned int get_cpu_freq();
const char *get_cpu_mode(unsigned int cpsr);


// EABI
unsigned int __udivmodsi4(unsigned int num, unsigned int den, unsigned int * rem_p);
signed int __aeabi_idiv(signed int num, signed int den);
unsigned int __aeabi_uidiv(unsigned int num, unsigned int den);

#endif /* __PMB8876_H__ */
