#include <pmb887x.h>
#include <printf.h>

void data_abort_handler2(void);

volatile bool is_data_abort = false;
volatile bool need_ignore_data_abort = false;
volatile uint32_t data_abort_context[16];
volatile uint8_t data_abort_stack[0x4000];
volatile uint32_t curr_icr_reg = 0;
volatile uint32_t curr_icr_val = 0;
volatile int curr_irq = -1;

static void write_addr(uint32_t addr, uint32_t v) {
	need_ignore_data_abort = true;
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	REG(addr) = v;
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	need_ignore_data_abort = false;
}

static uint32_t read_addr(uint32_t addr) {
	is_data_abort = false;
	need_ignore_data_abort = true;
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	uint32_t v = REG(addr);
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	__asm__ volatile("nop");
	need_ignore_data_abort = false;
	return is_data_abort ? 0xFFFFFFFF : v;
}

void data_abort_handler2(void) {
	is_data_abort = true;
	if (!need_ignore_data_abort) {
		printf("data abort!\n");
		while (true);
	}
}

static int check_irq() {
	int irqn = VIC_IRQ_CURRENT;
	VIC_IRQ_ACK = 1;
	return irqn;
}

int main(void) {
	wdt_init();
	cpu_enable_irq(false);
	
	for (int i = 0; i < 0x200; i++)
		VIC_CON(i) = 1;
	
	uint32_t addr = DIF_BASE;
	
	USART_IMSC(USART0);

	write_addr(addr, 0x100);
	stopwatch_msleep_wd(10);

	for (int i = 0x30; i < 0xEC; i += 4) {
		printf("----------------\n");
		printf("%08X\n", addr + i);

		printf("NONE\n");
		printf("%08X\n", read_addr(addr + i + 4)); // RIS
		printf("%08X\n", read_addr(addr + i + 8)); // MIS

		stopwatch_msleep_wd(10);

		printf("ISR\n");
		write_addr(addr + i + 0, 1); // IMSC
		write_addr(addr + i + 0x10, 1); // ISR
		printf("%08X\n", read_addr(addr + i + 4)); // RIS
		printf("%08X\n", read_addr(addr + i + 8)); // MIS

		int irq = check_irq();
		printf("irq %d\n", irq);

		printf("----------------\n");

		stopwatch_msleep_wd(10);

		printf("IMSC\n");
		write_addr(addr + i + 0, 0); // IMSC
		printf("%08X\n", read_addr(addr + i + 4)); // RIS
		printf("%08X\n", read_addr(addr + i + 8)); // MIS

		stopwatch_msleep_wd(10);

		printf("ICR\n");
		write_addr(addr + i + 0x0C, 1); // ICR
		printf("%08X\n", read_addr(addr + i + 4)); // RIS
		printf("%08X\n", read_addr(addr + i + 8)); // MIS
		printf("\n\n");

		if (irq)
			break;
	}

	/*
	for (int i = 0x04; i < 0xFF; i += 4) {
		printf("%08X + %08X: ", addr + i, addr + i + 0xC);
		
		// 0 134
		// 3 135
		// 4 136
		// 8 137
		
		uint32_t mask = 0xFF;
		
		write_addr(addr + i, mask);
		write_addr(addr + i + 0xC, mask);
		
		
		stopwatch_msleep_wd(10);
		printf("%d\n", check_irq());
		
		write_addr(addr + i, 0);
		write_addr(addr + i + 0xC, 0);
		
	}
	*/
	/*
	for (int i = 0x04; i < 0xFF; i += 4) {
		printf("%08X: ", addr + i);
		write_addr(addr + i, 0xFFFFFFFF);
		stopwatch_msleep_wd(10);
		printf("%d\n", check_irq());
	}
	*/
	/*
	uint32_t isr_addr = addr + 0xC4;
	uint32_t icr_addr = isr_addr - 4 * 1;
	uint32_t mis_addr = isr_addr - 4 * 2;
	uint32_t imsc_addr = isr_addr - 4 * 3;
	uint32_t ris_addr = isr_addr - 4 * 4;
	
	for (int i = 0x04; i < 0xFF; i += 4) {
		printf("%08X: ", addr + i);
		write_addr(addr + i, 0xFFFFFFFF);
		stopwatch_msleep_wd(10);
		printf("%d\n", VIC_IRQ_CURRENT);
	}
	
	for (int i = 0; i < 32; i++) {
		curr_icr_val = 1 << i;
		curr_icr_reg = icr_addr;
		
		write_addr(isr_addr, 1 << i);
		stopwatch_msleep_wd(10);
		
		if (curr_irq != -1 || 1) {
			printf("1 << %d [irq %d]\n", i, curr_irq);
		}
	}
	*/
	printf("Done.\n");
	return 0;
}
