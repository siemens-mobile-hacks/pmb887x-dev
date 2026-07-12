#include <pmb887x.h>

#include "test.h"

#define USART_IRQ_MASK 0xFF

static volatile uint32_t irq_count;
static volatile uint32_t irq_order[2];
static volatile uint32_t irq_status[2];
static volatile uint32_t fiq_count;
static volatile uint32_t fiq_number;
static volatile uint32_t fiq_status;

static bool wait_for_irq_count(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while (irq_count < expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count == expected;
}

static bool wait_for_fiq(void) {
	stopwatch_t start = stopwatch_get();

	while (fiq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return fiq_count != 0;
}

static void clear_sources(void) {
	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	USART_ICR(USART1) = USART_IRQ_MASK;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
}

static void reset_irq_capture(void) {
	irq_count = 0;
	irq_order[0] = 0;
	irq_order[1] = 0;
	irq_status[0] = 0;
	irq_status[1] = 0;
}

static void test_registers(void) {
	test_module_id("module ID", 0x0031C000, VIC_ID);
	test_eq_u32("IRQ_CON is clear without pending sources", 0, VIC_IRQ_CON);
	test_eq_u32("FIQ_CON is clear without pending sources", 0, VIC_FIQ_CON);
	test_eq_u32("IRQ_CURRENT is clear without pending sources", 0, VIC_IRQ_CURRENT);
	test_eq_u32("FIQ_CURRENT is clear without pending sources", 0, VIC_FIQ_CURRENT);
	VIC_IRQ_ACK = 1;
	VIC_FIQ_ACK = 1;

	VIC_CON(VIC_USART1_TX_IRQ) = 5;
	test_eq_u32("IRQ priority reads back", 5, VIC_CON(VIC_USART1_TX_IRQ) & VIC_CON_PRIORITY);
	VIC_CON(VIC_USART1_TX_IRQ) = 12 | VIC_CON_FIQ;
	test_eq_u32(
		"FIQ route and priority read back",
		12 | VIC_CON_FIQ,
		VIC_CON(VIC_USART1_TX_IRQ) & (VIC_CON_PRIORITY | VIC_CON_FIQ)
	);
	VIC_CON(VIC_USART1_TX_IRQ) = 0;

	VIC_CON(0) = 15 | VIC_CON_FIQ;
	test_eq_u32("CON0 decodes priority and FIQ", 15 | VIC_CON_FIQ, VIC_CON(0) & (VIC_CON_PRIORITY | VIC_CON_FIQ));
	VIC_CON(VIC_IRQ_COUNT - 1) = 15 | VIC_CON_FIQ;
	test_eq_u32(
		"last VIC_CON decodes priority and FIQ",
		15 | VIC_CON_FIQ,
		VIC_CON(VIC_IRQ_COUNT - 1) & (VIC_CON_PRIORITY | VIC_CON_FIQ)
	);
	VIC_CON(0) = 0;
	VIC_CON(VIC_IRQ_COUNT - 1) = 0;
}

static void test_priority_boundaries(void) {
	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(1000);
	test_eq_u32("priority zero does not raise an IRQ", 0, irq_count);
	test_check("priority-zero source remains pending", (SCU_EXTI0_SRC & MOD_SRC_SRR) != 0);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 1;
	test_check("priority one releases pending source", wait_for_irq_count(1));
	cpu_enable_irq(false);

	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 15;
	VIC_IRQ_CON = 15 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(1000);
	test_eq_u32("maximum mask blocks maximum priority", 0, irq_count);
	VIC_IRQ_CON = 14 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	test_check("mask below maximum releases priority 15", wait_for_irq_count(1));
	cpu_enable_irq(false);

	VIC_IRQ_CON = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_pending_reconfiguration(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 2;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 3;
	USART_ISR(USART1) = USART_ISR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("initial pending winner uses configured priority", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	VIC_CON(VIC_USART1_TX_IRQ) = 4;
	test_eq_u32("pending winner updates after priority change", VIC_USART1_TX_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);

	clear_sources();
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_irq_arrival_before_ack(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 2;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 7;
	USART_ISR(USART1) = USART_ISR_TX;
	test_eq_u32("low IRQ starts current frame", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	USART_ICR(USART1) = USART_ICR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("high IRQ becomes pending before low ACK", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	VIC_IRQ_ACK = 1;
	test_eq_u32("low ACK preserves newly pending high IRQ", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	test_eq_u32("next current frame selects high IRQ", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);

	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_multiple_current_frames(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 2;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 7;
	USART_ISR(USART1) = USART_ISR_TX;
	test_eq_u32("first CURRENT opens low-priority frame", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	test_eq_u32(
		"low-priority frame sets priority mask",
		2 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT,
		VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY
	);
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("second CURRENT opens high-priority frame", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);
	test_eq_u32(
		"high-priority frame replaces active mask",
		7 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT,
		VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY
	);
	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;
	test_eq_u32(
		"high frame ACK restores low frame mask",
		2 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT,
		VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY
	);
	USART_ICR(USART1) = USART_ICR_TX;
	VIC_IRQ_ACK = 1;
	test_eq_u32("low frame ACK clears priority mask", 0, VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY);

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_current_frame_depth(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	bool opened = true;
	for (uint32_t priority = 1; priority <= 15; priority++) {
		VIC_CON(VIC_SCU_EXTI0_IRQ) = priority;
		opened &= VIC_IRQ_CURRENT == VIC_SCU_EXTI0_IRQ;
		opened &= (VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY) == priority << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	}
	test_check("CURRENT opens frames for priorities 1 through 15", opened);
	test_eq_u32("priority 15 blocks a sixteenth frame", 0, VIC_IRQ_CURRENT);

	bool restored = true;
	for (uint32_t priority = 14; priority > 0; priority--) {
		VIC_IRQ_ACK = 1;
		restored &= (VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY) == priority << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	}
	VIC_IRQ_ACK = 1;
	restored &= (VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY) == 0;
	test_check("fifteen ACKs restore all frame masks", restored);

	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_active_frame_state(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 4;
	USART_ISR(USART1) = USART_ISR_TX;
	test_eq_u32("active-state test opens IRQ frame", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	VIC_CON(VIC_USART1_TX_IRQ) = 9 | VIC_CON_FIQ;
	test_eq_u32(
		"active frame keeps captured priority after reroute",
		4 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT,
		VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY
	);
	USART_ICR(USART1) = USART_ICR_TX;
	test_eq_u32(
		"active frame keeps mask after source deassert",
		4 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT,
		VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY
	);
	VIC_IRQ_ACK = 1;
	test_eq_u32("active frame ACK clears captured priority", 0, VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY);

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
}

static void test_priority_disable_pending(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 3;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("priority-disable source starts pending", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	test_eq_u32("priority zero removes source from VIC selection", 0, VIC_IRQ_CON);
	test_check("priority zero preserves peripheral request", (SCU_EXTI0_SRC & MOD_SRC_SRR) != 0);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 1;
	test_eq_u32("restored priority returns source to selection", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);

	clear_sources();
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_ack_underflow(void) {
	cpu_enable_irq(false);
	clear_sources();
	for (unsigned int i = 0; i < 20; i++) {
		VIC_IRQ_ACK = 1;
		VIC_FIQ_ACK = 1;
	}
	test_eq_u32("IRQ ACK underflow keeps IRQ mask clear", 0, VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY);
	test_eq_u32("FIQ ACK underflow keeps FIQ mask clear", 0, VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 2;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("IRQ works after ACK underflow", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_reserved_bits(void) {
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0xFFFFFFFF;
	test_eq_u32(
		"VIC_CON ignores reserved bits",
		15 | VIC_CON_FIQ,
		VIC_CON(VIC_SCU_EXTI0_IRQ) & 0xFFFFFFFF
	);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_pending_and_current(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 2;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 3;
	USART_ISR(USART1) = USART_ISR_TX;

	test_eq_u32("IRQ_CON reports pending IRQ", VIC_USART1_TX_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	test_eq_u32(
		"IRQ_CON reports pending priority",
		2,
		(VIC_IRQ_CON & VIC_IRQ_CON_PRIORITY) >> VIC_IRQ_CON_PRIORITY_SHIFT
	);
	test_eq_u32("IRQ_CURRENT returns pending IRQ", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("higher-priority IRQ preempts current selection", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);
	VIC_IRQ_ACK = 0;
	test_eq_u32("zero acknowledge keeps current selection", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);
	VIC_IRQ_ACK = 1;
	test_eq_u32("acknowledge does not hide pending source", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CURRENT);

	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_ACK = 1;
	VIC_IRQ_ACK = 1;
	VIC_IRQ_ACK = 1;
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_priority(void) {
	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_USART1_TX_IRQ) = 7;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 3;
	USART_ISR(USART1) = USART_ISR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	test_check("both priority IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("higher priority IRQ arrives first", VIC_USART1_TX_IRQ, irq_order[0]);
	test_eq_u32("lower priority IRQ arrives second", VIC_SCU_EXTI0_IRQ, irq_order[1]);
	test_eq_u32(
		"handler sees selected IRQ priority",
		7,
		(irq_status[0] & VIC_IRQ_CON_PRIORITY) >> VIC_IRQ_CON_PRIORITY_SHIFT
	);

	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_USART1_TX_IRQ) = 4;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 4;
	USART_ISR(USART1) = USART_ISR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	test_check("both equal-priority IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("lower IRQ number wins equal priority", VIC_USART1_TX_IRQ, irq_order[0]);
	test_eq_u32("higher IRQ number follows equal priority", VIC_SCU_EXTI0_IRQ, irq_order[1]);

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_priority_mask(void) {
	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 5;
	VIC_IRQ_CON = 5 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(1000);
	test_eq_u32("priority mask blocks equal priority", 0, irq_count);

	VIC_IRQ_CON = 4 << VIC_IRQ_CON_MASK_PRIORITY_SHIFT;
	test_check("lower priority mask releases pending IRQ", wait_for_irq_count(1));
	cpu_enable_irq(false);
	test_eq_u32("released IRQ is routed correctly", VIC_SCU_EXTI0_IRQ, irq_order[0]);
	test_eq_u32("IRQ acknowledge clears priority mask", 0, VIC_IRQ_CON & VIC_IRQ_CON_MASK_PRIORITY);

	VIC_IRQ_CON = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_src_priority(void) {
	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_TPU_INT0_IRQ) = 5;
	VIC_CON(VIC_TPU_INT1_IRQ) = 5;
	TPU_SRC(0) = 2 | MOD_SRC_SRE | MOD_SRC_SETR;
	TPU_SRC(1) = 9 | MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	test_check("both SRC-priority IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("lower IRQ number wins despite SRC priority", VIC_TPU_INT0_IRQ, irq_order[0]);
	test_eq_u32("higher IRQ number follows despite SRC priority", VIC_TPU_INT1_IRQ, irq_order[1]);

	clear_sources();
	reset_irq_capture();
	TPU_SRC(0) = 9 | MOD_SRC_SRE | MOD_SRC_SETR;
	TPU_SRC(1) = 2 | MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	test_check("both reversed SRC-priority IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("reversed SRC priority still keeps lower IRQ first", VIC_TPU_INT0_IRQ, irq_order[0]);
	test_eq_u32("reversed SRC priority still keeps higher IRQ second", VIC_TPU_INT1_IRQ, irq_order[1]);

	clear_sources();
	reset_irq_capture();
	VIC_CON(VIC_TPU_INT0_IRQ) = 8;
	VIC_CON(VIC_TPU_INT1_IRQ) = 3;
	TPU_SRC(0) = 1 | MOD_SRC_SRE | MOD_SRC_SETR;
	TPU_SRC(1) = 15 | MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(true);
	test_check("both mixed-priority IRQs arrive", wait_for_irq_count(2));
	cpu_enable_irq(false);
	test_eq_u32("VIC priority takes precedence over SRC priority", VIC_TPU_INT0_IRQ, irq_order[0]);
	test_eq_u32("lower VIC priority follows despite higher SRC priority", VIC_TPU_INT1_IRQ, irq_order[1]);

	VIC_CON(VIC_TPU_INT0_IRQ) = 0;
	VIC_CON(VIC_TPU_INT1_IRQ) = 0;
	clear_sources();
}

static void test_nested_src_priority(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 5;
	VIC_CON(VIC_TPU_INT1_IRQ) = 5;

	TPU_SRC(0) = 2 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("nested test enters lower SRC priority", VIC_TPU_INT0_IRQ, VIC_IRQ_CURRENT);
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = 9 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("SRC priority cannot preempt equal VIC priority", 0, VIC_IRQ_CURRENT);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_ACK = 1;

	TPU_SRC(0) = 9 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("nested test enters higher SRC priority", VIC_TPU_INT0_IRQ, VIC_IRQ_CURRENT);
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = 2 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("lower SRC priority cannot preempt equal VIC priority", 0, VIC_IRQ_CURRENT);
	clear_sources();
	VIC_IRQ_ACK = 1;

	VIC_CON(VIC_TPU_INT0_IRQ) = 0;
	VIC_CON(VIC_TPU_INT1_IRQ) = 0;
}

static void test_src_and_srb_priority(void) {
	cpu_enable_irq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 6;
	VIC_CON(VIC_TPU_INT0_IRQ) = 6;
	USART_ISR(USART1) = USART_ISR_TX;
	TPU_SRC(0) = 15 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("SRB wins equal VIC priority despite SRC SRPN", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	USART_ICR(USART1) = USART_ICR_TX;
	VIC_IRQ_ACK = 1;
	test_eq_u32("SRC follows equal-priority SRB", VIC_TPU_INT0_IRQ, VIC_IRQ_CURRENT);
	TPU_SRC(0) = MOD_SRC_CLRR;
	VIC_IRQ_ACK = 1;

	TPU_SRC(0) = 1 | MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("nested SRC test enters SRC", VIC_TPU_INT0_IRQ, VIC_IRQ_CURRENT);
	TPU_SRC(0) = MOD_SRC_CLRR;
	USART_ISR(USART1) = USART_ISR_TX;
	test_eq_u32("equal-priority SRB cannot preempt SRC", 0, VIC_IRQ_CURRENT);
	USART_ICR(USART1) = USART_ICR_TX;
	VIC_IRQ_ACK = 1;

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_TPU_INT0_IRQ) = 0;
	clear_sources();
}

static void test_fiq(void) {
	clear_sources();
	fiq_count = 0;
	fiq_number = 0;
	fiq_status = 0;
	VIC_FIQ_ACK = 1;
	VIC_FIQ_CON = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 6 | VIC_CON_FIQ;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_fiq(true);
	test_check("FIQ route raises FIQ", wait_for_fiq());
	cpu_enable_fiq(false);
	test_eq_u32("FIQ_CURRENT reports routed source", VIC_SCU_EXTI0_IRQ, fiq_number);
	test_eq_u32(
		"FIQ_CON reports source priority",
		6,
		(fiq_status & VIC_FIQ_CON_PRIORITY) >> VIC_FIQ_CON_PRIORITY_SHIFT
	);

	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	clear_sources();
}

static void test_fiq_mask(void) {
	clear_sources();
	fiq_count = 0;
	VIC_FIQ_ACK = 1;
	VIC_FIQ_CON = 6 << VIC_FIQ_CON_MASK_PRIORITY_SHIFT;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 6 | VIC_CON_FIQ;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_fiq(true);
	stopwatch_usleep_wd(1000);
	test_eq_u32("FIQ priority mask blocks equal priority", 0, fiq_count);
	VIC_FIQ_CON = 5 << VIC_FIQ_CON_MASK_PRIORITY_SHIFT;
	test_check("lower FIQ mask releases pending FIQ", wait_for_fiq());
	cpu_enable_fiq(false);
	test_eq_u32("FIQ acknowledge clears priority mask", 0, VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY);

	VIC_FIQ_CON = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	clear_sources();
}

static void test_fiq_frame_depth(void) {
	cpu_enable_fiq(false);
	clear_sources();
	VIC_FIQ_ACK = 1;
	VIC_FIQ_CON = 0;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	bool opened = true;
	for (uint32_t priority = 1; priority <= 15; priority++) {
		VIC_CON(VIC_SCU_EXTI0_IRQ) = priority | VIC_CON_FIQ;
		opened &= VIC_FIQ_CURRENT == VIC_SCU_EXTI0_IRQ;
		opened &= (VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY) == priority << VIC_FIQ_CON_MASK_PRIORITY_SHIFT;
	}
	test_check("FIQ_CURRENT opens frames for priorities 1 through 15", opened);
	test_eq_u32("FIQ priority 15 blocks a sixteenth frame", 0, VIC_FIQ_CURRENT);

	bool restored = true;
	for (uint32_t priority = 14; priority > 0; priority--) {
		VIC_FIQ_ACK = 1;
		restored &= (VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY) == priority << VIC_FIQ_CON_MASK_PRIORITY_SHIFT;
	}
	VIC_FIQ_ACK = 1;
	restored &= (VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY) == 0;
	test_check("fifteen FIQ ACKs restore all frame masks", restored);

	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_fiq_arrival_before_ack(void) {
	cpu_enable_fiq(false);
	clear_sources();
	VIC_FIQ_ACK = 1;
	VIC_FIQ_CON = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 2 | VIC_CON_FIQ;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 7 | VIC_CON_FIQ;
	USART_ISR(USART1) = USART_ISR_TX;
	test_eq_u32("low FIQ starts current frame", VIC_USART1_TX_IRQ, VIC_FIQ_CURRENT);
	USART_ICR(USART1) = USART_ICR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("high FIQ becomes pending before low ACK", VIC_SCU_EXTI0_IRQ, VIC_FIQ_CON & VIC_FIQ_CON_NUM);
	VIC_FIQ_ACK = 1;
	test_eq_u32("low FIQ ACK preserves newly pending high FIQ", VIC_SCU_EXTI0_IRQ, VIC_FIQ_CON & VIC_FIQ_CON_NUM);
	test_eq_u32("next FIQ frame selects high FIQ", VIC_SCU_EXTI0_IRQ, VIC_FIQ_CURRENT);

	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_FIQ_ACK = 1;
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_independent_frame_stacks(void) {
	cpu_enable_irq(false);
	cpu_enable_fiq(false);
	clear_sources();
	VIC_IRQ_ACK = 1;
	VIC_FIQ_ACK = 1;
	VIC_CON(VIC_USART1_TX_IRQ) = 3;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 6 | VIC_CON_FIQ;
	USART_ISR(USART1) = USART_ISR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("independent stack opens IRQ frame", VIC_USART1_TX_IRQ, VIC_IRQ_CURRENT);
	test_eq_u32("independent stack opens FIQ frame", VIC_SCU_EXTI0_IRQ, VIC_FIQ_CURRENT);
	VIC_IRQ_ACK = 1;
	test_eq_u32(
		"IRQ ACK preserves active FIQ frame",
		6 << VIC_FIQ_CON_MASK_PRIORITY_SHIFT,
		VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY
	);
	VIC_FIQ_ACK = 1;
	test_eq_u32("FIQ ACK clears FIQ frame independently", 0, VIC_FIQ_CON & VIC_FIQ_CON_MASK_PRIORITY);

	clear_sources();
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
}

static void test_irq_fiq_independence(void) {
	clear_sources();
	reset_irq_capture();
	fiq_count = 0;
	fiq_number = 0;
	VIC_IRQ_ACK = 1;
	VIC_FIQ_ACK = 1;
	VIC_CON(VIC_USART1_TX_IRQ) = 4;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 7 | VIC_CON_FIQ;
	USART_ISR(USART1) = USART_ISR_TX;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	cpu_enable_irq(false);
	cpu_enable_fiq(true);
	test_check("FIQ is serviced while IRQ is disabled", wait_for_fiq());
	test_eq_u32("simultaneous FIQ uses FIQ source", VIC_SCU_EXTI0_IRQ, fiq_number);
	test_eq_u32("IRQ remains pending after FIQ", VIC_USART1_TX_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	cpu_enable_irq(true);
	test_check("pending IRQ is serviced independently", wait_for_irq_count(1));
	cpu_enable_irq(false);
	cpu_enable_fiq(false);
	test_eq_u32("simultaneous IRQ uses IRQ source", VIC_USART1_TX_IRQ, irq_order[0]);

	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	clear_sources();
}

static void test_pending_reroute(void) {
	clear_sources();
	reset_irq_capture();
	fiq_count = 0;
	fiq_number = 0;
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 5;
	SCU_EXTI0_SRC = MOD_SRC_SRE | MOD_SRC_SETR;
	test_eq_u32("source starts pending as IRQ", VIC_SCU_EXTI0_IRQ, VIC_IRQ_CON & VIC_IRQ_CON_NUM);
	VIC_CON(VIC_SCU_EXTI0_IRQ) = 5 | VIC_CON_FIQ;
	test_eq_u32("rerouted source leaves IRQ pending state", 0, VIC_IRQ_CON);
	cpu_enable_fiq(true);
	test_check("pending source is delivered after reroute to FIQ", wait_for_fiq());
	cpu_enable_fiq(false);
	test_eq_u32("rerouted FIQ reports original source", VIC_SCU_EXTI0_IRQ, fiq_number);

	VIC_CON(VIC_SCU_EXTI0_IRQ) = 0;
	clear_sources();
}

int main(void) {
	test_start("VIC peripheral test");
	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	USART_IMSC(USART1) = USART_IMSC_TX;
	clear_sources();

	test_category("Registers");
	test_registers();

	test_category("Pending and acknowledge");
	test_pending_and_current();
	test_pending_reconfiguration();
	test_irq_arrival_before_ack();
	test_multiple_current_frames();
	test_current_frame_depth();
	test_active_frame_state();
	test_priority_disable_pending();
	test_ack_underflow();
	test_reserved_bits();

	test_category("Arbitration");
	test_priority();
	test_priority_boundaries();
	test_src_priority();
	test_nested_src_priority();
	test_src_and_srb_priority();
	test_priority_mask();

	test_category("FIQ");
	test_fiq();
	test_fiq_mask();
	test_fiq_frame_depth();
	test_fiq_arrival_before_ack();
	test_independent_frame_stacks();
	test_irq_fiq_independence();
	test_pending_reroute();

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t status = VIC_IRQ_CON;
	uint32_t number = VIC_IRQ_CURRENT;

	if (irq_count < ARRAY_SIZE(irq_order)) {
		irq_order[irq_count] = number;
		irq_status[irq_count] = status;
	}
	irq_count++;

	if (number == VIC_USART1_TX_IRQ)
		USART_ICR(USART1) = USART_ICR_TX;
	else if (number == VIC_SCU_EXTI0_IRQ)
		SCU_EXTI0_SRC = MOD_SRC_CLRR;
	else if (number == VIC_TPU_INT0_IRQ)
		TPU_SRC(0) = MOD_SRC_CLRR;
	else if (number == VIC_TPU_INT1_IRQ)
		TPU_SRC(1) = MOD_SRC_CLRR;

	VIC_IRQ_ACK = 1;
}

__IRQ void fiq_handler(void) {
	fiq_status = VIC_FIQ_CON;
	fiq_number = VIC_FIQ_CURRENT;
	fiq_count++;
	SCU_EXTI0_SRC = MOD_SRC_CLRR;
	VIC_FIQ_ACK = 1;
}
