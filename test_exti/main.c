#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	// Enable all IRQS
	SCU_EXTI1_SRC |= SCU_EXTI1_SRC_SRE;
	SCU_EXTI7_SRC |= SCU_EXTI7_SRC_SRE;
	
	SCU_EXTI = 0;
	SCU_EXTI |= SCU_EXTI_EXT1_RISING;
	SCU_EXTI |= SCU_EXTI_EXT7_RISING;
	
	NVIC_CON(NVIC_SCU_EXT1_IRQ) = 1;
	NVIC_CON(NVIC_SCU_EXT7_IRQ) = 1;
	
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
	int irqn = NVIC_CURRENT_IRQ;
	
	printf("irq: %d\n", irqn);
	
	if (irqn == NVIC_SCU_EXT1_IRQ) {
		SCU_EXTI1_SRC |= SCU_EXTI1_SRC_CLRR;
		SCU_EXTI1_SRC |= SCU_EXTI1_SRC_SRE;
		printf("slider open\n");
	}
	
	if (irqn == NVIC_SCU_EXT7_IRQ) {
		SCU_EXTI7_SRC |= SCU_EXTI7_SRC_CLRR;
		SCU_EXTI7_SRC |= SCU_EXTI7_SRC_SRE;
		printf("slider close\n");
	}
	
	NVIC_IRQ_ACK = 1;
}
