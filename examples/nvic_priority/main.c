#include <pmb887x.h>

int main(void) {
	wdt_init();
	
	cpu_enable_irq(false);
	
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	VIC_CON(VIC_TPU_INT1_IRQ) = 1;
	
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	
	TPU_SRC(0) = MOD_SRC_SRE | MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_SRE | MOD_SRC_CLRR;
	
	TPU_SRC(0) |= MOD_SRC_SETR;
	stopwatch_msleep(10);
	TPU_SRC(1) |= MOD_SRC_SETR;
	stopwatch_msleep(10);
	
	printf("START\n");
	
	cpu_enable_irq(true);
	stopwatch_msleep(10);
	cpu_enable_irq(false);
	
	TPU_SRC(0) |= MOD_SRC_SETR;
	stopwatch_msleep(10);
	TPU_SRC(1) |= MOD_SRC_SETR;
	stopwatch_msleep(10);
	cpu_enable_irq(true);
	
	printf("DONE\n");
	
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
	int irqn = VIC_CURRENT_IRQ;
	
	printf("IRQ FIRED: %X\n", irqn);
	if (irqn == VIC_TPU_INT0_IRQ) {
		TPU_SRC(0) |= MOD_SRC_CLRR;
	} else if (irqn == VIC_TPU_INT1_IRQ) {
		TPU_SRC(1) |= MOD_SRC_CLRR;
	}
	
	VIC_IRQ_ACK = 1;
}

