#include <pmb887x.h>
#include <printf.h>

typedef struct {
	volatile uint32_t *reg;
	int irq;
	uint32_t cfg;
} exti_regs_t;

static volatile int last_exti = 0;
static volatile int last_exti_cnt = 0;
static volatile int last_exti_arr[32];

static exti_regs_t exti_regs[] = {
#ifdef PMB8876
	{&SCU_EXTI0_SRC,	VIC_SCU_EXTI0_IRQ,	SCU_EXTI_EXT0_FALLING | SCU_EXTI_EXT0_RISING},
	{&SCU_EXTI1_SRC,	VIC_SCU_EXTI1_IRQ,	SCU_EXTI_EXT1_FALLING | SCU_EXTI_EXT1_RISING},
	{&SCU_EXTI2_SRC,	VIC_SCU_EXTI2_IRQ,	SCU_EXTI_EXT2_FALLING | SCU_EXTI_EXT2_RISING},
	{&SCU_EXTI3_SRC,	VIC_SCU_EXTI3_IRQ,	SCU_EXTI_EXT3_FALLING | SCU_EXTI_EXT3_RISING},
	{&SCU_EXTI4_SRC,	VIC_SCU_EXTI4_IRQ,	SCU_EXTI_EXT4_FALLING | SCU_EXTI_EXT4_RISING},
	{&SCU_EXTI5_SRC,	VIC_SCU_EXTI5_IRQ,	SCU_EXTI_EXT5_FALLING | SCU_EXTI_EXT5_RISING},
	{&SCU_EXTI6_SRC,	VIC_SCU_EXTI6_IRQ,	SCU_EXTI_EXT6_FALLING | SCU_EXTI_EXT6_RISING},
	{&SCU_EXTI7_SRC,	VIC_SCU_EXTI7_IRQ,	SCU_EXTI_EXT7_FALLING | SCU_EXTI_EXT7_RISING},
#endif

#ifdef PMB8875
	{&SCU_EXTI0_SRC,	VIC_SCU_EXTI0_IRQ,	SCU_EXTI_EXT0_FALLING | SCU_EXTI_EXT0_RISING},
	{&SCU_EXTI1_SRC,	VIC_SCU_EXTI1_IRQ,	SCU_EXTI_EXT1_FALLING | SCU_EXTI_EXT1_RISING},
	{&SCU_EXTI2_SRC,	VIC_SCU_EXTI2_IRQ,	SCU_EXTI_EXT2_FALLING | SCU_EXTI_EXT2_RISING},
	{&SCU_EXTI3_SRC,	VIC_SCU_EXTI3_IRQ,	SCU_EXTI_EXT3_FALLING | SCU_EXTI_EXT3_RISING},
	{&SCU_EXTI4_SRC,	VIC_SCU_EXTI4_IRQ,	SCU_EXTI_EXT4_FALLING | SCU_EXTI_EXT4_RISING},
#endif
};

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	SCU_CLC = 0x100;

	printf("Hello!\n");

	// Enable all IRQS
	for (uint32_t i = 0; i < ARRAY_SIZE(exti_regs); i++) {
		VIC_CON(exti_regs[i].irq) = 1;
		*(exti_regs[i].reg) = MOD_SRC_CLRR;
		*(exti_regs[i].reg) = MOD_SRC_SRE;
		SCU_EXTI |= exti_regs[i].cfg;
	}
/*
	for (int i = 0; i < 8; i++) {
		MMIO32(SCU_BASE + 0xB8 + i * 4) = MOD_SRC_CLRR;
		MMIO32(SCU_BASE + 0xB8 + i * 4) = MOD_SRC_SRE;
	}

	VIC_CON(VIC_SCU_EXTI0_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI1_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI2_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI3_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI4_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI5_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI6_IRQ) = 1;
	VIC_CON(VIC_SCU_EXTI7_IRQ) = 1;

	SCU_EXTI = 0xFFFFFFFF;
*/

	#ifdef PMB8876
	int gpio_count = 114;
	#endif

	#ifdef PMB8875
	int gpio_count = 77;
	#endif

	printf("DO. DO SCAN. DO SCAN IT!\n");

	uint32_t alts[] = {
		GPIO_IS_ALT0,
		GPIO_IS_ALT1,
		GPIO_IS_ALT2,
		GPIO_IS_ALT3,
		GPIO_IS_ALT4,
		GPIO_IS_ALT5,
		GPIO_IS_ALT6
	};
	
	for (uint32_t alt = 0; alt < ARRAY_SIZE(alts); alt++) {
		printf("ALT%d:\n", alt);
		
		for (int i = 0; i < gpio_count; i++) {
			if (i == GPIO_USART0_TXD || i == GPIO_USART0_RXD || i == GPIO_PM_WADOG || i == 60)
				continue;
			GPIO_PIN(i) = 0;
		}
		
		for (int i = 0; i < gpio_count; i++) {
			if (i == GPIO_USART0_TXD || i == GPIO_USART0_RXD || i == GPIO_PM_WADOG || i == 60)
				continue;
			
			last_exti = -1;
			
			for (uint32_t i2 = 0; i2 < ARRAY_SIZE(last_exti_arr); i2++)
				last_exti_arr[i2] = -1;

			GPIO_PIN(i) = 0;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLUP | GPIO_ENAQ_ON;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLUP;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLDOWN | GPIO_ENAQ_ON;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLDOWN;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLUP | GPIO_PS_MANUAL;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_PDPU_PULLDOWN | GPIO_PS_MANUAL;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt] | GPIO_ENAQ_ON;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = alts[alt];
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = GPIO_PDPU_PULLUP | GPIO_PS_MANUAL;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = GPIO_PDPU_PULLDOWN | GPIO_PS_MANUAL;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = GPIO_PS_MANUAL;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = GPIO_PS_MANUAL | GPIO_ENAQ_ON;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = GPIO_PS_MANUAL | GPIO_ENAQ_OFF;
			stopwatch_msleep_wd(1);
			GPIO_PIN(i) = 0;
			stopwatch_msleep_wd(100);
			
			if (last_exti != -1) {
				printf("  GPIO_%d -> EXTI%d", i, last_exti);

				for (uint32_t i2 = 0; i2 < ARRAY_SIZE(last_exti_arr); i2++) {
					if (last_exti_arr[i2] != -1 && last_exti_arr[i2] != last_exti)
						printf(" EXTI%d", last_exti_arr[i2]);
				}

				last_exti_cnt = 0;
				printf("\n");
			}
		}
	}
	printf("done.\n");
	
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
	int irqn = VIC_IRQ_CURRENT;
	
	int found = 0;
	for (uint32_t i = 0; i < ARRAY_SIZE(exti_regs); i++) {
		if (irqn == exti_regs[i].irq) {
			last_exti = i;
			last_exti_arr[last_exti_cnt] = i;
			*(exti_regs[i].reg) = MOD_SRC_CLRR;
			*(exti_regs[i].reg) = MOD_SRC_SRE;
			last_exti_cnt++;
			found = 1;
		}
	}

	if (!found) {
		while (true);
	}
	
	VIC_IRQ_ACK = 1;
}
