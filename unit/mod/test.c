#include <pmb887x.h>

#include "test.h"

#define USART_IRQ_MASK 0xFF

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t irq_order[2];

static bool wait_for_irq_count(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while (irq_count < expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count == expected;
}

static void test_usart_clc(void) {
	uint32_t saved = USART_CLC(USART1);

	USART_CLC(USART1) = 0x33 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("USART1 CLC stores RMC", 0x33 << MOD_CLC_RMC_SHIFT, USART_CLC(USART1) & MOD_CLC_RMC);
	test_check("USART1 CLC enables module", (USART_CLC(USART1) & (MOD_CLC_DISR | MOD_CLC_DISS)) == 0);

	USART_CLC(USART1) = (0xCC << MOD_CLC_RMC_SHIFT) | MOD_CLC_DISR;
	test_eq_u32(
		"USART1 CLC stores disabled RMC",
		0xCC << MOD_CLC_RMC_SHIFT,
		USART_CLC(USART1) & MOD_CLC_RMC
	);
	test_check(
		"USART1 CLC acknowledges disable",
		(USART_CLC(USART1) & (MOD_CLC_DISR | MOD_CLC_DISS)) == (MOD_CLC_DISR | MOD_CLC_DISS)
	);

	USART_CLC(USART1) = saved;
	test_eq_u32("USART1 CLC restores state", saved, USART_CLC(USART1));
}

static void test_src(void) {
	uint32_t saved = SCU_EXTI0_SRC;

	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	SCU_EXTI0_SRC = MOD_SRC_SRE;
	test_check("SRC stores SRE", (SCU_EXTI0_SRC & MOD_SRC_SRE) != 0);
	test_check("SRC starts without request", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);

	SCU_EXTI0_SRC |= MOD_SRC_SETR;
	test_check("SRC SETR sets SRR", (SCU_EXTI0_SRC & MOD_SRC_SRR) != 0);
	test_check("SRC SETR is write-only", (SCU_EXTI0_SRC & MOD_SRC_SETR) == 0);
	SCU_EXTI0_SRC |= MOD_SRC_CLRR;
	test_check("SRC CLRR clears SRR", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);
	test_check("SRC CLRR is write-only", (SCU_EXTI0_SRC & MOD_SRC_CLRR) == 0);
	SCU_EXTI0_SRC = MOD_SRC_SRR;
	test_check("SRC ignores direct SRR write", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);
	SCU_EXTI0_SRC = MOD_SRC_SETR | MOD_SRC_CLRR;
	test_check("SRC CLRR wins when SETR and CLRR are written together", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);
	SCU_EXTI0_SRC = MOD_SRC_CLRR;

	irq_count = 0;
	irq_number = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 1;
	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	cpu_enable_irq(true);
	SCU_EXTI0_SRC = MOD_SRC_SETR;
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_check("SRC keeps request pending while SRE is clear", (SCU_EXTI0_SRC & MOD_SRC_SRR) != 0);
	test_eq_u32("SRC with SRE clear does not raise IRQ", 0, irq_count);

	cpu_enable_irq(true);
	SCU_EXTI0_SRC |= MOD_SRC_SRE;
	test_check("SRC pending request raises IRQ when SRE is enabled", wait_for_irq_count(1));
	cpu_enable_irq(false);
	test_check("SRC pending IRQ handler clears SRR", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);

	irq_count = 0;
	irq_number = 0;
	SCU_EXTI0_SRC = 1 | MOD_SRC_SRE | MOD_SRC_SETR;
	SCU_EXTI0_SRC &= ~MOD_SRC_SRE;
	test_check("SRC keeps SRR when SRE is cleared", (SCU_EXTI0_SRC & MOD_SRC_SRR) != 0);
	cpu_enable_irq(true);
	test_check("SRC clearing SRE does not withdraw asserted IRQ", wait_for_irq_count(1));
	cpu_enable_irq(false);
	test_check("SRC asserted IRQ handler clears SRR", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);
	SCU_EXTI0_SRC = MOD_SRC_CLRR;

	irq_count = 0;
	irq_number = 0;
	SCU_EXTI0_SRC = 1 | MOD_SRC_SRE | MOD_SRC_CLRR;
	cpu_enable_irq(true);
	SCU_EXTI0_SRC |= MOD_SRC_SETR;
	test_check("SRC software request raises IRQ", wait_for_irq_count(1));
	cpu_enable_irq(false);
	test_eq_u32("SRC IRQ is routed to VIC", VIC_SCU_EXTI0_IRQ, irq_number);
	test_check("SRC IRQ handler clears SRR", (SCU_EXTI0_SRC & MOD_SRC_SRR) == 0);

	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	SCU_EXTI0_SRC = saved | MOD_SRC_CLRR;
}

static void test_srb(void) {
	uint32_t saved_clc = USART_CLC(USART1);

	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32(
		"USART1 is enabled for SRB test",
		1 << MOD_CLC_RMC_SHIFT,
		USART_CLC(USART1) & (MOD_CLC_RMC | MOD_CLC_DISR | MOD_CLC_DISS)
	);
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_IMSC(USART1) = 0;
	test_eq_u32("SRB IMSC clears", 0, USART_IMSC(USART1) & USART_IRQ_MASK);
	test_eq_u32("SRB RIS clears", 0, USART_RIS(USART1) & USART_IRQ_MASK);
	test_eq_u32("SRB MIS clears", 0, USART_MIS(USART1) & USART_IRQ_MASK);

	USART_ISR(USART1) = USART_ISR_TX | USART_ISR_RX | USART_ISR_CTS;
	test_eq_u32("SRB ISR is write-only", 0, USART_ISR(USART1));
	test_eq_u32(
		"SRB ISR sets raw status",
		USART_RIS_TX | USART_RIS_RX | USART_RIS_CTS,
		USART_RIS(USART1) & USART_IRQ_MASK
	);
	test_eq_u32("SRB masked status stays clear", 0, USART_MIS(USART1) & USART_IRQ_MASK);

	USART_IMSC(USART1) = USART_IMSC_TX | USART_IMSC_CTS;
	test_eq_u32(
		"SRB IMSC readback",
		USART_IMSC_TX | USART_IMSC_CTS,
		USART_IMSC(USART1) & USART_IRQ_MASK
	);
	test_eq_u32(
		"SRB MIS is RIS masked by IMSC",
		USART_MIS_TX | USART_MIS_CTS,
		USART_MIS(USART1) & USART_IRQ_MASK
	);

	USART_ICR(USART1) = USART_ICR_TX;
	test_eq_u32("SRB ICR is write-only", 0, USART_ICR(USART1));
	test_eq_u32(
		"SRB ICR clears selected raw status",
		USART_RIS_RX | USART_RIS_CTS,
		USART_RIS(USART1) & USART_IRQ_MASK
	);
	test_eq_u32("SRB ICR updates masked status", USART_MIS_CTS, USART_MIS(USART1) & USART_IRQ_MASK);

	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_IMSC(USART1) = 0;
	irq_count = 0;
	irq_number = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 1;
	cpu_enable_irq(true);
	USART_ISR(USART1) = USART_ISR_TX;
	stopwatch_usleep_wd(1000);
	test_eq_u32("SRB masked pending event does not raise IRQ", 0, irq_count);
	test_check("SRB masked event remains raw", (USART_RIS(USART1) & USART_RIS_TX) != 0);
	USART_IMSC(USART1) = USART_IMSC_TX;
	test_check("SRB pending event raises IRQ when mask is enabled", wait_for_irq_count(1));
	cpu_enable_irq(false);
	test_eq_u32("SRB pending IRQ is routed to VIC", VIC_USART1_TX_IRQ, irq_number);
	test_check("SRB pending IRQ handler clears raw status", (USART_RIS(USART1) & USART_RIS_TX) == 0);

	irq_count = 0;
	irq_number = 0;
	USART_IMSC(USART1) = USART_IMSC_TX;
	USART_ISR(USART1) = USART_ISR_TX;
	USART_IMSC(USART1) = 0;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_check("SRB keeps RIS when mask is cleared", (USART_RIS(USART1) & USART_RIS_TX) != 0);
	test_eq_u32("SRB clearing mask withdraws IRQ", 0, irq_count);
	test_eq_u32("SRB clearing mask removes MIS", 0, USART_MIS(USART1) & USART_MIS_TX);
	USART_ICR(USART1) = USART_ICR_TX;

	USART_ISR(USART1) = USART_ISR_TX;
	USART_ISR(USART1) = USART_ISR_TX;
	test_check("SRB repeated ISR keeps event pending", (USART_RIS(USART1) & USART_RIS_TX) != 0);
	USART_ICR(USART1) = USART_ICR_TX;
	test_check("SRB one ICR clears repeated ISR", (USART_RIS(USART1) & USART_RIS_TX) == 0);

	irq_count = 0;
	irq_number = 0;
	irq_order[0] = 0;
	irq_order[1] = 0;
	USART_IMSC(USART1) = USART_IMSC_TX | USART_IMSC_RX;
	VIC_CON(VIC_USART1_RX_IRQ) = 1;
	cpu_enable_irq(true);
	USART_ISR(USART1) = USART_ISR_TX | USART_ISR_RX;
	test_check("SRB independent TX and RX IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("SRB TX IRQ arrives first", VIC_USART1_TX_IRQ, irq_order[0]);
	test_eq_u32("SRB RX IRQ arrives second", VIC_USART1_RX_IRQ, irq_order[1]);
	test_eq_u32(
		"SRB TX and RX handlers clear raw status",
		0,
		USART_RIS(USART1) & (USART_RIS_TX | USART_RIS_RX)
	);

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_USART1_RX_IRQ) = 0;
	USART_IMSC(USART1) = 0;
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_CLC(USART1) = saved_clc;
}

int main(void) {
	test_start("Common module blocks test");

	test_category("CLC");
	test_usart_clc();

	test_category("SRC");
	test_src();

	test_category("SRB");
	test_srb();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	if (irq_count < ARRAY_SIZE(irq_order))
		irq_order[irq_count] = irq_number;
	irq_count++;

	if (irq_number == VIC_SCU_EXTI0_IRQ)
		SCU_EXTI0_SRC |= MOD_SRC_CLRR;
	else if (irq_number == VIC_USART1_TX_IRQ)
		USART_ICR(USART1) = USART_ICR_TX;
	else if (irq_number == VIC_USART1_RX_IRQ)
		USART_ICR(USART1) = USART_ICR_RX;

	VIC_IRQ_ACK = 1;
}
