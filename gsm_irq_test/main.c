#include <pmb887x.h>
#include <printf.h>

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

int unk_7530 = 0;

int main(void) {
	wdt_init();
	
	uint32_t addr;
	for (addr = 0xf2800030; addr <= 0xf28002a8; ++addr) {
		REG(addr) = 0;
	}
	
	cpu_enable_irq(false);

	writel(256, (void *)0xF6400000);
	writel(1, (void *)0xF6400068);
	writel(4, (void *)0xF640006C);
	writel(2, (void *)0xF6400070);
	
	for (int i = 0; i < 512; i++) {
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

	NVIC_CON(0x77) = 1;
	
	printf("Xuj!\n");
	
	cpu_enable_irq(true);

	while (true) {
		wdt_serve();
	}
	
	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;
	
	printf("IRQ FIRED: %X\n", irqn);
	
	if (irqn == 0x77) {
		printf("GSM TIMER: %d \n", STM_TIM4);
		GSM_CON_SET( GSM_CON() | PMB8876_GSM_TPU_CON_RESET );
	}
	
	NVIC_IRQ_ACK = 1;
}

