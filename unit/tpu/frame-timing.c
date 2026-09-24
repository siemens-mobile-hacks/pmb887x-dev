#include <pmb887x.h>

#include "test.h"

/*
 * TPU frame timing — the numbers the GSM notes rest on, measured.
 *
 * `tpu/test.c` already establishes the counter clock and the compare/correction
 * machinery, and `tpu/baseband-clock.c` measures the baseband word clock against
 * the K/L divider. What neither asserts, and what `memory/ke970-gsm-stack.md`
 * states as fact, is:
 *
 *   1. one TDMA frame is exactly 10 000 ticks of the counter, i.e. a wrap of
 *      OVERFLOW = 9999 takes the 4.615 ms frame time;
 *   2. the frame rate is 2 166 666 Hz (13 MHz / 6);
 *   3. a 459-word / 153-event event table is a valid hardware table — EAPT
 *      accepts 459, the event machine really steps through all 153 events
 *      (CEAP walks 0 -> 456), and such a table does not disturb the frame;
 *   4. TPU_INT0 and TPU_INT1 are wired to VIC 119 and 120 respectively, in that
 *      order, and repeat once per frame.
 *
 * All four are register-programming facts, so this test needs no external
 * stimulus and is meaningful on silicon as well as under emulation. The
 * firmware values it mirrors are `gsmtu_start_frame_counter`'s: OVERFLOW=9999,
 * TGER=0x4001, an EAPT=459 table, and INT0 parked above OVERFLOW.
 */

#define TPU_COUNTER_FREQUENCY 2166666
#define TPU_TIMEOUT_MS 100

/* Firmware: 4.61538 ms frame * 2 166 666.7 Hz = 10 000.0 ticks (exact). */
#define TPU_FRAME_TICKS 10000
#define TPU_FRAME_US 4615

/* Compare raw counters over 10 ms and average frame periods across 16 wraps. */
#define TPU_TOLERANCE_PPM 1000
#define TPU_CROSSCHECK_PPM 1000
#define TPU_CROSSCHECK_US 10000
#define TPU_FRAME_PERIODS 16
/* TPU_CLC.RMC = 1, GSMCLK1.K = 1, GSMCLK2.L = 2 -> fcounter = fSYS / 12. */
#define TPU_CROSSCHECK_TPU_DIVISOR 12

/* Firmware: gsmtu_init_event_ram uploads 153 events of 3 words each. */
#define TPU_EVENT_COUNT 153
#define TPU_EVENT_WORDS_PER_ENTRY 3
#define TPU_EVENT_TABLE_WORDS (TPU_EVENT_COUNT * TPU_EVENT_WORDS_PER_ENTRY)
#define TPU_TIMER_RAM_BASE 512
#define TPU_EVENT_FIRST_TICK 100
#define TPU_EVENT_TICK_STRIDE 13
#define TPU_EVENT_LAST_OFFSET ((TPU_EVENT_COUNT - 1) * TPU_EVENT_WORDS_PER_ENTRY)

/* The two live compare lines the firmware uses (INT0 is parked above OVERFLOW). */
#define TPU_COMPARE0_TICK 2000
#define TPU_COMPARE1_TICK 7000
#define TPU_REARM_IRQS 3

/* Event decoder for GP0, the first general-purpose timing output (tpu/gp.c uses
   the same base for GP0..GP4); the decoder occupies event-word bits 6..10. */
#define TPU_DECODER_GP0 10

static volatile uint32_t compare_irqs;
static volatile uint32_t compare_sequence[8];
static volatile uint32_t compare_request_irqs;
static volatile bool compare_request_seen;
static volatile bool compare_rearm;

static void tpu_configure_clock(uint32_t rmc, uint32_t k, uint32_t l) {
	TPU_CLC = rmc << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = k << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = l << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
}

/* The firmware's frame configuration: 13 MHz / 6 counter, one-frame modulo,
   the full table in Timer RAM, group 0 enabled. */
static void tpu_configure_frame(const char *events_name) {
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 2);
	TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
	TPU_OFFSET = 0;
	TPU_TGER = BIT(0);
	TPU_EAPB = 0;
	TPU_EAPT = events_name ? TPU_EVENT_TABLE_WORDS : 0;

	for (uint32_t i = 0; events_name && i < TPU_EVENT_COUNT; i++) {
		TPU_RAM(TPU_TIMER_RAM_BASE + i * TPU_EVENT_WORDS_PER_ENTRY + 0) = 0;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * TPU_EVENT_WORDS_PER_ENTRY + 1) =
			TPU_EVENT_FIRST_TICK + i * TPU_EVENT_TICK_STRIDE;
		TPU_RAM(TPU_TIMER_RAM_BASE + i * TPU_EVENT_WORDS_PER_ENTRY + 2) = 0;
	}
}

static uint32_t measure_wrap_us(void) {
	uint32_t previous = TPU_COUNTER;
	uint32_t wraps = 0;
	stopwatch_t first = 0;
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS) {
		uint32_t current = TPU_COUNTER;
		if (current < previous) {
			stopwatch_t now = stopwatch_get();
			if (wraps == 0)
				first = now;
			if (wraps == TPU_FRAME_PERIODS) {
				uint64_t elapsed_ticks = now - first;
				return elapsed_ticks * 1000000 / (TPU_FRAME_PERIODS * stopwatch_ticks_per_s());
			}
			wraps++;
		}
		previous = current;
		test_watchdog_serve();
	}

	return 0;
}

static bool wait_counter_at_least(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_COUNTER < value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_COUNTER >= value;
}

static bool wait_ceap_at_least(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_CEAP < value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_CEAP >= value;
}

/* Returns the frame counter at which the GP0 request was first observed, or 0 if
   it never arrived within the timeout. */
static uint32_t wait_gp_request(void) {
	stopwatch_t start = stopwatch_get();

	while ((TPU_GP_SRC(0) & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	if ((TPU_GP_SRC(0) & MOD_SRC_SRR) == 0)
		return 0;

	return TPU_COUNTER;
}

static bool wait_compare_irqs(uint32_t count) {
	stopwatch_t start = stopwatch_get();

	while (compare_irqs < count && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return compare_irqs >= count;
}

static bool value_matches_ppm(uint64_t actual, uint64_t expected, uint32_t tolerance_ppm) {
	uint64_t tolerance = expected * tolerance_ppm / 1000000;

	if (tolerance == 0)
		tolerance = 1;

	return actual + tolerance >= expected && actual <= expected + tolerance;
}

/*
 * Read EAPT back, letting the write settle, and report how many reads it took.
 *
 * On the phone EAPT read 0 immediately after 459 was written and 459 later in
 * the same test, so one read races something - a write that has not landed, a
 * field that is write-only, or a read aimed at the wrong offset.  A bounded
 * poll of the value that was written distinguishes those, where a single read
 * cannot; the caller prints the count so a settle delay is documented rather
 * than assumed.
 */
static uint32_t probe_readback(uint32_t expected, uint32_t *reads) {
	uint32_t value = TPU_EAPT;

	for (*reads = 1; (value & TPU_EAPT_VALUE) != expected && *reads < 64; (*reads)++) {
		test_watchdog_serve();
		value = TPU_EAPT;
	}

	return value;
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (compare_rearm && irq == VIC_TPU_INT0_IRQ) {
		if (compare_irqs < ARRAY_SIZE(compare_sequence))
			compare_sequence[compare_irqs] = irq;
		compare_irqs++;
		if ((TPU_SRC(0) & MOD_SRC_SRR) != 0)
			compare_request_irqs++;
		TPU_PARAM = 0;
		if (compare_irqs < TPU_REARM_IRQS) {
			TPU_OVERFLOW = TPU_FRAME_TICKS - 1;
			TPU_INT(0) = TPU_COMPARE0_TICK;
			TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
			TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
		} else {
			TPU_SRC(0) = MOD_SRC_CLRR;
		}
	} else if (irq == VIC_TPU_INT0_IRQ || irq == VIC_TPU_INT1_IRQ) {
		uint32_t index = irq == VIC_TPU_INT0_IRQ ? 0 : 1;

		if (compare_irqs < ARRAY_SIZE(compare_sequence))
			compare_sequence[compare_irqs] = irq;
		compare_irqs++;

		/* The request is level-sticky on the SRC node: the handler must retire
		   it, and retiring it takes the enable down, so re-assert SRE. */
		if ((TPU_SRC(index) & MOD_SRC_SRR) != 0)
			compare_request_seen = true;
		TPU_SRC(index) = MOD_SRC_CLRR | MOD_SRC_SRE;
	}

	VIC_IRQ_ACK = 1;
}

/*
 * Two hardware clocks off one crystal, compared against each other.
 *
 * Every rate assertion in this file divides a hardware counter by an ASSUMED
 * reference rate, so a wrong reference constant and a wrong counter divider are
 * indistinguishable from inside the phone: it has one crystal, and a
 * measurement whose only reference is the clock under test cannot falsify that
 * clock.  (The playback suites have the same shape taken further:
 * dsp/afe-playback.asm times both phases by counting the DSP's own sample-ring
 * wraps, so a clock halved by any cause leaves the wrap count and the "1 kHz"
 * tone identical - the tone is 1 kHz in samples.)
 *
 * This category counts three divider chains against each other, from raw
 * counter values sampled over the same window, with no microsecond conversion
 * anywhere:
 *
 *   GPTU0 T0   fSYS / GPTU_CLC.RMC                    (RMC = 1 -> fSYS)
 *   TPU        fSYS / TPU_CLC.RMC / 6L * K            (= fSYS / 12 here)
 *   STM        register-derived STM counter clock
 *
 * The ratios they must hold are fixed by register programming, so the
 * observable is the DISAGREEMENT: agreement to ppm is what clocks off one
 * crystal owe each other, and a disagreement is a constant or a divider, never
 * noise.  What this cannot see is an absolute crystal error - all three scale
 * together and cancel.  The RTC's 32.768 kHz crystal is the only reference in
 * the part that could see that; cgu-fsys checks the fSYS rate against it.
 */
static void gptu0_counter_start(void) {
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_T01IRS(GPTU0) =
		GPTU_T01IRS_T0BINS_CONCAT | GPTU_T01IRS_T0CINS_CONCAT | GPTU_T01IRS_T0DINS_CONCAT;
	GPTU_T0RDCBA(GPTU0) = 0;
	GPTU_T0DCBA(GPTU0) = 0;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN |
		GPTU_T012RUN_T0CRUN | GPTU_T012RUN_T0DRUN;
}

static void gptu0_counter_stop(void) {
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_CLC(GPTU0) = MOD_CLC_DISR;
}

static void test_clock_agreement(void) {
	test_category("TPU / STM / GPTU: clocks off one crystal must agree");

	/* OVERFLOW is 15 bit, so the counter must stay below 32768 ticks for the
	   whole window: fSYS / 12 * 10 ms = 21667 ticks. */
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 2);
	TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	gptu0_counter_start();

	/* Sample the two ends in opposite orders, so the read skew of the three
	   MMIO reads cancels between them instead of biasing the ratios. */
	stopwatch_t stm_first = stopwatch_get();
	uint32_t tpu_first = TPU_COUNTER;
	uint32_t gptu_first = GPTU_T0DCBA(GPTU0);

	stopwatch_usleep_wd(TPU_CROSSCHECK_US);

	uint32_t gptu_last = GPTU_T0DCBA(GPTU0);
	uint32_t tpu_last = TPU_COUNTER;
	stopwatch_t stm_last = stopwatch_get();

	gptu0_counter_stop();
	TPU_PARAM = 0;

	uint64_t stm_ticks = (uint64_t) (stm_last - stm_first);
	uint64_t tpu_ticks = tpu_last - tpu_first;
	uint64_t gptu_ticks = gptu_last - gptu_first;
	uint32_t f_sys = cpu_get_sys_freq();
	uint32_t f_stm = cpu_get_stm_freq();
	/* The stopwatch's own constant is the only unit here; the raw ratios above
	   hold regardless of it, and the last check is the two constants agreeing. */
	uint32_t stm_hz = stopwatch_ticks_per_s();
	uint32_t tpu_hz = (uint32_t) (stm_ticks ? stm_hz * tpu_ticks / stm_ticks : 0);
	uint32_t gptu_hz = (uint32_t) (stm_ticks ? stm_hz * gptu_ticks / stm_ticks : 0);

	/* The window bounds every count below 2^32 (fSYS / 12 * 10 ms = 21667 TPU
	   ticks; even a 208 MHz fSYS gives 2.6e6 STM ticks), and the BSP printf does
	   not implement the length modifiers. */
	printf("# cross-check over %u us: STM=%u ticks, TPU=%u ticks, GPTU0 T0=%u ticks\n",
		(unsigned int) TPU_CROSSCHECK_US,
		(unsigned int) stm_ticks, (unsigned int) tpu_ticks, (unsigned int) gptu_ticks);
	printf("# cross-check rates in the stopwatch's own unit: STM=%u Hz, TPU=%u Hz, GPTU0=%u Hz\n",
		(unsigned int) stm_hz, (unsigned int) tpu_hz, (unsigned int) gptu_hz);

	if (stm_ticks == 0 || tpu_ticks == 0 || gptu_ticks == 0 || f_stm == 0) {
		test_check("the three counters all ran for the whole window", false);
		return;
	}

	/* Model-free: two chains off fSYS, one divided by 12 by its own registers. */
	test_check("the TPU counter is exactly fSYS/12 of the GPTU T0 counter",
		value_matches_ppm(tpu_ticks * TPU_CROSSCHECK_TPU_DIVISOR, gptu_ticks, TPU_CROSSCHECK_PPM));

	/* The STM chain against the register-derived STM and fSYS frequencies. */
	test_check("the STM counter agrees with the register-derived frequency",
		value_matches_ppm(stm_ticks * f_sys, gptu_ticks * f_stm, TPU_CROSSCHECK_PPM));

	/* The headline: the two constants the phone disagreed about, compared with
	   each other rather than each against its nominal. */
	test_check("the TPU counter timed by the STM is the firmware's 13 MHz / 6",
		value_matches_ppm(tpu_hz, TPU_COUNTER_FREQUENCY, TPU_CROSSCHECK_PPM));
}

static uint32_t test_frame_period(void) {
	test_category("One frame is 10 000 ticks (OVERFLOW = 9999)");

	tpu_configure_frame(NULL);
	test_eq_u32("OVERFLOW holds the firmware's frame value", TPU_FRAME_TICKS - 1, TPU_OVERFLOW);

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t period_us = measure_wrap_us();
	TPU_PARAM = 0;
	printf("# frame period with OVERFLOW=9999: %u us (expected %u us)\n",
		(unsigned int) period_us, (unsigned int) TPU_FRAME_US);
	test_check("frame wrap takes the 4.615 ms TDMA frame",
		value_matches_ppm(period_us, TPU_FRAME_US, TPU_TOLERANCE_PPM));

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("counter stays inside the frame modulo", wait_counter_at_least(TPU_FRAME_TICKS - 100));
	test_check("counter never exceeds OVERFLOW", TPU_COUNTER <= TPU_FRAME_TICKS - 1);
	TPU_PARAM = 0;

	return period_us;
}

static void test_event_table(uint32_t unarmed_us) {
	test_category("459-word / 153-event hardware table");

	tpu_configure_frame("frame");

	/*
	 * EAPT probe - evidence, never an assertion.
	 *
	 * The phone run read 0 here, immediately after 459 was written, and 459 later
	 * in the same test.  Delayed visibility (the write needs time before a read
	 * sees it) is a HYPOTHESIS, not a diagnosis: the same two readings arise if
	 * the ARM's store is STAGED and something else commits it - and the only
	 * candidate for "something else" on this path is the timer initialise bit,
	 * because no store between the two assertions targets 0x03C at all (the
	 * window contains only TPU_RAM, TPU_GP_SRC and TPU_PARAM writes, and no helper
	 * writes EAPT).  So that alternative is excluded FIRST: three values are
	 * written and read back immediately and after a bounded poll (which covers
	 * delayed visibility), and then one write is observed across a TINI (which
	 * covers a staged write committed by the initialise).  Both columns are
	 * printed for every case, with the two neighbouring pointer registers.
	 *
	 * What the next run's numbers decide:
	 *
	 *  - 0 immediately, the value after a poll (no TINI in between) -> delayed
	 *    visibility; the assertion below polls instead of racing;
	 *  - 0 before TINI, the value after TINI -> the write is STAGED and TINI
	 *    commits it.  Delayed visibility is then excluded, the assertion's timing
	 *    was never the problem, and the readback has to be taken after arming;
	 *  - 0 for every value at every time -> EAPT is write-only on the part, or the
	 *    read is somewhere else; the honest assertions are then the observables
	 *    (the shortened-table control and the 153rd event's request);
	 *  - CEAP (0x038) or EAPB (0x040) moves instead -> the read lands on the
	 *    wrong offset;
	 *  - the value comes back masked or shifted -> the part's field is narrower
	 *    than the map says.  TPU.cfg writes "VALUE 0 9" for all three pointers and
	 *    the generator turns that into GENMASK(8, 0) (tools/gen_headers.pl:
	 *    "GENMASK(start + size - 1, start)" - the cfg's second token is a SIZE, not
	 *    an end bit), i.e. a 9-bit field, 0..511, which is what the header emits and
	 *    what a 459-word table needs.  cfg and header agree; only the part can
	 *    disagree.
	 */
	static const uint32_t eapt_probe[] = { TPU_EVENT_TABLE_WORDS, 0x0FF, 0x001 };

	for (uint32_t i = 0; i < ARRAY_SIZE(eapt_probe); i++) {
		uint32_t settled_reads = 0;
		uint32_t immediate;

		TPU_EAPT = eapt_probe[i];
		immediate = TPU_EAPT;
		uint32_t settled = probe_readback(eapt_probe[i], &settled_reads);

		printf("# EAPT probe: wrote %08X -> immediate %08X; after %u reads %08X; "
			"CEAP=%08X EAPB=%08X\n",
			(unsigned int) eapt_probe[i], (unsigned int) immediate, (unsigned int) settled_reads,
			(unsigned int) settled, (unsigned int) TPU_CEAP, (unsigned int) TPU_EAPB);
	}

	/* The same write observed across a timer initialise - the exclusion step: if
	   the value appears only once TINI has been written, the store was staged and
	   TINI committed it, which is a different mechanism from delayed visibility
	   and would mean the assertion's timing was never the problem.  The machine
	   has no GP0 decoder in its table yet, so this brief arm cannot raise a
	   request. */
	TPU_EAPT = TPU_EVENT_TABLE_WORDS;
	uint32_t eapt_before_arm = TPU_EAPT;
	uint32_t eapt_arm_reads = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t eapt_after_arm = probe_readback(TPU_EVENT_TABLE_WORDS, &eapt_arm_reads);
	TPU_PARAM = 0;
	printf("# EAPT probe: wrote %08X -> before TINI %08X; after TINI %08X (%u reads)\n",
		(unsigned int) TPU_EVENT_TABLE_WORDS, (unsigned int) eapt_before_arm,
		(unsigned int) eapt_after_arm, (unsigned int) eapt_arm_reads);

	/* The claim, read where it matters - after the table has been armed, which is
	   the state the machine actually uses.  Whether it also reads back BEFORE the
	   arm is exactly what the line above reports, and this assertion deliberately
	   does not depend on the answer: it fails only if the value never becomes
	   readable, or comes back wrong. */
	test_eq_u32("EAPT accepts the firmware's 459-word table", TPU_EVENT_TABLE_WORDS,
		eapt_after_arm & TPU_EAPT_VALUE);
	test_eq_u32("the table starts at the firmware's Timer RAM base", 0, TPU_EAPB);

	/* Give ONLY the 153rd event an ARM-visible effect: decoder 10 is GP0, whose
	   service request (TPU_GP_SRC(0)) is readable from the ARM. Nothing else in
	   the table requests service, so a request proves the engine really walked
	   all 153 entries. (tpu/gp.c establishes the GP decoder range and the GP VIC
	   lines; this test only needs the request bit.) */
	TPU_RAM(TPU_TIMER_RAM_BASE + TPU_EVENT_LAST_OFFSET + 2) = TPU_DECODER_GP0 << 6;
	TPU_GP_SRC(0) = MOD_SRC_CLRR;

	/*
	 * Negative control for EAPT, and the observable form of "the table length is
	 * the value written": with the SAME table but EAPT shortened to two entries,
	 * the 153rd event lies outside the table, so the request must NOT appear over
	 * a complete frame.  Its pair is the 153rd-event request below - together they
	 * show the machine's walk boundary follows the written length, which is what
	 * the readback assertion is a proxy for and cannot show by itself (a machine
	 * that ignored EAPT and a machine that read it as 459 would both walk 153
	 * events).  Checked after the frame has wrapped rather than by polling, so a
	 * slow ARM cannot mistake "not yet" for "never".
	 */
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	TPU_EAPT = 2 * TPU_EVENT_WORDS_PER_ENTRY;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t short_frame_us = measure_wrap_us();
	bool short_table_clear = (TPU_GP_SRC(0) & MOD_SRC_SRR) == 0;
	TPU_PARAM = 0;
	TPU_EAPT = TPU_EVENT_TABLE_WORDS;
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	printf("# EAPT control: EAPT=%u over a %u us frame -> SRR=%u\n",
		(unsigned int) (2 * TPU_EVENT_WORDS_PER_ENTRY), (unsigned int) short_frame_us,
		(unsigned int) (short_table_clear ? 0U : 1U));
	test_check("a shortened EAPT stops the walk before the 153rd event",
		short_frame_us != 0 && short_table_clear);

	/*
	 * "The engine starts at the first event", as an observable rather than a
	 * pointer snapshot.  The old check read CEAP once immediately after TINI and
	 * demanded 0; the engine advances one event per 13 ticks (6 us) while an MMIO
	 * read is not instantaneous, so on the phone the sample was already past the
	 * start (the same run observed the engine at CEAP=3 while the counter was at
	 * 6895).  Moving the GP0 decoder into the FIRST entry instead makes the claim
	 * measurable: if the engine starts there, its request appears at that entry's
	 * tick, which is a property of the hardware rather than of the ARM's read
	 * latency.
	 */
	TPU_RAM(TPU_TIMER_RAM_BASE + TPU_EVENT_LAST_OFFSET + 2) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + 2) = TPU_DECODER_GP0 << 6;
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t first_event_counter = wait_gp_request();
	TPU_PARAM = 0;
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	TPU_RAM(TPU_TIMER_RAM_BASE + 2) = 0;
	TPU_RAM(TPU_TIMER_RAM_BASE + TPU_EVENT_LAST_OFFSET + 2) = TPU_DECODER_GP0 << 6;
	printf("# first entry: request at COUNTER=%u (first event tick %u)\n",
		(unsigned int) first_event_counter, (unsigned int) TPU_EVENT_FIRST_TICK);
	test_check("the engine starts at the first event",
		first_event_counter >= TPU_EVENT_FIRST_TICK && first_event_counter <= TPU_EVENT_FIRST_TICK + 200);

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("event machine steps past the first events",
		wait_ceap_at_least(2 * TPU_EVENT_WORDS_PER_ENTRY));
	TPU_PARAM = 0;

	/*
	 * Restart the frame and take BOTH samples below before any console output:
	 * a TAP line costs milliseconds of guest time, which is longer than a
	 * 4.615 ms frame, so a print between arming and sampling moves the counter a
	 * later sample is attributed to.  This is exactly what happened on the phone
	 * - the request raised at tick 2076 was observed at COUNTER=6895, ~2.2 ms of
	 * print later - so the observation and the assertion about it are now taken
	 * in the same uninterrupted statement, and the sample lag is printed.
	 */
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool no_early_request = wait_ceap_at_least(2 * TPU_EVENT_WORDS_PER_ENTRY) && TPU_COUNTER < 1000 &&
		(TPU_GP_SRC(0) & MOD_SRC_SRR) == 0;
	uint32_t request_counter = wait_gp_request();
	uint32_t last_event_tick = TPU_EVENT_FIRST_TICK + (TPU_EVENT_COUNT - 1) * TPU_EVENT_TICK_STRIDE;

	test_check("no request while the engine is among the first events", no_early_request);
	printf("# table walk: CEAP=%u at COUNTER=%u, last event offset %u at tick %u (observed +%d)\n",
		(unsigned int) TPU_CEAP, (unsigned int) request_counter,
		(unsigned int) TPU_EVENT_LAST_OFFSET, (unsigned int) last_event_tick,
		(int) (request_counter - last_event_tick));
	test_check("the 153rd table event executes and requests service", request_counter != 0);
	test_check("the request appears at the last event's tick",
		request_counter >= last_event_tick && request_counter <= last_event_tick + 200);
	TPU_GP_SRC(0) = MOD_SRC_CLRR;
	test_eq_u32("the last event's request is retired by CLRR", 0,
		TPU_GP_SRC(0) & MOD_SRC_SRR);
	/* CEAP is the pending-event pointer: it wraps back to EAPB once the table is
	   exhausted (tpu/ram.c asserts the same shape for a 2-event table), so the
	   value read after a full frame is EAPB plus the first expired event, not the
	   last executed one - which is why the GP request, not CEAP, is the proof that
	   the whole table ran. */
	TPU_PARAM = 0;

	/* The table must not disturb the counter: same OVERFLOW, and therefore the
	   period this run measures must match the one the unarmed run measured -
	   compared against that measurement, not against the nominal 4615 us, so a
	   swing of the frame cannot hide inside a band centred on a value neither
	   measurement was. */
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t armed_us = measure_wrap_us();
	TPU_PARAM = 0;
	printf("# frame period with the 459-word table armed: %u us (unarmed: %u us)\n",
		(unsigned int) armed_us, (unsigned int) unarmed_us);
	test_check("arming the 459-word table leaves the frame period unchanged",
		value_matches_ppm(armed_us, unarmed_us, TPU_TOLERANCE_PPM));
	test_eq_u32("event table length is unchanged by the walk", TPU_EVENT_TABLE_WORDS,
		TPU_EAPT);
}

static void test_compare_lines(void) {
	test_category("Compare lines on VIC 119/120");

	tpu_configure_frame(NULL);
	TPU_INT(0) = TPU_COMPARE0_TICK;
	TPU_INT(1) = TPU_COMPARE1_TICK;
	test_eq_u32("INT0 holds the first compare", TPU_COMPARE0_TICK, TPU_INT(0) & TPU_INT_VALUE);
	test_eq_u32("INT1 holds the second compare", TPU_COMPARE1_TICK, TPU_INT(1) & TPU_INT_VALUE);

	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;

	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	VIC_CON(VIC_TPU_INT1_IRQ) = 1;
	compare_irqs = 0;
	compare_request_seen = false;
	cpu_enable_irq(true);

	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("both compare lines fire in one frame", wait_compare_irqs(2));
	cpu_enable_irq(false);
	TPU_PARAM = 0;

	test_eq_u32("INT0 is wired to VIC line 119", 119, compare_sequence[0]);
	test_eq_u32("INT1 is wired to VIC line 120", 120, compare_sequence[1]);
	test_check("the compare request is set when the IRQ is delivered", compare_request_seen);

	cpu_enable_irq(true);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("both compare lines repeat after the frame wrap", wait_compare_irqs(4));
	cpu_enable_irq(false);
	TPU_PARAM = 0;

	test_eq_u32("second frame INT0 is VIC line 119", 119, compare_sequence[2]);
	test_eq_u32("second frame INT1 is VIC line 120", 120, compare_sequence[3]);
	test_eq_u32("INT0 request is retired by CLRR|SRE", 0, TPU_SRC(0) & MOD_SRC_SRR);
	test_eq_u32("INT1 request is retired by CLRR|SRE", 0, TPU_SRC(1) & MOD_SRC_SRR);

	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	VIC_CON(VIC_TPU_INT0_IRQ) = 0;
	VIC_CON(VIC_TPU_INT1_IRQ) = 0;
}

static void test_compare_rearm(void) {
	test_category("INT0 one-shot re-arm");

	tpu_configure_frame(NULL);
	TPU_INT(0) = TPU_COMPARE0_TICK;
	TPU_INT(1) = TPU_FRAME_TICKS;
	TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_CLRR;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	VIC_CON(VIC_TPU_INT1_IRQ) = 0;
	compare_irqs = 0;
	compare_request_irqs = 0;
	compare_rearm = true;
	cpu_enable_irq(true);
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;

	wait_compare_irqs(TPU_REARM_IRQS);
	cpu_enable_irq(false);
	compare_rearm = false;
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	uint32_t pending = TPU_SRC(0) & MOD_SRC_SRR;
	VIC_CON(VIC_TPU_INT0_IRQ) = 0;

	test_eq_u32("INT0 explicit re-arm produces exactly three IRQs", TPU_REARM_IRQS, compare_irqs);
	test_eq_u32("every re-armed IRQ carries an INT0 request", TPU_REARM_IRQS, compare_request_irqs);
	bool routed = true;
	for (uint32_t i = 0; i < TPU_REARM_IRQS; i++)
		routed &= compare_sequence[i] == VIC_TPU_INT0_IRQ;
	test_check("every re-armed INT0 is wired to VIC line 119", routed);
	test_eq_u32("INT0 request is clear after re-arm test", 0, pending);
}

int main(void) {
	test_start("TPU frame timing test");

	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_clock_agreement();
	uint32_t unarmed_us = test_frame_period();
	test_event_table(unarmed_us);
	test_compare_lines();
	test_compare_rearm();

	TPU_PARAM = 0;
	return test_finish();
}
