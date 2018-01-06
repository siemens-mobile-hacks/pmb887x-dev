#include <printf.h>
#include "main.h"

/* register */
#define PMB8876_GSM_TPU_CON		0xF64000F8

/* flags */
#define PMB8876_GSM_TPU_CON_RESET	0x4000
#define PMB8876_GSM_TPU_CON_ENABLE	0x1000

#define PMB8876_GSM_CLOCK_FREQ		2166000


#define GSM_CON()		readl((void *)PMB8876_GSM_TPU_CON)
#define GSM_CON_SET(x)		writel(x, (void *)PMB8876_GSM_TPU_CON);


#define writel(v, d) REG(d) = v
#define readl(d) REG(d)


/* handlers */
void __IRQ reset_addr() {
	pmb8876_serial_print("\n***** reset_addr! *****\n");
}

void __IRQ undef_addr() {
	pmb8876_serial_print("\n***** undef_addr! *****\n");
}

void __IRQ swi_addr() {
	
}

void __IRQ prefetch_addr() {
	pmb8876_serial_print("\n***** prefetch_addr! *****\n");
	while (1);

	
}
void __IRQ abort_addr() {
	pmb8876_serial_print("\n***** abort_addr! *****\n");
	while (1);
}

void __IRQ reserved_addr() {
	
}


int unk_7530 = 0;

void __IRQ c_irq_handler() {
	
	int irqn = REG(IRQ_CURRENT_NUM);
	
	//printf("IRQ FIRED: %X\n", irqn);
	
	if(irqn == 0x77) {
		printf("GSM TIMER: %d \n", REG(STM_4));
		GSM_CON_SET( GSM_CON() | PMB8876_GSM_TPU_CON_RESET );
	}
	
	REG(IRQ_ACK) = 1;
}

void __IRQ fiq_test() {
	pmb8876_serial_print("fiq_test!\n");
	while (1);
}


void main() {
	init_watchdog_noinit();
	
	int i;
	void **vectors = (void **) 0;
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = swi_addr;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = reserved_addr;
	vectors[14] = c_irq_handler; // asm_irq_handler;
	vectors[15] = fiq_test;
	
	unsigned int addr;
	for (addr = 0xf2800030; addr <= 0xf28002a8; ++addr) {
		REG(addr) = 0;
	}
	
	enable_irq(1);

	
	writel(256, (void *)0xF6400000);
	writel(1, (void *)0xF6400068);
	writel(4, (void *)0xF640006C);
	writel(2, (void *)0xF6400070);
	
	for( i = 0; i < 512; i ++ ) {
		writel(0, (void *)0xF6401800 + (i*4));
	}
	
	writel(65024, (void *)0xF6401800);
	writel(0, (void *)0xF6401804);
	writel(0, (void *)0xF6401808);
	writel(32256, (void *)0xF640180C);
	writel(32760, (void *)0xF6401810);
	writel(4096, (void *)0xF6401814);
	writel(0, (void *)0xF6400040);
	writel(6, (void *)0xF640003C);
	writel(0x80000000, (void *)0xF6400044);
	writel(9999, (void *)0xF6400020);
	writel(0, (void *)0xF640002C);
	
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_RESET);
	
	writel(0, (void *)0xF6400024);
	
	writel(0x7530, (void *)0xF6400028);
	writel(3, (void *)0xF640005C);
	
	GSM_CON_SET(GSM_CON() | PMB8876_GSM_TPU_CON_ENABLE);

	PMB8876_IRQ(0x77) = 1;
	
	printf("Xuj!\n");
	
	while(1) {
		serve_watchdog();
	}
}

