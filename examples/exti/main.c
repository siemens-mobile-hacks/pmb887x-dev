#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	// Enable all IRQS
	SCU_EXTI1_SRC |= MOD_SRC_SRE;
	SCU_EXTI7_SRC |= MOD_SRC_SRE;
	
	SCU_EXTI = 0;
	SCU_EXTI |= SCU_EXTI_EXT1_RISING;
	SCU_EXTI |= SCU_EXTI_EXT7_RISING;
	
	VIC_CON(VIC_SCU_EXTI1_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI7_IRQ) = 1;
	
	// Init slider pins
	GPIO_PIN(GPIO_OPEN_CLOSE_SW1) = GPIO_IS_ALT2;
	GPIO_PIN(GPIO_OPEN_CLOSE_SW2) = GPIO_IS_ALT2;
	
	while (true) {
		cpu_enable_irq(false);
		printf("wait...\n");
		cpu_enable_irq(true);
		
		stopwatch_msleep_wd(1000);
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
	int irqn = VIC_CURRENT_IRQ;
	
	printf("irq: %d\n", irqn);
	
	if (irqn == VIC_SCU_EXTI1_IRQ) {
		SCU_EXTI1_SRC |= MOD_SRC_CLRR;
		SCU_EXTI1_SRC |= MOD_SRC_SRE;
		printf("slider open\n");
	}
	
	if (irqn == VIC_SCU_EXTI7_IRQ) {
		SCU_EXTI7_SRC |= MOD_SRC_CLRR;
		SCU_EXTI7_SRC |= MOD_SRC_SRE;
		printf("slider close\n");
	}
	
	VIC_IRQ_ACK = 1;
}
