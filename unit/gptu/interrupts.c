#include <pmb887x.h>

#include "test.h"

#define GPTU_EVENT_TIMEOUT_MS 100
#define GPTU_IRQ_TIMEOUT_MS 100
#define GPTU_T2_RC_RELOAD_DIRECTIONAL 6
#define GPTU_REPEAT_COUNTS 260000
#define GPTU_REPEAT_RELOAD (0xFFFFFFFFu - GPTU_REPEAT_COUNTS + 1u)
#define GPTU_REPEAT_IRQS 3

static volatile uint32_t gptu0_src6_irqs;
static volatile uint32_t gptu0_src7_irqs;
static volatile uint32_t gptu0_other_irqs;
static volatile bool gptu0_src6_repeat;

static bool wait_mask_equal(volatile uint32_t *reg, uint32_t mask, uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((*reg & mask) == value)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static void clear_gptu0_sources(void) {
	for (uint32_t index = 0; index < 8; index++)
		GPTU_SRC(GPTU0, index) = MOD_SRC_CLRR;
}

static uint32_t gptu0_pending_mask(void) {
	uint32_t mask = 0;

	for (uint32_t index = 0; index < 8; index++) {
		if ((GPTU_SRC(GPTU0, index) & MOD_SRC_SRR) != 0)
			mask |= BIT(index);
	}

	return mask;
}

static void configure_gptu0_pending_request(void) {
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_T01IRS(GPTU0) = 0;
	GPTU_T01OTS(GPTU0) = GPTU_T01OTS_SSR00_A;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR7_SR00;
	GPTU_T0RDCBA(GPTU0) = 0;
	GPTU_T0DCBA(GPTU0) = 0xF0;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T0ARUN;
}

static void disable_gptu0(void) {
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_T01IRS(GPTU0) = 0;
	GPTU_T01OTS(GPTU0) = 0;
	GPTU_SRSEL(GPTU0) = 0;
	GPTU_T0RDCBA(GPTU0) = 0;
	GPTU_T0DCBA(GPTU0) = 0;
	GPTU_CLC(GPTU0) = MOD_CLC_DISR;
}

static bool wait_gptu0_source(uint32_t index) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < GPTU_EVENT_TIMEOUT_MS) {
		if ((GPTU_SRC(GPTU0, index) & MOD_SRC_SRR) != 0)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool wait_gptu0_irq(volatile uint32_t *count) {
	stopwatch_t start = stopwatch_get();

	while (*count == 0 && stopwatch_elapsed_ms(start) < GPTU_IRQ_TIMEOUT_MS)
		test_watchdog_serve();

	return *count != 0;
}

static void wait_gptu0_irqs(volatile uint32_t *count, uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while (*count < expected && stopwatch_elapsed_ms(start) < GPTU_IRQ_TIMEOUT_MS)
		test_watchdog_serve();
}

static void test_t2a_repeated_interrupt(void) {
	uint32_t saved_vic = VIC_CON(VIC_GPTU0_SRC6_IRQ);
	bool irq_was_disabled = cpu_enable_irq(false);

	test_category("GPTU0 T2A repeated interrupt");
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_T2CON(GPTU0) = 0;
	GPTU_T2RCCON(GPTU0) = GPTU_T2_RC_RELOAD_DIRECTIONAL << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2RC0(GPTU0) = GPTU_REPEAT_RELOAD;
	GPTU_T2(GPTU0) = GPTU_REPEAT_RELOAD;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR6_OUV_T2A;
	GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | MOD_SRC_SRE;
	GPTU_SRC(GPTU0, 7) = MOD_SRC_CLRR;
	VIC_CON(VIC_GPTU0_SRC6_IRQ) = 1;
	gptu0_src6_irqs = 0;
	gptu0_src7_irqs = 0;
	gptu0_other_irqs = 0;
	gptu0_src6_repeat = true;
	cpu_enable_irq(true);
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ASETR;

	wait_gptu0_irqs(&gptu0_src6_irqs, GPTU_REPEAT_IRQS);
	cpu_enable_irq(false);
	gptu0_src6_repeat = false;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ACLRR;
	GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR;
	uint32_t pending = GPTU_SRC(GPTU0, 6) & MOD_SRC_SRR;
	VIC_CON(VIC_GPTU0_SRC6_IRQ) = saved_vic;
	cpu_enable_irq(!irq_was_disabled);

	test_eq_u32("T2A raises exactly three SRC6 IRQs", GPTU_REPEAT_IRQS, gptu0_src6_irqs);
	test_eq_u32("T2A SRC6 routing raises no other IRQ", 0, gptu0_other_irqs);
	test_eq_u32("T2A SRC6 request is clear after stop", 0, pending);
}

static void test_pending_request_survives_disable(void) {
	const uint32_t source_config = MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE;
	uint32_t saved_vic6 = VIC_CON(VIC_GPTU0_SRC6_IRQ);
	uint32_t saved_vic7 = VIC_CON(VIC_GPTU0_SRC7_IRQ);
	uint32_t saved_src6 = GPTU_SRC(GPTU0, 6) & source_config;
	uint32_t saved_src7 = GPTU_SRC(GPTU0, 7) & source_config;
	bool irq_was_disabled = cpu_enable_irq(false);

	test_category("GPTU0 pending request after module disable");
	clear_gptu0_sources();
	configure_gptu0_pending_request();
	test_check("T0A overflow reaches SRC7", wait_gptu0_source(7));
	uint32_t pending_before_disable = gptu0_pending_mask();
	test_eq_u32("T0A overflow only raises its routed SR00 request", BIT(7), pending_before_disable);
	test_eq_u32("SRC7 SRE remains disabled", 0, GPTU_SRC(GPTU0, 7) & MOD_SRC_SRE);

	disable_gptu0();
	test_check("CLC.DISR disables GPTU0", wait_mask_equal(&GPTU_CLC(GPTU0), MOD_CLC_DISS, MOD_CLC_DISS));
	uint32_t pending_after_disable = gptu0_pending_mask();
	printf("# GPTU0 pending mask: before CLC.DISR=%02X after=%02X\n",
		(unsigned int) pending_before_disable, (unsigned int) pending_after_disable);
	test_eq_u32("only SRC7 pending request survives CLC.DISR", BIT(7), pending_after_disable);

	VIC_CON(VIC_GPTU0_SRC6_IRQ) = 1;
	gptu0_src6_irqs = 0;
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_SRC(GPTU0, 6) = MOD_SRC_SRE;
	cpu_enable_irq(true);
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_eq_u32("enabling SRC6 does not raise stale START_A IRQ93", 0, gptu0_src6_irqs);

	VIC_CON(VIC_GPTU0_SRC7_IRQ) = 1;
	gptu0_src7_irqs = 0;
	GPTU_SRC(GPTU0, 7) = MOD_SRC_SRE;
	cpu_enable_irq(true);
	test_check("Surviving SRC7 request raises IRQ92", wait_gptu0_irq(&gptu0_src7_irqs));
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_eq_u32("Surviving request raises exactly one IRQ92", 1, gptu0_src7_irqs);
	test_eq_u32("IRQ handler clears surviving request", 0, GPTU_SRC(GPTU0, 7) & MOD_SRC_SRR);

	clear_gptu0_sources();
	GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | saved_src6;
	GPTU_SRC(GPTU0, 7) = MOD_SRC_CLRR | saved_src7;
	VIC_CON(VIC_GPTU0_SRC6_IRQ) = saved_vic6;
	VIC_CON(VIC_GPTU0_SRC7_IRQ) = saved_vic7;
	disable_gptu0();
	cpu_enable_irq(!irq_was_disabled);
}

int main(void) {
	test_start("GPTU interrupt test");
	test_t2a_repeated_interrupt();
	test_pending_request_survives_disable();

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_GPTU0_SRC6_IRQ) {
		gptu0_src6_irqs++;
		if (gptu0_src6_repeat && gptu0_src6_irqs < GPTU_REPEAT_IRQS) {
			GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | MOD_SRC_SRE;
		} else {
			GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ACLRR;
			GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR;
		}
	} else if (irq == VIC_GPTU0_SRC7_IRQ) {
		gptu0_src7_irqs++;
		GPTU_SRC(GPTU0, 7) = MOD_SRC_CLRR;
	} else {
		gptu0_other_irqs++;
	}
	VIC_IRQ_ACK = 1;
}
