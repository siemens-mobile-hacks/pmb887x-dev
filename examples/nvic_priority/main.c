#include <pmb887x.h>

int main(void) {
	wdt_init();
	
	cpu_enable_irq(false);
	
	VIC_CON(VIC_TPU_INT0_IRQ) = 2;
	VIC_CON(VIC_TPU_INT1_IRQ) = 3;
	
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


// 00010078
// 01000000

__IRQ void irq_handler(void) {
	uint32_t con = VIC_IRQ_CON;

	uint32_t irqn = (con & VIC_IRQ_CON_NUM) >> VIC_IRQ_CON_NUM_SHIFT;
	uint32_t priority = (con & VIC_IRQ_CON_PRIORITY) >> VIC_IRQ_CON_PRIORITY_SHIFT;

	if (irqn == 0)
		return;

	//VIC_IRQ_CON = (irqn & ~VIC_IRQ_CON_MASK_PRIORITY) | (priority << VIC_IRQ_CON_MASK_PRIORITY_SHIFT);

	printf("IRQ FIRED: %X // %08X\n", irqn, con);

	if (irqn == VIC_TPU_INT0_IRQ) {
		TPU_SRC(0) |= MOD_SRC_CLRR;
	} else if (irqn == VIC_TPU_INT1_IRQ) {
		TPU_SRC(1) |= MOD_SRC_CLRR;
	}

	//VIC_IRQ_CON = con;
}


__IRQ void irq_handler2(void) {
	uint32_t stat = VIC_IRQ_CON;

	int irqn = VIC_IRQ_CURRENT;
	if (irqn == 0)
		return;

	// VIC_IRQ_CON = (VIC_IRQ_CON & ~0x0F000000) | (0xF << 24);

	printf("IRQ FIRED: %X // %08X // %d\n", irqn, stat, 0);

	if (irqn == VIC_TPU_INT0_IRQ) {
		TPU_SRC(0) |= MOD_SRC_CLRR;
	} else if (irqn == VIC_TPU_INT1_IRQ) {
		TPU_SRC(1) |= MOD_SRC_CLRR;
	}

	VIC_IRQ_ACK = 1;
}

