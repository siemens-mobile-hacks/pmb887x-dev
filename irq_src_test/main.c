#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	cpu_enable_irq(false);
	
	for (int i = 0; i < 0x200; i++)
		NVIC_CON(i) = 1;
	
	SCU_EXTI1_SRC |= MOD_SRC_CLRR;
	SCU_EXTI1_SRC |= MOD_SRC_SRE;
	
	SCU_EXTI1_SRC |= MOD_SRC_SETR;
	printf("MOD_SRC_SRR=%d\n", SCU_EXTI1_SRC & MOD_SRC_SRR ? 1 : 0);
	
	SCU_EXTI1_SRC |= MOD_SRC_CLRR;
	printf("MOD_SRC_SRR=%d\n", SCU_EXTI1_SRC & MOD_SRC_SRR ? 1 : 0);
	
	SCU_EXTI1_SRC |= MOD_SRC_SETR;
	printf("MOD_SRC_SRR=%d\n", SCU_EXTI1_SRC & MOD_SRC_SRR ? 1 : 0);
	
	SCU_EXTI1_SRC &= ~MOD_SRC_SRE;
	printf("MOD_SRC_SRR=%d\n", SCU_EXTI1_SRC & MOD_SRC_SRR ? 1 : 0);
	
	SCU_EXTI1_SRC |= MOD_SRC_CLRR;
	SCU_EXTI1_SRC |= MOD_SRC_SETR;
	printf("MOD_SRC_SRR=%d\n", SCU_EXTI1_SRC & MOD_SRC_SRR ? 1 : 0);
	cpu_enable_irq(true);
	stopwatch_msleep_wd(10);
	printf("now enable irq\n");
	SCU_EXTI1_SRC |= MOD_SRC_SRE;
	stopwatch_msleep_wd(10);
	printf("done\n");
	
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
	
	printf("irq: %d\n", irqn);
	
	SCU_EXTI1_SRC |= MOD_SRC_CLRR;
	
	NVIC_IRQ_ACK = 1;
}
