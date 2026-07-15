#include <pmb887x.h>

#include "test.h"

#ifdef PMB8876

#define SIM_IRQ_MASK (SIM_IMSC_ERR | SIM_IMSC_IN | SIM_IMSC_OK)
#define SIM_EVENT_ENABLE_MASK ( \
	SIM_IRQEN_ENOKINT | SIM_IRQEN_ENPAR | SIM_IRQEN_ENOVR | SIM_IRQEN_ENT0END | \
	SIM_IRQEN_ENCHTIMER | SIM_IRQEN_ENBWTTIMER | SIM_IRQEN_UNK6 \
)

struct irq_event {
	const char *name;
	uint32_t bit;
	uint32_t irq;
};

static const struct irq_event IRQ_EVENTS[] = {
	{"error", SIM_ISR_ERR, VIC_SIM_ERR_IRQ},
	{"presence", SIM_ISR_IN, VIC_SIM_IN_IRQ},
	{"character complete", SIM_ISR_OK, VIC_SIM_OK_IRQ},
};

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count != 0;
}

static void test_reset_values(void) {
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, SIM_CLC);
	test_module_id("ID", 0xF000C032, SIM_ID);

	SIM_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("CLC enables module", SIM_CLC);
	test_eq_u32("CON reset value", 0, SIM_CON);
	test_eq_u32("BRF reset value", 0x5D, SIM_BRF);
	/* SIMDET reflects the external card-presence input and is not a deterministic reset flag. */
	test_eq_u32("STAT deterministic reset flags", 0, SIM_STAT & ~SIM_STAT_SIMDET);
	test_eq_u32("IRQEN reset value", 0, SIM_IRQEN);
	test_eq_u32("RXSPC reset value", 0x28, SIM_RXSPC);
	test_eq_u32("TXSPC reset value", 0, SIM_TXSPC);
	test_eq_u32("CHTIMER reset value", 0x2580, SIM_CHTIMER);
	test_eq_u32("UNK3C reset value", 0, SIM_UNK3C);
	test_eq_u32("UNK40 reset value", 0, SIM_UNK40);
	test_eq_u32("BWT reset value", 0x3C0B, SIM_BWT);
	test_eq_u32("TXB reset value", 0, SIM_TXB);
	test_eq_u32("RXB reset value", 0, SIM_RXB);
	test_eq_u32("INS reset value", 0, SIM_INS);
	test_eq_u32("P3 reset value", 0, SIM_P3);
	test_eq_u32("SW1 reset value", 0, SIM_SW1);
	test_eq_u32("SW2 reset value", 0, SIM_SW2);
	test_eq_u32("IMSC reset value", 0, SIM_IMSC);
	test_eq_u32("RIS reset value", 0, SIM_RIS);
	test_eq_u32("MIS reset value", 0, SIM_MIS);
	test_eq_u32("ICR is write-only after reset", 0, SIM_ICR);
	test_eq_u32("ISR is write-only after reset", 0, SIM_ISR);
	test_eq_u32("DMAE reset value", 0, SIM_DMAE);
}

static void test_interrupt_block(void) {
	VIC_CON(VIC_SIM_ERR_IRQ) = 1;
	VIC_CON(VIC_SIM_IN_IRQ) = 1;
	VIC_CON(VIC_SIM_OK_IRQ) = 1;
	SIM_IMSC = 0;
	SIM_ICR = SIM_IRQ_MASK;

	for (size_t i = 0; i < ARRAY_SIZE(IRQ_EVENTS); i++) {
		const struct irq_event *event = &IRQ_EVENTS[i];

		SIM_ISR = event->bit;
		test_eq_u32("ISR is write-only", 0, SIM_ISR);
		test_eq_u32("ISR sets selected RIS bit", event->bit, SIM_RIS & SIM_IRQ_MASK);
		test_eq_u32("Masked event does not set MIS", 0, SIM_MIS & SIM_IRQ_MASK);
		SIM_IMSC = event->bit;
		test_eq_u32("IMSC exposes pending event in MIS", event->bit, SIM_MIS & SIM_IRQ_MASK);
		SIM_IMSC = 0;

		SIM_ISR = SIM_IRQ_MASK;
		SIM_ICR = event->bit;
		test_eq_u32("ICR is write-only", 0, SIM_ICR);
		test_eq_u32(
			"ICR clears only selected RIS bit",
			SIM_IRQ_MASK & ~event->bit,
			SIM_RIS & SIM_IRQ_MASK
		);
		SIM_ICR = SIM_IRQ_MASK;

		irq_count = 0;
		irq_number = 0;
		SIM_ISR = event->bit;
		SIM_IMSC = event->bit;
		cpu_enable_irq(true);
		bool arrived = wait_for_irq();
		cpu_enable_irq(false);
		test_check(event->name, arrived);
		test_eq_u32("Event routes to expected VIC IRQ", event->irq, irq_number);
		test_eq_u32("IRQ handler clears raw status", 0, SIM_RIS & SIM_IRQ_MASK);
		test_eq_u32("IRQ handler clears masked status", 0, SIM_MIS & SIM_IRQ_MASK);
		SIM_IMSC = 0;
	}

	VIC_CON(VIC_SIM_ERR_IRQ) = 0;
	VIC_CON(VIC_SIM_IN_IRQ) = 0;
	VIC_CON(VIC_SIM_OK_IRQ) = 0;
}

int main(void) {
	test_start("SIM");

	test_category("Reset values");
	test_reset_values();
	test_category("Interrupt block");
	test_interrupt_block();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_count++;
	SIM_ICR = SIM_IRQ_MASK;
	VIC_IRQ_ACK = 1;
}

#else

int main(void) {
	test_start("SIM");
	test_skip("SIM", "is only available on PMB8876");

	return test_finish();
}

#endif
