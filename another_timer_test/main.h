#pragma once
#include <pmb8876.h>


/* register */
#define PMB8876_GSM_TPU_CLC		0xF6400000
#define PMB8876_GSM_TPU_CON		0xF64000F8

/* flags */
#define PMB8876_GSM_TPU_CON_RESET	0x4000
#define PMB8876_GSM_TPU_CON_ENABLE	0x1000
#define PMB8876_GSM_TPU_CLC_RMC(x)	((x << 8) & 0x127)


#define GSM_CON()			readl((void *)PMB8876_GSM_TPU_CON)
#define GSM_CON_SET(x)			writel(x, (void *)PMB8876_GSM_TPU_CON);


/* freq */
#define PMB8876_GSM_CLOCK_FREQ		21660000



#define HZ		100

#define writel(v, d)	REG(d) = v
#define readl(d)	REG(d)

#define printk printf


unsigned int __delay(unsigned int loops);
unsigned long calibrate_delay_converge(void);



void __IRQ reset_addr();
void __IRQ undef_addr();
void __IRQ swi_addr();
void __IRQ prefetch_addr();
void __IRQ abort_addr();
void __IRQ reserved_addr();
void __IRQ c_irq_handler();
void __IRQ fiq_test();
