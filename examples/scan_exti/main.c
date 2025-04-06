#include <pmb887x.h>
#include <printf.h>

/*
	ALT1
	GPIO_56 -> EXT4
	GPIO_98 -> EXT4
	GPIO_108 -> EXT5

	ALT2
	GPIO_0 -> EXT0
	GPIO_7 -> EXT1
	GPIO_28 -> EXT6
	GPIO_29 -> EXT2
	GPIO_39 -> EXT1
	GPIO_101 -> EXT6

	ALT3
	GPIO_16 -> EXT3
	GPIO_18 -> EXT0
	GPIO_31 -> EXT3
	GPIO_58 -> EXT4

	ALT4
	GPIO_44 -> EXT5
	GPIO_47 -> EXT6
	GPIO_50 -> EXT5
	GPIO_51 -> EXT7
*/

typedef struct {
	volatile uint32_t *reg;
	int irq;
	uint32_t cfg;
} exti_regs_t;

static volatile int last_exti = 0;

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
	{&SCU_DSP_SRC(0),		VIC_SCU_EXTI5_IRQ,	SCU_EXTI_EXT5_FALLING | SCU_EXTI_EXT5_RISING},
	{&SCU_DSP_SRC(1),		VIC_SCU_EXTI6_IRQ,	SCU_EXTI_EXT6_FALLING | SCU_EXTI_EXT6_RISING},
	{&SCU_DSP_SRC(2),		VIC_SCU_EXTI7_IRQ,	SCU_EXTI_EXT7_FALLING | SCU_EXTI_EXT7_RISING},
#endif
};

int main(void) {
	wdt_init();
	cpu_enable_irq(true);
	
	SCU_CLC = 0x100;

	printf("Hello!\n");

	for (uint32_t r = SCU_BASE + 0xB8; r < SCU_BASE + 0xF4; r += 4) {
		printf("%08X\n", r);
		MMIO32(r) = MOD_SRC_CLRR | MOD_SRC_SRE;
	}

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
			stopwatch_msleep_wd(20);
			
			if (last_exti != -1) {
				printf("  GPIO_%d -> EXT%d\n", i, last_exti);
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
	int irqn = VIC_CURRENT_IRQ;
	
	for (uint32_t i = 0; i < ARRAY_SIZE(exti_regs); i++) {
		if (irqn == exti_regs[i].irq) {
			last_exti = i;
			*(exti_regs[i].reg) = MOD_SRC_CLRR;
			*(exti_regs[i].reg) = MOD_SRC_SRE;
		}
	}
	
	VIC_IRQ_ACK = 1;
}
