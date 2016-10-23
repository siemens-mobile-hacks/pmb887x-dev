#pragma once

#define REG(addr)         (*( unsigned int *) (addr))

#define __mrc(coproc, opcode1, CRn, CRm, opcode2)\
  ({unsigned int rd; __asm__(\
    "MRC p" #coproc ", " #opcode1 ", %0, c" #CRn ", c" #CRm ", " #opcode2 \
    :"=r"(rd)); rd; })


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

// EBU
#define EBU_BASE 		0xF0000000
#define EBU_ADDRSEL1 	(EBU_BASE + 0x88)
#define EBU_BUSCON1		(EBU_BASE + 0xC8)
#define EBU_SDRMREF0	(EBU_BASE + 0x40)
#define EBU_SDRMCON0	(EBU_BASE + 0x50)
#define EBU_SDRMOD0		(EBU_BASE + 0x60)
#define EBU_ADDRSEL0 	(EBU_BASE + 0x80)
#define EBU_ADDRSEL4 	(EBU_BASE + 0xA0)
#define EBU_BUSCON0 	(EBU_BASE + 0xC0)
#define EBU_BUSCON4 	(EBU_BASE + 0xE0)

// SCU
#define SCU_BASE		0xf4400000
#define SCU_WDTCON0		(SCU_BASE + 0x24)
#define SCU_WDTCON1		(SCU_BASE + 0x28)
#define SCU_CHIPID		(SCU_BASE + 0x60)
#define SCU_ROMAMCR		(SCU_BASE + 0x7C)

// PCL registers
#define PCL_CLC		0xf4300000
#define PCL_ID		0xf4300008
#define MON_CR1		0xf4300010
#define MON_CR2		0xf4300014
#define MON_CR3		0xf4300018
#define MON_CR4		0xf430001C
#define PCL_00		0xf4300020
#define PCL_51		0xf43000EC
#define PCL_62		0xf4300118

// STM
#define STM_4    0xF4B00020

static struct watchdog {
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

static void init_sdram();
static void enable_irq(int flag);
static void enable_fiq(int flag);
static void disable_first_whatchdog();
static void set_einit(char flag);
static void dump_cpsr();

// watchdog
static void init_watchdog();
static void switch_watchdog();
static void serve_watchdog();

// utils
static char to_hex(unsigned char b);
static void hexdump(unsigned char *data, unsigned int len);
static void exec_address(unsigned int addr);

// UART
static void pmb8876_serial_set_speed(unsigned int speed);
static void pmb8876_serial_putc(char c);
static char pmb8876_serial_getc();
static void pmb8876_serial_print(char *data);
