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
	
	uint32_t addr = SSC_BASE;
	
	write_addr(addr, 0x100);
	stopwatch_msleep_wd(10);

	for (uint32_t i = 0; i < 0xFF; i += 4) {
		printf("%08X: %08X", addr + i, read_addr(addr + i));

		if (i == 0) {
			printf("\n");
			continue;
		}

		write_addr(addr + i, 0xFFFFFFFF);
		printf(" %08X", read_addr(addr + i));

		write_addr(addr + i, 0);
		printf(" %08X\n", read_addr(addr + i));
	}

	printf("Done.\n");
	return 0;
}
