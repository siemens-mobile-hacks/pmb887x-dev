#include <pmb887x.h>
#include <printf.h>

void data_abort_handler2(void);

volatile bool is_data_abort = false;
volatile bool need_ignore_data_abort = false;
volatile uint32_t data_abort_context[16];
volatile uint8_t data_abort_stack[0x4000];
volatile uint32_t curr_src_reg = 0;
volatile int curr_irq = -1;

static const uint32_t regs[] = {
	#if defined(PMB8876)
	// cat ../lib/gen/pmb8876_regs.h  | grep -P -i '#define[\t]+([a-z0-9]*)_BASE' -i | awk '{print $2}' | sort | uniq | sed 's/$/,/g'
	AFC_BASE,
	AMC_BASE,
	CAPCOM0_BASE,
	CAPCOM1_BASE,
	CIF_BASE,
	DIF_BASE,
	DMAC_BASE,
	DSP_BASE,
	EBU_BASE,
	GPIO_BASE,
	GPRSCU_BASE,
	GPTU0_BASE,
	GPTU1_BASE,
	I2C_BASE,
	KEYPAD_BASE,
	MCI_BASE,
	MMCI_BASE,
	MMICIF_BASE,
	NVIC_BASE,
	PLL_BASE,
	RTC_BASE,
	SCU_BASE,
	SIM_BASE,
	STM_BASE,
	TPU_BASE,
	USART0_BASE,
	USART1_BASE,
	USB_BASE,
	0xF1100000,
	0xF4600000,
	#elif defined(PMB8875)
	// cat ../lib/gen/pmb8875_regs.h  | grep -P -i '#define[\t]+([a-z0-9]*)_BASE' -i | awk '{print $2}' | sort | uniq | sed 's/$/,/g'
	AFC_BASE,
	AMC_BASE,
	CAPCOM0_BASE,
	CAPCOM1_BASE,
	DIF_BASE,
	DMAC_BASE,
	DSP_BASE,
	EBU_BASE,
	GPIO_BASE,
	GPRSCU_BASE,
	GPTU0_BASE,
	GPTU1_BASE,
	I2C_BASE,
	KEYPAD_BASE,
	NVIC_BASE,
	PLL_BASE,
	RTC_BASE,
	SCU_BASE,
	STM_BASE,
	TPU_BASE,
	USART0_BASE,
	USART1_BASE,
	USB_BASE,
	0xF1100000,
	0xF4600000,
	#endif
};

static void write_addr(uint32_t addr, uint32_t v) {
	need_ignore_data_abort = true;
	REG(addr) = v;
	need_ignore_data_abort = false;
}

static uint32_t read_addr(uint32_t addr) {
	is_data_abort = false;
	need_ignore_data_abort = true;
	uint32_t v = REG(addr);
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

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;
	
	curr_irq = irqn;
	REG(curr_src_reg) = MOD_SRC_CLRR;
	
	NVIC_IRQ_ACK = 1;
}

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	const bool for_config = true;
	
	SCU_RTCIF = 0xAA;
	SCU_CLC = 0x200;
	SCU_EXTI = 0xFFFFFFFF;
	
	for (int i = 0; i < 0x200; i++)
		NVIC_CON(i) = 1;
	
	for (uint32_t i = 0; i < ARRAY_SIZE(regs); i++) {
		uint32_t addr = regs[i];
		
		write_addr(addr, 0x100);
		stopwatch_msleep_wd(10);
		
		printf("%08X\n", addr);
		for (int j = 0xA0; j < 0xFF; j += 4) {
			uint32_t v = read_addr(addr + j);
			if (v != 0xFFFFFFFF) {
				curr_irq = -1;
				curr_src_reg = addr + j;
				write_addr(curr_src_reg, read_addr(curr_src_reg) | MOD_SRC_SRE);
				write_addr(curr_src_reg, read_addr(curr_src_reg) | MOD_SRC_CLRR);
				write_addr(curr_src_reg, read_addr(curr_src_reg) | MOD_SRC_SETR);
				
				stopwatch_msleep_wd(10);
				
				if (curr_irq != -1) {
					printf("+%02X: %08X [irq %d]\n", j, read_addr(addr + j), curr_irq);
				}
			}
		}
	}
	
	printf("Done.\n");
	return 0;
}
