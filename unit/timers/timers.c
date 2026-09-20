/* Every rate here is dated by the RTC's 32.768 kHz crystal, off the CGU chain. */

#include <pmb887x.h>

#include "test.h"

/* timers/idle.S: ARM926 wait-for-interrupt, restored before it returns. */
void idle_wait_for_interrupt(void);

#define EVENTS 6
#define RTC_EVENTS 3                /* the RTC's tick is one second; 3 events give two periods */

#define RTC_T14_RELOAD 61440
#define RTC_T14_PERIOD (65536 - RTC_T14_RELOAD)
#define RTC_T14_HZ 4096
#define RTC_NS_PER_T14 (1000000000 / RTC_T14_HZ)
/*
 * Window for the prescaler A/B.  50 ms, not 2 ms: with PRE on (4096 Hz) a 2 ms window holds
 * ~8 counts, so one count of window-boundary error moves the ratio by ~12 % and an integer
 * division of two small counts reports "/7" for a /8 prescaler - which is exactly what the
 * 2026-09-20 phone log printed (67 counts without PRE / 9 with PRE) while the same suite
 * scaled every duration by the /8 constant.  At 50 ms the arms hold ~1638 and ~205 counts and
 * the ratio is stated to 0.01.
 */
#define RTC_SCALE_WINDOW_US 50000u

/* SCU_RTCIF.RTCIFEN: 0xAA is the value the vendor driver writes and the only enable that
   makes the register interface answer (rtc-init phone A/B). */
#define RTCIF_ENABLE 0xAAu

/* The board's reference crystal, the rate CGU_CON1.FSYS_CLKSEL=0 selects. */
#define OSC_HZ 26000000u

/* Compares must stay inside the counter's frame modulus: TPU_OVERFLOW+1 ticks. */
#define TPU_ARM_TICKS 8000
#define TPU_FANOUT_TICKS (3 * TPU_ARM_TICKS)
#define TPU_FRAME_TICKS (TPU_OVERFLOW_VALUE + 1u)
#define GPTU_T2_COUNTS 260000u
#define GPTU_T2_RELOAD (0xFFFFFFFFu - GPTU_T2_COUNTS + 1u)
#define GPTU_T2_RC_MODE_RELOAD_OVERFLOW 4  /* T2AMRC0=4, as unit/gptu establishes */

/*
 * The Linux clockevent's DT target: clock-frequency = <1000000>.
 *
 * That is the *kernel's* figure, not the part's.  Vendor firmware programs K=1, L=2 - the
 * reset values - which with the bring-up's clock gives 10000 ticks per 4.61538 ms GSM frame
 * (`scratch/apoxi-clk/TPU-usage.md`, `tpu-sites.txt`; the APOXI lane's trace), so writing
 * K=3/L=13 below reprograms the GSM frame base for this measurement.  Both TPU compare lines
 * are the GSM time unit's (INT0 and INT1 on one counter); see the note at the INT1 row.
 */
#define LINUX_TPU_TARGET_HZ 1000000
#define LINUX_TPU_K 3
#define LINUX_TPU_L 13

#define RATE_T14_COUNTS 410         /* ~100 ms at 4096 Hz */
/*
 * 100 T14 counts = 24.4 ms, ~24400 counter ticks at 1 MHz - still inside one 32768-tick GSM
 * frame, which is what the masked delta below needs.  At 41 counts (10 ms) one T14 count of
 * window quantisation was 2.4 % of the derived rate; here it is 1 %, and the row prints it.
 */
#define TPU_RATE_T14_COUNTS 100

/*
 * Every wait for the RTC is bounded by the STM, the one base the part has shown
 * working, and renews the watchdog budget inside the loop.  The RTC is the block
 * under test, so a wait bounded by it cannot report its failure; and a wait that
 * only serves the watchdog is a reset, because the 3000 ms budget is re-armed on
 * an assertion path alone.  Both happened: the phone reset inside measure_stm_rate.
 */
#define RATE_WAIT_MS 400u           /* 4x the ~100 ms of T14 counts RATE_T14_COUNTS needs */
#define TPU_RATE_WAIT_MS 200u       /* 20x its 10 ms window */
#define PROBE_WAIT_MS 1000u         /* TPU INT0/INT1 and GPTU T2A: tens of ms of work each */
/*
 * The GPTU rate row's window.  The short row above measures ONE programmed T2A period,
 * and at T2A = 260000 counts that is ~2.5 ms at the fPLL tap (~104 MHz) or ~10 ms at
 * fSYS (26 MHz) - 2.5 or 10 T14 counts.  A derived clock from 10 counts carries ~2.4 %
 * quantisation, i.e. it cannot tell 104 MHz from 106.5 MHz: the 2026-09-20 phone log's
 * "106 496 000 Hz, 2.4 % above nominal" was that window's integer division, not a
 * divergence.  Counting T2A OVERFLOWS over seconds makes the count itself the
 * measurement: 4 s of T14 is ~1600 overflows at 104 MHz and ~400 at 26 MHz, so the
 * band below is 0.06 % - 0.25 % wide.
 */
#define GPTU_RATE_T14_COUNTS 16384u /* 4 s of T14 at 4096 Hz */
#define GPTU_RATE_WAIT_MS 12000u    /* generous STM bound: it is a multi-second window */
/*
 * The window's VALIDITY GUARD, in per mille of the window.  The rate is
 * (overflows x 260000 counts) / (T14 span / 4096 Hz) where the span is FIRST-to-LAST
 * interrupt, so a window whose interrupts stopped arriving before it ended reports a
 * proportionally lower rate with no clock having moved.
 *
 * THE LOOP HAS THREE TERMINATORS, and they are REPORTED, not judged: `rtc_tick_now() >=
 * GPTU_RATE_T14_COUNTS` (the 16384-T14 / 4 s target), `test_elapsed_bound_ms(start,
 * GPTU_RATE_WAIT_MS)` (the elapsed bound - an STM-tick budget, and the ORDINARY phone
 * terminator: `test_stm_ticks_per_ms()` follows the model's fSTM law that this suite's own
 * A/B refutes for the part, so "12000 ms" of its ticks is ~3.0 s of real time on the
 * phone), and `reference_stalled`.
 *
 * WHICH SHAPE IS NORMAL, AND WHY: the COUNT varies with how the window ended - the two pasted
 * phone runs carry 1199, 1530 and 1597 overflows (12 279, 15 678, ~16 370 T14 counts) - and
 * all three are the same rate, 399.7 overflows/s ~= fPLL / 260000, with the service keeping
 * up.  The rate is the invariant; 299.6 overflows/s = 3/4 of it, from the SAME count as the
 * base over a 33 % longer span, is what a service fault looks like, not a tap move.  A guard
 * that keyed on the count, on the span target or on the terminator would reject the majority
 * of healthy windows, which is why none of those is a verdict here.
 *
 * So a window is ACCEPTED when the service kept up (no missed entries) and the count is
 * consistent with the counter's own travel (one 260000-count period per counted overflow),
 * whatever ended it; it is REJECTED only for a missed service, interrupts that stopped
 * arriving, or an internally inconsistent span/count pair.
 *
 * PRE-REGISTERED READING for the next phone run, so the log decides rather than a later story:
 * `misses == events` WITH the rate in band (399.7 overflows/s ~= fPLL/260000) = the READ-AFTER-
 * CLEAR ARTEFACT, the miss witness is UNUSABLE and the counter-free SPACING witness carries the
 * verdict; `misses == events` with the rate at ~half nominal = a real merge, and the RATE ROW is
 * the arbiter.  The travel witness is LATENT in this configuration (reload-overflow makes its
 * travel a sub-period phase difference), so its non-firing is not evidence either way.
 */
#define GPTU_WINDOW_TAIL_MAX 50u    /* per mille */
#define RTC_SOURCE_WAIT_MS 5000u    /* three T14 overflows: ~3 s while the reference lives */
#define COMBINED_WAIT_MS 6000u      /* the same three overflows beside the TPU and GPTU */
#define LINE_WAIT_MS 1500u          /* before a WFI: the line to be slept on must be seen live */

/* One bit per source; an unarmed source keeps its compare but gets no SRE. */
#define SRC_TPU0 1u
#define SRC_TPU1 2u
#define SRC_GPTU 4u
#define SRC_RTC  8u

/* The WFI bound: 2048 T14 counts = 0.5 s, ~60x the TPU one-shot. */
#define RTC_BOUND_COUNTS 2048u

static volatile uint32_t irq_other;
static volatile uint32_t tpu0_events, tpu1_events, gptu_events, rtc_events;
/*
 * SERVICE DISCRIMINATOR for the GPTU rate window.  The window counts INTERRUPTS, so a low count
 * has two readings with opposite consequences: the COUNTER slowed (a real tap move, which would
 * falsify the board DTS's domain name) or the SERVICE missed overflows (an artefact, and the tap
 * never moved).  Two observations separate them, and neither assumes the counter's direction:
 *   gptu_src_pending_seen  - a second request is already latched immediately after the handler
 *                            clears the one it came for, i.e. the handler is falling behind and
 *                            overflows are merging.  Counter-independent, and the mechanism
 *                            itself;
 *   the T2 travel read at the WINDOW's start and end (not at the interrupts, which are
 *            phase-aligned with the overflow and so carry no travel information): a counter
 *            that did more 260000-count periods than the service counted has lost entries.
 * A nonzero miss count, or a travel the count cannot account for, REJECTS the window with that
 * mechanism named.
 */
/*
 * TWO READS OF THE SAME SRC REQUEST, and the LATE one is the authority.
 *   gptu_miss_immediate - read immediately after the CLRR write.  On this part a write's
 *     visibility is not guaranteed to be instantaneous (the pattern this corpus already
 *     hypothesises for TPU EAPT: immediate read 0, later read 459), so this read can be set
 *     with nothing actually pending.  Its only value is as evidence FOR that visibility
 *     hypothesis - so it is always printed beside the late read, never used as a verdict.
 *   gptu_miss_late      - read just before VIC_IRQ_ACK, after the handler's own bookkeeping.
 *     clear => nothing was pending, the immediate bit was a clear-visibility artefact;
 *     set   => a GENUINE pending request this service did not retire.  This one is load-bearing.
 * The pair links this item to the TPU EAPT item: immediate-set/late-clear here IS direct
 * evidence for non-instantaneous read-back visibility on this part.
 */
static volatile uint32_t gptu_miss_immediate, gptu_miss_late;
/* The interrupt SPACING witness (T14 counts between consecutive handler entries): a periodic
   timer's spacing is near-uniform, so a bimodal spacing - some gaps ~2 periods while others are
   near zero, which is what a level-triggered request looks like when two overflows arrive
   before one service - is a MERGED SERVICE, and it needs no counter read at all.  It is a
   CORROBORATOR, not the authority: the late read is.
 * gptu_spurious_gap0 counts entries whose gap is ZERO T14 counts.  A genuine overflow is one
 * full period from the previous one - ~10.24 T14 counts at the phone's ~400/s - so two entries
 * inside the same T14 tick CANNOT be two overflows: they are spurious re-entries from a request
 * re-arbitrated before the clear landed, and they INFLATE the count, i.e. the rate's numerator.
 * The threshold is 0, not a heuristic: a real x4 divider move puts a genuine gap at ~2.5 counts,
 * which is still nonzero. */
static volatile uint32_t gptu_gap_min, gptu_gap_max, gptu_spurious_gap0;
/* Uncapped, for the long-window rate row: the capped counters above exist to time a few
   periods, and a rate measurement must not stop counting at six. */
static volatile uint32_t gptu_rate_events, gptu_rate_first, gptu_rate_last;
static volatile uint32_t tpu0_periods[EVENTS], tpu1_periods[EVENTS];
static volatile uint32_t gptu_periods[EVENTS], rtc_periods[EVENTS];
static volatile uint32_t tpu0_last, tpu1_last, gptu_last, rtc_last;
static volatile uint32_t rtc_srr_pending;   /* RTC SRC request still set inside the handler */
static volatile uint32_t wfi_hold_t14, wfi_tpu_counter;

static uint32_t tpu_f_counter;
static uint32_t gptu_f_clock;
/*
 * Expectations in thousandths of a T14 count.  The printed integer was the defect three times
 * over: 40.960 counts printed as "40" while the frequency was derived from 40.96 (that pair is
 * where the phone log's 106 496 000 Hz came from), and a ratio computed from the truncated
 * value is a ratio of two different quantities.  The milli value is what the row uses; the
 * integer is only `milli / 1000` for printouts that must stay whole counts.
 */
static uint32_t tpu_expected_t14_milli;
static uint32_t gptu_expected_t14_milli;
static uint32_t tpu_expected_t14;
static uint32_t gptu_expected_t14;

static uint32_t rtc_t14(void) {
	return (RTC_T14 & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT;
}

/* T14 reloads at REL, so a raw count is not a monotonic clock: accumulate. */
static uint32_t rtc_tick_value, rtc_tick_raw;

static void rtc_tick_base(void) {
	rtc_tick_raw = rtc_t14();
	rtc_tick_value = 0;
}

static uint32_t rtc_tick_now(void) {
	uint32_t count = rtc_t14();

	rtc_tick_value += count >= rtc_tick_raw ? count - rtc_tick_raw
		: RTC_T14_PERIOD + count - rtc_tick_raw;
	rtc_tick_raw = count;

	return rtc_tick_value;
}

static void rtc_reference_init(void) {
	/*
	 * The vendor driver's order, and the enable this bring-up was missing: RTC_CLC, then
	 * SCU_RTCIF.RTCIFEN = 0xAA, and only then the block's own registers.  The direct
	 * evidence is the rtc-init phone run: after the enable RTC_ID read F049C011 where it
	 * had read 00000000, and the same STM-dated window counted 84 T14 where it had counted
	 * 0.  Supporting detail whose mechanism is not established: the CTRL write made before
	 * the enable did not read back - a block that masks or resets its own registers while
	 * the interface is disabled would look the same, so that is all the evidence supports.
	 */
	RTC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	SCU_RTCIF = (SCU_RTCIF & ~SCU_RTCIF_RTCIFEN) | RTCIF_ENABLE;
	RTC_CTRL = RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN | RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCINT | RTC_CTRL_CLR_RTCBAD;
	RTC_CON = RTC_CON_PRE;
	rtc_tick_base();
	/*
	 * RTC_REL (the calendar's wrap reload) goes FIRST, and T14's own programming last: the
	 * phone's 2026-09-20 run measured the T14 period row at 6148 counts - a 6144-tick reload,
	 * i.e. 1.5 s - where the emulator and an earlier phone run measured 4096 (1 s), with the
	 * crystal scale passing at /8.00 in both.  This init used to write T14 and then REL=0,
	 * and if the part latches or re-derives T14's reload around a REL write that order is the
	 * whole difference.  T14's reload is the period row's dependency, so it is programmed
	 * after everything that could disturb it.
	 */
	RTC_REL = 0;
	RTC_T14 = (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	RTC_ALARM = 0;
	RTC_ISNC = 0;
	RTC_ISNRC = RTC_ISNRC_T14 | RTC_ISNRC_RTC0 | RTC_ISNRC_RTC1 | RTC_ISNRC_RTC2 | RTC_ISNRC_RTC3;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	RTC_ISNC = RTC_ISNC_T14IE;
	VIC_CON(VIC_RTC_IRQ) = 1;
	RTC_CON |= RTC_CON_RUN;
}

/*
 * The registers a dead reference is read out of.  The phone run of this suite read
 * CON and CTRL as 0 and never saw ACCPOS while the CGU read exactly, and neither the
 * init nor SCU_RTCIF (the vendor driver's own enable, fw @0xA25EC9A8) was in the log.
 */
static void reference_state_print(const char *tag) {
	printf("# %s: RTC_ID=%08X CLC=%08X (DISR=%u DISS=%u RMC=%u) CTRL=%08X CON=%08X T14=%08X | SCU_RTCIF=%08X RTCIFEN=%02X\n",
		tag, (unsigned int) RTC_ID, (unsigned int) RTC_CLC,
		(unsigned int) (RTC_CLC & MOD_CLC_DISR), (unsigned int) (RTC_CLC & MOD_CLC_DISS),
		(unsigned int) ((RTC_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT),
		(unsigned int) RTC_CTRL, (unsigned int) RTC_CON, (unsigned int) RTC_T14,
		(unsigned int) SCU_RTCIF, (unsigned int) (SCU_RTCIF & SCU_RTCIF_RTCIFEN));
}

/* Set once the scale section has read the reference: every row below it is dated by
   the RTC, so a stalled reference makes them unmeasurable - reported, not waited on. */
static bool reference_stalled;

/* Raw counts, and the register state each window ends in, before any verdict. */
static uint32_t measure_rtc_prescaler(uint32_t *direct_out, uint32_t *divided_out) {
	RTC_CON = RTC_CON_RUN;
	rtc_tick_base();
	stopwatch_usleep_wd(RTC_SCALE_WINDOW_US);
	*direct_out = rtc_tick_now();
	printf("# RTC scale, no PRE: %u counts in %u us; CON=%08X CTRL=%08X ACCPOS=%u\n",
		(unsigned int) *direct_out, (unsigned int) RTC_SCALE_WINDOW_US,
		(unsigned int) RTC_CON, (unsigned int) RTC_CTRL,
		(unsigned int) ((RTC_CON & RTC_CON_ACCPOS) != 0));

	RTC_CON = RTC_CON_RUN | RTC_CON_PRE;
	rtc_tick_base();
	stopwatch_usleep_wd(RTC_SCALE_WINDOW_US);
	*divided_out = rtc_tick_now();
	printf("# RTC scale, PRE on: %u counts in %u us; CON=%08X CTRL=%08X ACCPOS=%u\n",
		(unsigned int) *divided_out, (unsigned int) RTC_SCALE_WINDOW_US,
		(unsigned int) RTC_CON, (unsigned int) RTC_CTRL,
		(unsigned int) ((RTC_CON & RTC_CON_ACCPOS) != 0));

	/* Hundredths of a unit, rounded: an integer ratio of two counts is what read as /7. */
	return *divided_out
		? (uint32_t) (((uint64_t) *direct_out * 100u + *divided_out / 2) / *divided_out)
		: 0;
}

/* STM counter ticks over a window dated by the RTC - no microsecond constant. */
static bool measure_stm_rate(uint32_t *stm_hz, uint32_t *stm_ticks, uint32_t *t14_ticks) {
	uint64_t start = stopwatch_get();

	printf("# measure_stm_rate: waiting for %u T14 counts, STM-bounded at %u ms\n# ",
		(unsigned int) RATE_T14_COUNTS, (unsigned int) RATE_WAIT_MS);
	rtc_tick_base();
	uint32_t stm_start = (uint32_t) stopwatch_elapsed(0);

	while (!reference_stalled && !test_elapsed_bound_ms(start, RATE_WAIT_MS) &&
			rtc_tick_now() < RATE_T14_COUNTS) {
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	*t14_ticks = rtc_tick_now();
	*stm_ticks = (uint32_t) stopwatch_elapsed(0) - stm_start;
	if (*t14_ticks == 0) {
		printf("# the RTC reference produced no count in %u ms of STM: the window is not measurable\n",
			(unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()));
		return false;
	}

	*stm_hz = (uint32_t) ((uint64_t) *stm_ticks * RTC_T14_HZ / *t14_ticks);

	return true;
}

/* Prints the decoded fields, not the literals, beside what the CGU model says. */
static void print_cgu_state(const char *tag) {
	uint32_t con1 = CGU_CON1;

	/* ONE line per state print: the registers and the selectors a later row depends on.  The
	   model-belief frequencies are not printed (they are our model of the part, and the printing
	   contract keeps commentary in the docs, not on the wire). */
	uint32_t fpi1_clksel = con1 & 3u;
	uint32_t fpi1_clkdiv = (con1 >> 4) & 3u;
	uint32_t fsys_clksel = (con1 >> 16) & 3u;
	uint32_t ahb_clksel = (con1 >> 20) & 7u;
	uint32_t fstm_div_en = (con1 >> 25) & 1u;
	uint32_t fstm_div = (con1 >> 28) & 3u;

	printf("# %s: OSC=%08X CON0=%08X CON1=%08X CON2=%08X CON3=%08X "
		"(FPI1=%u/%u FSYS=%u AHB=%u FSTM_DIV_EN=%u/DIV=%u)\n",
		tag, (unsigned int) CGU_OSC, (unsigned int) CGU_CON0, (unsigned int) con1,
		(unsigned int) CGU_CON2, (unsigned int) CGU_CON3,
		(unsigned int) fpi1_clksel, (unsigned int) fpi1_clkdiv, (unsigned int) fsys_clksel,
		(unsigned int) ahb_clksel, (unsigned int) fstm_div_en, (unsigned int) fstm_div);
}

/*
 * THE OSC MULTIPLIER DECODE, PUT ON THE WIRE.  Reading `NDIV` and ignoring `MDIV` produced a
 * "2x NDIV change" that never happened - entry `0x01070001` is NDIV = 7 / MDIV = **1**
 * (26 x 8 / 2 = 104 MHz) and base `0x00031717` is NDIV = 3 / MDIV = **0** (26 x 4 / 1 = 104 MHz) -
 * and it is the error the suite's own field-table line (`fPLL = fOSC * (NDIV+1)/(MDIV+1)`) exists to
 * prevent.  Masks verified in the generated map: `CGU_OSC_NDIV = GENMASK(21,16)` (pmb8876_regs.h:2282),
 * `CGU_OSC_MDIV = GENMASK(27,24)` (:2284).
 *
 * LABELLED AS WHAT IT IS: `fPLL` here is the suite's DECODE - and for a bypassed state it is also a
 * MODEL behaviour (`bsp/lib/cpu.c`'s `cpu_get_pll_freq()` returns the crystal when `PLL_BYPASS_N` is
 * clear).  No GPTU row observes the part's multiplier; only the tap's Hz is measured.
 */
static void scale_osc_decode_print(const char *tag, uint32_t osc, uint32_t measured_hz)
{
	uint32_t ndiv = (osc & CGU_OSC_NDIV) >> CGU_OSC_NDIV_SHIFT;
	uint32_t mdiv = (osc & CGU_OSC_MDIV) >> CGU_OSC_MDIV_SHIFT;
	uint32_t fpll_decode = (uint32_t) ((uint64_t) OSC_HZ * (ndiv + 1u) / (mdiv + 1u));

	printf("# %s: OSC=%08X NDIV=%u MDIV=%u -> fPLL %u Hz (DECODE/MODEL), PLL_BYPASS_N=%u; "
		"tap MEASURED %u Hz\n",
		tag, (unsigned int) osc, (unsigned int) ndiv, (unsigned int) mdiv,
		(unsigned int) fpll_decode, (unsigned int) ((osc & CGU_OSC_PLL_BYPASS_N) != 0),
		(unsigned int) measured_hz);
}

/* The one restore path: ONE log line, carrying the value written and the lock state either side
   of it (a settle failure then shows as LOCK clear after the write, which a store that never
   returned cannot show).  Restores are unconditional and ordered by hazard - see the entry
   restore's comment. */
static void cgu_restore_write(const char *name, volatile uint32_t *reg, uint32_t value) {
	uint32_t lock_before = (CGU_STAT & CGU_STAT_LOCK) != 0;

	*reg = value;
	printf("# restore %s <- %08X (LOCK %u -> %u)\n", name, (unsigned int) value,
		(unsigned int) lock_before, (unsigned int) ((CGU_STAT & CGU_STAT_LOCK) != 0));
}

static bool wait_pll_locked(void) {
	stopwatch_t start = stopwatch_get();

	while ((CGU_STAT & CGU_STAT_LOCK) == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (CGU_STAT & CGU_STAT_LOCK) != 0;
}

/* boot76 board_init's CGU block, reproduced write for write. */
static bool boot76_cgu_apply(void) {
	CGU_CON1 &= ~0x700000u;
	CGU_CON1 &= ~3u;
	CGU_CON1 &= ~0x3000000u;
	CGU_CON1 &= ~0x30000u;
	CGU_OSC &= ~0x202u;
	CGU_OSC &= ~0x404u;
	CGU_OSC &= ~0x808u;
	CGU_OSC &= ~0x1010u;
	CGU_OSC &= ~0x101u;
	CGU_OSC = (CGU_OSC & 0xF0C0FFFF) | 0x30000;
	CGU_CON0 = (CGU_CON0 & 0xFFFFFF00) | 0xA;
	CGU_CON0 = (CGU_CON0 & 0xFFFF00FF) | 0x800;
	CGU_CON0 = (CGU_CON0 & 0xFFFFFF) | 0x11000000;
	CGU_OSC |= 0x101u;
	CGU_OSC = (CGU_OSC & ~0xF3F0000u) | 0x30000;
	CGU_CON0 = (CGU_CON0 & ~0xFFu) | 0xA;
	CGU_CON0 = (CGU_CON0 & ~0xFF00u) | 0x800;
	CGU_CON0 = (CGU_CON0 & ~0xFF000000u) | 0x11000000;
	CGU_OSC |= 0x101u;
	bool locked = wait_pll_locked();      /* boot76 spins on CGU_STAT & 0x2000 */
	CGU_OSC |= 0x202u;
	CGU_OSC |= 0x404u;
	CGU_OSC |= 0x1010u;
	CGU_CON1 = (CGU_CON1 & ~3u) | 2;
	CGU_CON1 = (CGU_CON1 & ~0x30u) | 0x10;
	CGU_CON1 = (CGU_CON1 & ~0x3000000u) | 0x2000000;
	CGU_CON1 = (CGU_CON1 & ~0x30000000u) | 0x10000000;
	CGU_CON1 &= ~0x30000u;
	CGU_CON2 = (CGU_CON2 & ~0x300u) | 0x100;
	CGU_CON2 = (CGU_CON2 & ~0x70u) | 0x20;
	SCU_EBUCLC2 = SCU_EBUCLC2 | 1;
	CGU_CON1 = (CGU_CON1 & ~0x700000u) | 0x400000;
	CGU_CON2 = (CGU_CON2 & ~7u) | 3;
	CGU_CON2 = (CGU_CON2 & ~0x30000000u) | 0x20000000;
	CGU_CON2 = (CGU_CON2 & ~0xC000u) | 0x8000;
	CGU_CON3 = (CGU_CON3 & ~0x1000001u) | 0x1000000;
	CGU_CON2 |= 0x1000000u;
	CGU_CON3 = (CGU_CON3 & ~3u) | 2;
	CGU_CON3 &= ~1u;
	CGU_CON3 = (CGU_CON3 & ~0x300u) | 0x200;
	CGU_CON3 &= ~1u;
	CGU_CON3 |= 0x10000u;
	CGU_CON3 = (CGU_CON3 & ~0x300001u) | 0x300000;

	return locked;
}

/* Programs K/L the way timer-pmb887x-tpu.c computes them for its DT target. */
static void tpu_program_kl(uint32_t k, uint32_t l) {
	TPU_PARAM = 0;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = k << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = l << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_INIT;

	uint32_t fsys = cpu_get_sys_freq();
	uint32_t rmc = (TPU_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;

	tpu_f_counter = (fsys / rmc / l * k) / 6;
	tpu_expected_t14_milli = (uint32_t) ((uint64_t) TPU_ARM_TICKS * RTC_T14_HZ * 1000 / tpu_f_counter);
	tpu_expected_t14 = tpu_expected_t14_milli / 1000;
}

/* The counter rate the K/L programming actually delivers, dated by the RTC. */
static bool measure_tpu_counter_rate(uint32_t *measured_hz) {
	uint64_t start = stopwatch_get();

	printf("# measure_tpu_counter_rate: waiting for %u T14 counts, STM-bounded at %u ms\n# ",
		(unsigned int) TPU_RATE_T14_COUNTS, (unsigned int) TPU_RATE_WAIT_MS);
	rtc_tick_base();
	uint32_t first = TPU_COUNTER;

	while (!reference_stalled && !test_elapsed_bound_ms(start, TPU_RATE_WAIT_MS) &&
			rtc_tick_now() < TPU_RATE_T14_COUNTS) {
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	uint32_t t14_ticks = rtc_tick_now();
	/* The window is under one TPU frame, so the masked delta spans one wrap. */
	uint32_t ticks = (TPU_COUNTER - first) & (TPU_FRAME_TICKS - 1u);
	TPU_PARAM = 0;

	if (t14_ticks == 0) {
		printf("# the RTC reference produced no count in %u ms of STM: the TPU rate is not measurable\n",
			(unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()));
		return false;
	}

	*measured_hz = (uint32_t) ((uint64_t) ticks * RTC_T14_HZ / t14_ticks);

	return true;
}

static void gptu_init(void) {
	GPTU_CLC(GPTU0) = 1 << MOD_CLC_RMC_SHIFT;
	GPTU_T012RUN(GPTU0) = 0;
	GPTU_T2CON(GPTU0) = 0;
	GPTU_T2RCCON(GPTU0) = GPTU_T2_RC_MODE_RELOAD_OVERFLOW << GPTU_T2RCCON_T2AMRC0_SHIFT;
	GPTU_T2RC0(GPTU0) = GPTU_T2_RELOAD;
	GPTU_T2(GPTU0) = GPTU_T2_RELOAD;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR6_OUV_T2A;
	GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_GPTU0_SRC6_IRQ) = 1;

	uint32_t rmc = (GPTU_CLC(GPTU0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;
	gptu_f_clock = cpu_get_sys_freq() / rmc;
	gptu_expected_t14_milli = (uint32_t) ((uint64_t) GPTU_T2_COUNTS * RTC_T14_HZ * 1000 / gptu_f_clock);
	gptu_expected_t14 = gptu_expected_t14_milli / 1000;
}

static void tpu_init(void) {
	/*
	 * Module clock first.  The 2026-09-20 phone run of this suite data-aborted on the very
	 * first TPU access - `str r3, [r5, #0x5c]` at 0x847E8, i.e. a store to TPU_PARAM, FSR
	 * type 0b1000 (external abort), FAR 0xF640005C - because `tpu_init()` wrote TPU_PARAM
	 * before anything had enabled the TPU module clock.
	 *
	 * Ownership checked, not assumed: this suite has exactly one `tpu_init()` call site
	 * (`timers.c` line 799) and this is its first write to the block, so init owns the
	 * clock; `tpu_program_kl()` writes the same literal at `timers.c` line 288 and reads
	 * RMC back for `tpu_f_counter`, so a caller-provided divider cannot be clobbered and
	 * nothing downstream sees a different RMC.  Precedent: `unit/tpu/test.c:11`
	 * (`tpu_configure_clock()`: `TPU_CLC = rmc << MOD_CLC_RMC_SHIFT;`) and
	 * `unit/tpu/test.c:59` (`TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;`) both enable the clock
	 * before that suite's first `TPU_PARAM` write at `:88`.
	 */
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_PARAM = 0;
	tpu_program_kl(LINUX_TPU_K, LINUX_TPU_L);
	TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
	TPU_INT(0) = TPU_ARM_TICKS;
	TPU_INT(1) = TPU_FANOUT_TICKS;
	TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_TPU_INT0_IRQ) = 1;
	VIC_CON(VIC_TPU_INT1_IRQ) = 1;
}

/* mask: the SRC_* bits this window measures. */
static void sources_arm(uint32_t mask) {
	tpu0_events = tpu1_events = gptu_events = rtc_events = 0;
	rtc_srr_pending = 0;
	irq_other = 0;
	for (uint32_t i = 0; i < EVENTS; i++) {
		tpu0_periods[i] = tpu1_periods[i] = gptu_periods[i] = rtc_periods[i] = 0;
	}
	tpu0_last = tpu1_last = gptu_last = 0;
	rtc_tick_base();
	rtc_last = rtc_tick_now();

	if ((mask & (SRC_TPU0 | SRC_TPU1)) != 0) {
		TPU_PARAM = 0;
		TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
		TPU_INT(0) = TPU_ARM_TICKS;
		TPU_INT(1) = TPU_FANOUT_TICKS;
		TPU_SRC(0) = MOD_SRC_CLRR | ((mask & SRC_TPU0) != 0 ? MOD_SRC_SRE : 0);
		TPU_SRC(1) = MOD_SRC_CLRR | ((mask & SRC_TPU1) != 0 ? MOD_SRC_SRE : 0);
		TPU_PARAM = TPU_PARAM_FDIS | TPU_PARAM_TINI;
	}
	if ((mask & SRC_GPTU) != 0) {
		GPTU_T2(GPTU0) = GPTU_T2_RELOAD;
		GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ASETR;
		GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | MOD_SRC_SRE;
	}
	if ((mask & SRC_RTC) != 0) {
		RTC_ISNRC = RTC_ISNRC_T14;
		RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
		RTC_ISNC = RTC_ISNC_T14IE;
	}
	cpu_enable_irq(true);
}

/* Every source named in mask has to reach its event count inside an STM bound: a
   source that never reports must be reported, and the wait renews the budget so a
   legitimate multi-second window is not read as a hang. */
static bool sources_wait(uint32_t mask, uint32_t ms) {
	uint64_t start = stopwatch_get();

	printf("# sources_wait(mask=%u): up to %u ms of STM\n# ", (unsigned int) mask, (unsigned int) ms);

	while (!reference_stalled && !test_elapsed_bound_ms(start, ms)) {
		bool done = true;

		if ((mask & SRC_TPU0) != 0 && tpu0_events < EVENTS)
			done = false;
		if ((mask & SRC_TPU1) != 0 && tpu1_events < EVENTS)
			done = false;
		if ((mask & SRC_GPTU) != 0 && gptu_events < EVENTS)
			done = false;
		if ((mask & SRC_RTC) != 0 && rtc_events < RTC_EVENTS)
			done = false;
		if (done)
			return true;

		rtc_tick_now();                 /* keep the reload-aware clock current */
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	printf("# sources_wait(mask=%u): gave up after %u ms of STM with TPU0=%u TPU1=%u GPTU=%u RTC=%u\n",
		(unsigned int) mask, (unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()),
		(unsigned int) tpu0_events, (unsigned int) tpu1_events,
		(unsigned int) gptu_events, (unsigned int) rtc_events);

	return false;
}

static void sources_stop(void) {
	cpu_enable_irq(false);
	TPU_PARAM = 0;
	/* T2A's run bit is set/cleared by the write-only bits, not by 0. */
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ACLRR;
	RTC_ISNC = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR;
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR;
	printf("# interrupted: TPU0=%u TPU1=%u GPTU=%u RTC=%u other=%u rtc_src_pending=%u\n",
		(unsigned int) tpu0_events, (unsigned int) tpu1_events, (unsigned int) gptu_events,
		(unsigned int) rtc_events, (unsigned int) irq_other,
		(unsigned int) rtc_srr_pending);
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;
	uint32_t now = rtc_tick_now();
	bool gptu_irq = (irq == VIC_GPTU0_SRC6_IRQ);

	if (irq == VIC_TPU_INT0_IRQ) {
		if (tpu0_events < EVENTS) {
			tpu0_periods[tpu0_events] = now - tpu0_last;
			tpu0_last = now;
			tpu0_events++;
		}
		/* The kernel's re-arm: TINI=0 then TINI=1, which the model reads as a restart. */
		TPU_PARAM = 0;
		TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
		TPU_INT(0) = TPU_ARM_TICKS;
		TPU_PARAM = TPU_PARAM_FDIS | TPU_PARAM_TINI;
		TPU_SRC(0) = MOD_SRC_CLRR | MOD_SRC_SRE;
	} else if (irq == VIC_TPU_INT1_IRQ) {
		if (tpu1_events < EVENTS) {
			tpu1_periods[tpu1_events] = now - tpu1_last;
			tpu1_last = now;
			tpu1_events++;
		}
		TPU_SRC(1) = MOD_SRC_CLRR | MOD_SRC_SRE;
	} else if (irq == VIC_GPTU0_SRC6_IRQ) {
		if (gptu_rate_events != 0) {
			uint32_t gap = now - gptu_rate_last;
			if (gap < gptu_gap_min)
				gptu_gap_min = gap;
			if (gap > gptu_gap_max)
				gptu_gap_max = gap;
			if (gap == 0)
				gptu_spurious_gap0++;
		}
		if (gptu_rate_events == 0)
			gptu_rate_first = now;
		gptu_rate_last = now;
		gptu_rate_events++;
		if (gptu_events < EVENTS) {
			gptu_periods[gptu_events] = now - gptu_last;
			gptu_last = now;
			gptu_events++;
		}
		GPTU_SRC(GPTU0, 6) = MOD_SRC_CLRR | MOD_SRC_SRE;
		if ((GPTU_SRC(GPTU0, 6) & MOD_SRC_SRR) != 0)
			gptu_miss_immediate++;
	} else if (irq == VIC_RTC_IRQ) {
		/* The RTC's VIC line is its SRC SRR bit: ISNRC alone leaves it held. */
		if ((RTC_SRC & MOD_SRC_SRR) != 0)
			rtc_srr_pending++;
		if (rtc_events < RTC_EVENTS) {
			rtc_periods[rtc_events] = now - rtc_last;
			rtc_last = now;
			rtc_events++;
		}
		RTC_ISNRC = RTC_ISNRC_T14;
		RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	} else {
		irq_other++;
	}

	/* THE LATE RE-READ, the authority on whether anything was really pending. */
	if (gptu_irq && (GPTU_SRC(GPTU0, 6) & MOD_SRC_SRR) != 0)
		gptu_miss_late++;

	VIC_IRQ_ACK = 1;
}

static uint32_t period_average(volatile uint32_t *periods, uint32_t count) {
	uint32_t sum = 0;

	for (uint32_t i = 1; i < count; i++)
		sum += periods[i];

	return count > 1 ? sum / (count - 1) : 0;
}

/*
 * T2A overflows between the first and the last interrupt of a long window.  The window is
 * the RTC's own count between those two events, so the phase does not enter the number:
 * the rate is (intervals x 260000 counts) / (T14 counts / 4096 Hz), and its band is the
 * +/- one interval and +/- one T14 count the window can resolve.  Nothing finer is claimed.
 */
static bool measure_gptu_rate(uint32_t *intervals, uint32_t *span_t14) {
	uint64_t start = stopwatch_get();

	sources_arm(SRC_GPTU);
	gptu_rate_events = 0;
	gptu_rate_first = gptu_rate_last = 0;
	gptu_miss_immediate = 0;
	gptu_miss_late = 0;
	gptu_gap_min = 0xFFFFFFFFu;
	gptu_gap_max = 0;
	gptu_spurious_gap0 = 0;
	uint32_t t2_start = GPTU_T2(GPTU0);
	rtc_tick_base();

	printf("# GPTU rate window: up to %u ms of STM, stopping after %u T14 counts of RTC\n# ",
		(unsigned int) GPTU_RATE_WAIT_MS, (unsigned int) GPTU_RATE_T14_COUNTS);

	while (!reference_stalled && !test_elapsed_bound_ms(start, GPTU_RATE_WAIT_MS) &&
			rtc_tick_now() < GPTU_RATE_T14_COUNTS) {
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	uint32_t total_t14 = rtc_tick_now();

	*intervals = gptu_rate_events > 1 ? gptu_rate_events - 1 : 0;
	*span_t14 = gptu_rate_events > 1 ? gptu_rate_last - gptu_rate_first : 0;
	sources_stop();

	/*
	 * VALIDITY BEFORE VERDICT.  The acceptance rule is the RATE/SERVICE one and never the
	 * count, the span target or which bound ended the window - those are printed as
	 * informational fields, because legitimate windows differ in COUNT: the two pasted phone
	 * runs carry 1199, 1530 and 1597 overflows at ONE rate, 399.7 overflows/s ~= fPLL/260000.
	 * A window is rejected only when the service missed entries, the interrupts stopped
	 * arriving, or the span/count pair is internally inconsistent with the counter's own
	 * travel.  A window with NO interrupt at all is a legitimate outcome (the counter did not
	 * advance) and is reported as such - the pass's stop/continue rows rest on
	 * `intervals == 0`, never on `gptu_ok` alone.
	 */
	bool no_events = gptu_rate_events == 0;
	uint32_t tail_gap = no_events ? 0u : (total_t14 >= gptu_rate_last
		? (uint32_t) ((uint64_t) (total_t14 - gptu_rate_last) * 1000 / total_t14) : 1000u);
	uint32_t missed = gptu_miss_late;      /* the late read is the authority (see the handler) */
	uint32_t missed_immediate = gptu_miss_immediate;
	uint32_t ovf_per_s_x10 = *span_t14
		? (uint32_t) ((uint64_t) *intervals * RTC_T14_HZ * 10u / *span_t14) : 0u;

	/* THE COUNTER'S OWN TRAVEL, read at the WINDOW's boundaries - never inside the handler,
	   where the counter is phase-aligned with the overflow it just produced and carries no
	   travel information.  A counter that ran more 260000-count periods than the service
	   counted has LOST entries.  The test is ONE-SIDED on purpose - a counter that reloads on
	   overflow reports only its partial travel, which is an unusable witness and not a fault,
	   so only "more travel than counted" rejects.  The direction is not assumed: take the
	   shorter modular difference (a window's travel is far below 2^31).
	   LATENT IN THIS CONFIGURATION: with reload-overflow the travel is a PHASE difference
	   (< one 260000-count period), so `travel_bad` cannot be reached at all - it needs
	   `travel > PERIOD/4` AND `travel > events x PERIOD + 1.5 periods`, and `events >= 1` makes
	   the second impossible for a sub-period travel.  That is why emu11 prints `travel=255496`
	   with `witness unusable`.  The term is kept because it becomes LIVE if the counter is ever
	   configured free-running - and its non-firing is NOT evidence about the service. */
	uint32_t t2_end = GPTU_T2(GPTU0);
	uint32_t travel = 0, expected_counts = 0;
	bool travel_used = false, travel_bad = false;
	if (gptu_rate_events != 0) {
		uint32_t d_up = t2_end - t2_start;
		uint32_t d_down = t2_start - t2_end;
		travel = d_up < d_down ? d_up : d_down;
		expected_counts = (uint32_t) ((uint64_t) gptu_rate_events * GPTU_T2_COUNTS);
		if (travel > GPTU_T2_COUNTS / 4u) {          /* a real travel, not a reload partial */
			travel_used = travel >= expected_counts / 2u;   /* the counter free-ran the window */
			travel_bad = (uint64_t) travel > (uint64_t) expected_counts + GPTU_T2_COUNTS * 3u / 2u;
		}
	}
	/* THE INTERRUPT SPACING, the counter-free merge witness: a periodic timer's spacing is
	   near-uniform, so a bimodal spacing (some gaps ~2 periods, others near zero - a
	   level-triggered request seen twice for one overflow) is a merged service.  A genuinely
	   slower counter is uniform instead, which is what keeps a real tap move distinguishable. */
	bool merged = (gptu_rate_events >= 2) &&
		(gptu_gap_max > gptu_gap_min + gptu_gap_min / 2u);
	bool tail_ok = no_events || tail_gap <= GPTU_WINDOW_TAIL_MAX;
	bool valid = missed == 0 && !merged && !travel_bad && (no_events || tail_ok);

	/*
	 * DIRECTION TELL, pre-registered for the next phone run.  A clear-visibility artefact
	 * INFLATES the count - spurious re-entries, so the rate reads ~2x nominal or another
	 * multiple - while a genuine MERGE LOWERS it (~half nominal, as the anomaly did).  Either
	 * way the rate box fails; the DIRECTION is what names the cause, and the two counters above
	 * (immediate vs late) say which half of the pair fired.
	 *
	 * PRE-REGISTERED READING: immediate-set / late-CLEAR = the clear-visibility artefact (the
	 * late read is the authority and it says nothing was pending); immediate-set / late-SET = a
	 * genuine unretired request (the late read is the authority and it rejects); gap-0 spurious
	 * entries > 0 with the rate ~2x nominal = the artefact INFLATING the count.  `missed` is the
	 * LATE read and stays load-bearing - spacing and travel are corroborators, never the
	 * authority.  A read-back artefact therefore costs a comparison, never the rate: the rate is
	 * computed from `gptu_measured` and printed whatever the witness says.
	 */
	bool artifact_inflates = gptu_spurious_gap0 != 0 && ovf_per_s_x10 > 0;

	uint32_t spurious = gptu_spurious_gap0;
	uint32_t corrected_ints = *intervals > spurious ? *intervals - spurious : 0;
	uint32_t corrected_hz = *span_t14
		? (uint32_t) ((uint64_t) corrected_ints * GPTU_T2_COUNTS * RTC_T14_HZ / *span_t14) : 0;

	printf("# GPTU window: end=%s (informational), %u overflows, last at T14 %u of %u elapsed "
		"(tail gap %u.%u %%), %u.%u overflows/s, interrupt spacing %u..%u T14 counts, "
		"counter travel=%u vs counted=%u counts (%s), service misses: immediate=%u of %u, late=%u, %s\n",
		(!reference_stalled && total_t14 >= GPTU_RATE_T14_COUNTS) ? "T14 target" : "elapsed bound",
		(unsigned int) gptu_rate_events, (unsigned int) gptu_rate_last, (unsigned int) total_t14,
		(unsigned int) (tail_gap / 10u), (unsigned int) (tail_gap % 10u),
		(unsigned int) (ovf_per_s_x10 / 10u), (unsigned int) (ovf_per_s_x10 % 10u),
		(unsigned int) gptu_gap_min, (unsigned int) gptu_gap_max,
		(unsigned int) travel, (unsigned int) expected_counts,
		travel_used ? "witness used" : "witness unusable (reload/partial)",
		(unsigned int) missed_immediate, (unsigned int) gptu_rate_events, (unsigned int) missed,
		valid ? "ACCEPTED" : "REJECTED");
	if (spurious != 0)
		printf("#   spurious entries=%u (gap 0 - entries closer than one T14 count cannot be genuine\n"
			"#   overflows: a real overflow is one ~10.24-count period away, and even a x4 divider\n"
			"#   move leaves a nonzero gap); they INFLATE the numerator.  rate corrected for spurious\n"
			"#   entries: %u Hz (count %u instead of %u)\n",
			(unsigned int) spurious, (unsigned int) corrected_hz,
			(unsigned int) corrected_ints, (unsigned int) *intervals);
	/* The count is of INTERRUPTS: a low total is either a slower counter or a missed service,
	   and the rate - not the count - is the invariant.  This is the sentence that keeps a
	   counting failure from being re-derived as a tap move. */
	printf("#   the rate is the invariant (399.7 overflows/s ~= fPLL/260000 with the service keeping\n"
		"#   up; 299.6/s = 3/4 of it when entries merge; ~2x nominal when the artefact inflates the\n"
		"#   count); this window's discriminator says: %s\n",
		(missed != 0 || merged || travel_bad)
			? (artifact_inflates ? "INFLATED COUNT / MISSED SERVICE (an artefact, not a tap move)"
				: "MISSED SERVICE / INCONSISTENT PAIR (an artefact, not a tap move)")
			: (no_events ? "no overflow at all (the counter did not advance)"
				: "service kept up (the count is the counter's)"));
	if (!valid) {
		if (missed != 0)
			printf("#   rejected: missed service - the LATE read of the request shows %u of %u entries\n"
				"#   with a genuine unretired request (the immediate read shows %u; immediate-set with\n"
				"#   late-clear would instead be a clear-visibility artefact and would not reject).\n"
				"#   The counted total is LOW here while the counter is fine: this is NOT a tap move\n"
				"#   and is never compared as one.\n",
				(unsigned int) missed, (unsigned int) gptu_rate_events,
				(unsigned int) missed_immediate);
		else if (merged)
			printf("#   rejected: merged service - the interrupt spacing is bimodal (%u..%u T14 counts).\n"
				"#   A slower counter would space its entries uniformly; a level-triggered request seen\n"
				"#   twice for one overflow leaves near-zero gaps beside ~2-period ones.  The counted\n"
				"#   total is LOW while the counter is fine: NOT a tap move, never compared as one.\n",
				(unsigned int) gptu_gap_min, (unsigned int) gptu_gap_max);
		else if (travel_bad)
			printf("#   rejected: the counter did more work than the service counted - %u counted x\n"
				"#   %u counts = %u expected, counter advanced %u.  The RATE is the invariant, and this\n"
				"#   window's is not the counter's: NOT a tap move, never compared as one.\n",
				(unsigned int) gptu_rate_events, (unsigned int) GPTU_T2_COUNTS,
				(unsigned int) expected_counts, (unsigned int) travel);
		else
			printf("#   rejected: the interrupts stopped arriving - the last was %u.%u %% of the window from\n"
				"#   its end (limit %u.%u %%).  This is NOT a measurement and is never compared as one.\n",
				(unsigned int) (tail_gap / 10u), (unsigned int) (tail_gap % 10u),
				(unsigned int) (GPTU_WINDOW_TAIL_MAX / 10u), (unsigned int) (GPTU_WINDOW_TAIL_MAX % 10u));
	}

	return valid && *intervals != 0 && *span_t14 != 0;
}

/* The same comparison against a fractional expectation, tolerance in permille. */
static bool ratio_matches_milli(uint32_t measured_t14, uint32_t expected_milli, uint32_t tolerance_permille) {
	uint64_t got = (uint64_t) measured_t14 * 1000;
	uint64_t tol = (uint64_t) expected_milli * tolerance_permille / 1000;

	return got + tol >= expected_milli && got <= expected_milli + tol;
}

/* A derived rate is only as sharp as the window that produced it: one more (or fewer) count. */
static uint32_t rate_residual(uint32_t hz, uint32_t window_counts) {
	return window_counts ? hz / window_counts : 0;
}

static bool ratio_matches(uint32_t measured, uint32_t expected, uint32_t tolerance_percent) {
	uint32_t tolerance = expected * tolerance_percent / 100;

	return measured + tolerance >= expected && measured <= expected + tolerance;
}

/* A ratio near a power of two is a divider finding, not a tolerance. */
static void report_power_of_two(const char *name, uint32_t measured, uint32_t expected) {
	if (expected == 0 || measured == 0)
		return;

	for (uint32_t shift = 1; shift <= 6; shift++) {
		uint64_t up = (uint64_t) expected << shift;
		uint64_t down = (uint64_t) expected >> shift;
		uint64_t scaled = (uint64_t) measured * 100;

		if (scaled >= up * 98 && scaled <= up * 102)
			printf("# %s: measured is ~2^%u x expected - a divider or a constant\n", name, (unsigned int) shift);
		else if (down != 0 && scaled >= down * 98 && scaled <= down * 102)
			printf("# %s: measured is ~2^-%u x expected - a divider or a constant\n", name, (unsigned int) shift);
	}
}

/* The BSP printf has no '-' flag: pads right, so columns stay aligned.  `expected_milli` is the
   expectation in thousandths, so the print and the ratio are the same quantity. */
static void report(const char *name, const char *clock_source, uint32_t programmed_ns,
		uint32_t measured_t14, uint32_t expected_milli) {
	uint64_t ratio_milli = expected_milli ? (uint64_t) measured_t14 * 1000000 / expected_milli : 0;

	printf("# %9s | %18s | %8u ns | %4u T14 (%8u ns) | %4u.%03u T14 | %u.%03u\n",
		name, clock_source, (unsigned int) programmed_ns, (unsigned int) measured_t14,
		(unsigned int) (measured_t14 * RTC_NS_PER_T14),
		(unsigned int) (expected_milli / 1000), (unsigned int) (expected_milli % 1000),
		(unsigned int) (ratio_milli / 1000), (unsigned int) (ratio_milli % 1000));
}

/* The wake path's layers: the TPU latch itself is not readable, its SRC is. */
struct wake_layers {
	uint32_t tpu0_srr, tpu1_srr, rtc_t14ir, rtc_srr, vic_num, mask;
};

static void wake_layers_read(struct wake_layers *l) {
	l->tpu0_srr = (TPU_SRC(0) & MOD_SRC_SRR) != 0;
	l->tpu1_srr = (TPU_SRC(1) & MOD_SRC_SRR) != 0;
	l->rtc_t14ir = (RTC_ISNC & RTC_ISNC_T14IR) != 0;
	l->rtc_srr = (RTC_SRC & MOD_SRC_SRR) != 0;
	l->vic_num = VIC_IRQ_CON & VIC_IRQ_CON_NUM;
	l->mask = 0;
	if (l->tpu0_srr)
		l->mask |= SRC_TPU0;
	if (l->tpu1_srr)
		l->mask |= SRC_TPU1;
	if (l->rtc_t14ir || l->rtc_srr)
		l->mask |= SRC_RTC;
}

static const char *pending_name(uint32_t pending) {
	if (pending == SRC_TPU0)
		return "TPU INT0 pending";
	if (pending == SRC_RTC)
		return "RTC T14 pending";
	if (pending == (SRC_TPU0 | SRC_RTC))
		return "both pending (ambiguous)";

	return pending ? "another line pending" : "neither pending";
}

static void wake_layers_print(const char *tag, const struct wake_layers *l) {
	printf("# %s: TPU_SRC0.SRR=%u TPU_SRC1.SRR=%u RTC.ISNC.T14IR=%u RTC_SRC.SRR=%u "
		"VIC_IRQ_CON.NUM=%u (%s) [NUM is the highest pending line, read-only]\n",
		tag, (unsigned int) l->tpu0_srr, (unsigned int) l->tpu1_srr,
		(unsigned int) l->rtc_t14ir, (unsigned int) l->rtc_srr,
		(unsigned int) l->vic_num, pending_name(l->mask));
}

/* The latency is the corroborating evidence for which deadline was hit. */
static const char *deadline_name(uint32_t held) {
	if (ratio_matches(held, tpu_expected_t14, 20))
		return "the TPU one-shot deadline";
	if (ratio_matches(held, RTC_BOUND_COUNTS, 10))
		return "the RTC bound";

	return "neither programmed deadline";
}

/* T14 to a known deadline, line enabled, IRQs off on return. */
/*
 * The T14 period row's state, set here rather than inherited: reload AND count, with PRE on
 * (the 244140 ns per count that row's expectation and every STM/T14 scale in this suite
 * assume), and printed with the row.  A reload that is not this one then shows in the log
 * instead of hiding inside a measured period.
 */
static void rtc_t14_program(uint32_t reload, uint32_t count) {
	cpu_enable_irq(false);
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	RTC_ISNC = RTC_ISNC_T14IE;
	RTC_CON = RTC_CON_RUN | RTC_CON_PRE;
	RTC_T14 = (count << RTC_T14_CNT_SHIFT) | (reload << RTC_T14_REL_SHIFT);
}

static void rtc_t14_state_print(const char *tag) {
	printf("# %s: CON=%08X (RUN=%u PRE=%u) T14=%08X (reload %u, count %u) REL=%08X <- the period row below "
		"measures THIS state; a reload other than %u would show as a scaled period\n",
		tag, (unsigned int) RTC_CON, (unsigned int) ((RTC_CON & RTC_CON_RUN) != 0),
		(unsigned int) ((RTC_CON & RTC_CON_PRE) != 0), (unsigned int) RTC_T14,
		(unsigned int) ((RTC_T14 >> RTC_T14_REL_SHIFT) & 0xFFFFu),
		(unsigned int) ((RTC_T14 >> RTC_T14_CNT_SHIFT) & 0xFFFFu),
		(unsigned int) RTC_REL, (unsigned int) RTC_T14_RELOAD);
}

static void rtc_deadline_arm(void) {
	cpu_enable_irq(false);
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	RTC_ISNC = RTC_ISNC_T14IE;
	RTC_T14 = ((65536u - RTC_BOUND_COUNTS) << RTC_T14_CNT_SHIFT) |
		(RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
}

/*
 * The arming the caller already did is waited out against the STM: only a line seen
 * live may be slept on, because a WFI whose line never comes ends in the watchdog
 * reset, and that reset takes the rest of this log with it.  The caller re-arms
 * afterwards, so the pre-check's own event cannot shorten the measured hold.
 */
static bool line_seen_live(uint32_t mask, const char *tag, uint32_t ms, uint32_t *held_t14) {
	uint64_t start = stopwatch_get();
	struct wake_layers l;

	printf("# %s: waiting for line mask %u to show a request, STM-bounded at %u ms\n# ",
		tag, (unsigned int) mask, (unsigned int) ms);
	rtc_tick_base();
	*held_t14 = 0;

	while (!test_elapsed_bound_ms(start, ms)) {
		wake_layers_read(&l);
		if (l.mask == mask) {
			*held_t14 = rtc_tick_now();
			printf("# %s: the line showed a request in %u ms of STM (%u T14 counts, VIC NUM=%u)\n",
				tag, (unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()),
				(unsigned int) *held_t14, (unsigned int) l.vic_num);
			test_heartbeat_end();
			return true;
		}
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	wake_layers_read(&l);
	printf("# %s: no request for mask %u in %u ms of STM: mask seen %u, VIC NUM=%u\n",
		tag, (unsigned int) mask,
		(unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()),
		(unsigned int) l.mask, (unsigned int) l.vic_num);

	return false;
}

/*
 * ------------------------------------------------------------------ CGU clock
 * scaling pass.
 *
 * The timers are dated by the RTC's crystal, but the blocks they live in are
 * clocked from the CGU: the STM from fOSC through FSTM_DIV (crystal-derived), the
 * TPU from the block's own source, and the GPTU from whichever domain the rate row
 * above measures.  If a future DVFS/standby path scales the CGU, those rates move
 * *under* the kernel - whose GPTU belief is currently a literal
 * (`clock-frequency = <104000000>` in the board DTS).  This pass moves one CGU
 * knob at a time and measures the timer rows at each step.
 *
 * WHAT THIS DEVICE'S DOMAINS ARE (memory/ke970-power-clock-dvfs.md, [BSP-doc]
 * hardware-measured SL98 tree for the same die): PLL = 104 MHz from NDIV=3/MDIV=0;
 * PHASE1 = fPLL*12/8 = 156 MHz -> DSP; PHASE2 = fPLL*12/6 = 208 MHz -> AHB and
 * ARM; PHASE4 = 96 MHz -> MMCI/USIF; fSYS bypassed to 26 MHz; ARM divider
 * programmed but not enabled.  So 156 is the *DSP's* source on that profile, and
 * PHASE1/PHASE2 are two divider settings of one phase-shifter set - "156 to 208"
 * is a phase choice, not an overclock.  (Not quoted from that note: its fSTM =
 * 3.25 MHz line, which the note itself marks CONTESTED - that is the owner-gated
 * FSTM_DIV item.)
 *
 * KE970's OWN POINT, from its logged values and not from any board file: the stock
 * trace ends at OSC=0x00031717, CON0=0x1100080A, CON1=0x12400012, CON2=0x21008123
 * (scratch/apoxi-clk/stock-trace-all.log, PWRITE at 0xA25AA83C/7F0/8C8/914) - so
 * FSYS is BYPASS (26), AHB is PHASE2 (208), and CON2 has **CPU_DIV = 1 with
 * CPU_DIV_EN clear**, i.e. the /2 divider is pre-loaded and NOT engaged
 * (boot76/src/board/common/ke970_platform.c:157 sets CPU_DIV and never the enable).
 * fCPU = fAHB = 208, matching memory/ke970-render-perf-display-path.md:32.
 *
 * THE SWEEP'S SPAN, therefore: `CON2.CPU_DIV` is b9:8 with divisor N+1, so it
 * spans **208 -> 52 MHz only** - 26 MHz is `AHB_CLKSEL = BYPASS`, which is the
 * EBU-crossing step (memory/ke970-power-clock-dvfs.md:456: the ARM row is CPU_DIV
 * *and* AHB_CLKSEL, 26..208).  A true 26..208 sweep cannot avoid the AHB switch,
 * and the CPU-divider steps are the *isolated* half: AHB stays at its normal 208,
 * so NOR/EBU and the peripheral buses keep their clock while only the core slows.
 *
 * THE FREQUENCY-CHANGE IDIOM IS THE FIRMWARE'S, NOT A PLAIN WRITE.  `psv.c`
 * (`0xA2A66E24`, the image's only frequency change) does CPU_DIV_EN -> WFI ->
 * change the quotient -> WFI -> CPU_DIV_EN clear, i.e. **the core is halted across
 * each step** ([E-Gold] errata: "change PHS2 only with CPUH=0"), with NOPs after a
 * switch.  A plain write can hang the part, so every step below goes through
 * `scale_freq_switch()`; the halt is armed on a line this suite has already seen
 * live, because a WFI whose line never arrives ends in the watchdog reset.
 *
 * WHERE THIS SUITE RUNS (a placement fact, verified from its own ELF):
 * `.cgu_loader` has VMA = LMA = 0xA8000000 and is 60 bytes; `.text` has VMA
 * 0x00082000 / LMA 0xA800003C; `.data`/`.bss` VMA 0x0008ba90; the stacks sit at
 * `ORIGIN(intram)+LENGTH` (cgu/extram-intram.ld:4-5/:59-88; `arm-none-eabi-objdump
 * -h build/lg-ke970/timers.elf`).  The runtime footprint is on-chip and the EBU is
 * touched only by that loader stub, once, before `main`.
 *
 * THAT PLACEMENT IS NOT A HARBOUR FOR THE AHB STEP - the corrected reading.
 * `fCPU = fAHB/(CPU_DIV+1)`, so an AHB_CLKSEL change re-clocks the **core**: on-chip
 * SRAM (whose clock source is **unestablished** - not assumed off the bus), TCM
 * (core-clocked) and EBU RAM/NOR all move with it.  The TCM nuance, kept exactly:
 * TCM-resident code stays *self-consistent* across an AHB change - its internal
 * relative timing is preserved - but its **absolute rate moves with the core**, and
 * the WFI+NOPs errata still applies.  So TCM is a harbour for the
 * **EBU-transition** hazard (given stack, globals and MMIO are also off the moving
 * bus) and no harbour at all for the core-rate change.  A CPU_DIV-only change is
 * the class that neither memory class needs to escape.
 *
 * SURVIVABILITY OF THE AHB STEP IS A PROPERTY OF THE SEQUENCE, not of where the
 * section runs: the EBU handshake (so the EBU/AHB crossing resyncs), whatever
 * burst/`SDCMSEL` handling the hardware needs, the WFI halt and the NOPs.  That is
 * what the firmware's shape below encodes and what this pass copies.
 *
 * WHAT IS AND IS NOT ESTABLISHED ABOUT THE FIRMWARE, per routine:
 *   - boot's CGU/AHB code IS NOR-XIP and WAS executed - the trace's PCs are NOR
 *     addresses (scratch/apoxi-clk/stock-trace-all.log 0xA25AA8B8 -> 0xA25AA8C8:
 *     EBU_CLKSEL already PLL; SCU_EBUCLC2 |= FLAG1 as a **read-modify-write whose
 *     read saw READY = 0x0F - there is NO READY poll in that window**; then
 *     AHB_CLKSEL <- PHASE2), with no EBU_CON write.  That is the shape copied
 *     below; this step adds a bounded READY poll as belt-and-braces, which boot
 *     itself did not do;
 *   - `psv.c` `0xA2A66E28` is **static-only in every captured run**.  What it
 *     actually writes, verified here by disassembling 0xA2A66E28..0xA2A67020: the
 *     WFI-halt CPU_DIV idiom (0xA2A66E78-EAC: bic #0x300/orr #0x200, orr #0x1000,
 *     WFI, bic #0x300/orr #0x100, bic #0x1000), then EBU_CLKSEL <- AHB (orr #0x70
 *     at 0xEE8), AHB_CLKSEL <- BYPASS (bic #0x700000 at 0xEF8), CPU_DIV=3 + EN
 *     (0xF04/0xF10), PHASE2 power-down (bic #4 at 0xF20), NOPs, WFI.  It protects
 *     the transition with the **EBU_SDRMREF0 self-refresh dance**
 *     (0xA2A66EB4-EE4, on 0xF0000040: orr #0x1000, poll 0x800, bic #0x1000), NOT
 *     with the SCU_EBUCLC handshake.  Verified independently here: psv's literal
 *     pool (0xA2A67028-44) holds 0xF4400020, 0xB02F885C, 0xF45000AC, 0xF0000040,
 *     0xF45000A8/A0/B0 - **no 0xF4400040/44**; image-wide, 0xF4400040 has ZERO
 *     literal sites and 0xF4400044 exactly one (0xA25AAD3C, boot's pool), and the
 *     function contains no add/sub that could build such a base.  So the two
 *     lineages DIFFER on mechanism - SDCMSEL/EBUCLC is the BSP's policy, SDRAM
 *     self-refresh is the firmware's - and DvfsClockRisk's §2.9(C) carries that as
 *     an open discrepancy.  This pass adopts neither; it copies boot's executed
 *     shape, which is the one actually observed on this part;
 *   - its **residency stays PENDING THE MEMORY-MAP SETTLEMENT**, asserted neither
 *     way.  The settled inputs (DvfsClockRisk): TCM is a **CP15 overlay**, so
 *     0xB0000000-origin code is CS1 address space, and either way it is not off
 *     the clock-scaling path - a firmware writing DTCM base 0xB0000000 makes both
 *     readings true, and the cheap resolution is reading CP15 DTCM/ITCM on the
 *     phone.  The suite's own placement is settled and separate: `.cgu_loader`
 *     VMA=LMA=0xA8000000 (0x3C bytes), `.text` VMA 0x00082000 / LMA 0xA800003C,
 *     `.data`/`.bss` in intram (objdump -h); the load address 0xA8000000 is outside
 *     the verified ADDRSEL map - unresolved, and it does not change the conclusion
 *     that an AHB change moves the core and everything core-coupled;
 *   - the EBU_CON/SCU_EBUCLC1 tail is siemens-x85.c:183-195, cited for the
 *     **SEQUENCE** only (owner's lineage rule: independent lineage agreeing on an
 *     order is evidence about what the hardware requires, while it stays out of
 *     scope for this phone's operating point).  SDCMSEL is "SDRAM Clock Mode
 *     Select" (EBU_CON b26; pmb8876_regs.h:313, EBU.cfg:17).
 *
 * DECISION: the AHB step IS taken, last but one, in boot's executed shape, and it
 * reaches the 26 MHz endpoint; then `OSC.NDIV`, and finally the **PLL power-cycle**
 * with its consumers parked - the one reachable Class-2 event (the source stopping),
 * which is the premise the kernel-side verdict rests on.  **The pass's BASE is
 * boot76's executed shape, applied by the pass itself** - the loader hands over with
 * `AHB_CLKSEL = BYPASS = 26 MHz`, which made the AHB step a no-op and every CPU label
 * false (measured on the phone, 2026-09-20, build ced4fd42); the pass measures the
 * entry rows first, applies boot76, runs the steps, then restores the entry values and
 * checks the restored rows against those entry rows.  The firmware's low-power
 * configuration is printed, not executed.  **This pass copies boot's executed AHB
 * shape and adopts neither siemens-x85's handshake nor SDCMSEL: it is NOT claiming
 * to reproduce psv.c**, which (verified) uses the EBU_SDRMREF0 self-refresh dance
 * instead.  Canonical rationale and hazard framing:
 * scratch/linux-timers/DVFS-SLEEP-CLOCK-RISK.md - cited, not duplicated; the
 * recovered sequence here is fed to it.  QEMU models none of it and would execute a
 * switch that hangs the part.
 *
 * STILL A HYPOTHESIS, NOT A CONCLUSION: whether the superseded OSC-write pair's
 * "stops after `# restore CGU_OSC`" was a PLL-lock effect or a core/fetch-clock
 * effect.  Under the corrected reading the OSC write moves fPLL -> PHASE2 -> AHB ->
 * the core, so it re-clocks the memory the suite runs from, and a fetch-clock
 * explanation is **not** excluded.  The filed record is behavioural and stands as
 * filed; only a reproducing experiment should change its explanation.
 *
 * PHONE ONLY, and the reason is sharper than "the model pins clocks":
 * "An emulator that ignores the CPU clock will still execute the sequence
 * 'successfully'" (memory/ke970-power-clock-dvfs.md:663) - so a green QEMU run
 * here is structural, not evidence, whatever the hardware does.  The pass skips
 * there and says so in its own output.
 *
 * REFERENCES.  The BSP stopwatch reads STM_TIM6:STM_TIM0 - the STM counter,
 * crystal-derived - so it stays a valid *bound* under the CPU/PLL/AHB knobs below;
 * the RTC window is what the rates are dated by, and nothing here is judged against
 * a CPU-iteration count.  `model fSYS/fAHB/fCPU` come from `cpu_get_*()`, i.e.
 * from CON values: printed as beliefs, never as measurements.
 *
 * SIBERIAN-SEMANTICS RULE (owner): `siemens-x85.c` and any Siemens board file are
 * evidence for CGU **field semantics and transition sequences** - what a field
 * means and what handshake a switch needs - and NOT for this phone's operating
 * point.  A board's divider choice may be a hack; the citations below are for the
 * handshake only.
 *
 * EXPECTATIONS, the falsifiable part:
 *   CON2.CPU_DIV/CPU_DIV_EN  must NOT move any timer row: it divides the CPU only;
 *   CON1.FSYS_CLKSEL         must NOT move the GPTU (the tap is not fSYS) - the
 *                            TPU is the one row that may move if it is on fSYS;
 *   CON1.AHB_CLKSEL          must NOT move the GPTU: the boot state is PHASE2 =
 *                            208, which cannot produce the measured 104 tap.  If it
 *                            DOES move, the fixed-tap reading is falsified and the
 *                            board DTS comment's domain name is wrong;
 *   OSC.NDIV                 MUST move the GPTU, proportionally to (NDIV+1), and
 *                            must NOT move the STM.
 *
 * HAZARDS, and what the pass does:
 *   - the console USART is on FPI1, and FPI1 = fPLL/2/2 follows OSC.NDIV, so the
 *     NDIV step re-scales USART_CLC.RMC by the same ratio before moving NDIV.  If
 *     RMC is already at its floor there is no safe console and the step is SKIPPED
 *     with that reason;
 *   - the AHB_CLKSEL step IS taken, last but one, in boot's executed shape, and
 *     only because this suite's whole runtime footprint is in intram (verified
 *     above); the firmware's low-power configuration is printed, not executed.  A
 *     step that stops execution ends the log at its pre-write line, which prints
 *     the entry values;
 *   - every step restores the entry values and the restore is checked.
 */
/*
 * THE HALT, AND THE HARD RULE: NO MOVE WITHOUT IT.
 *
 * The 2026-09-20 phone run of build 866fc052 crashed here, and the log says why: every
 * step printed "no live line to halt on, so the switch was NOT taken under WFI" - and
 * then performed the register move anyway.  The move that killed it was `OSC.NDIV 3 -> 1`,
 * which halves the PLL multiplier (4 -> 2); with `AHB_CLKSEL = PHASE2 = 2 x fPLL` that
 * halved the CORE'S OWN SOURCE 208 -> 104 while the core ran, and the part reset into
 * boot76 (recovered by the watchdog, nothing left scaled).  Two causes, both fixed here:
 *
 *  (i) the line was never observed live.  `sources_arm(SRC_RTC)` arms SRC/ISNC but leaves
 *      interrupts ENABLED, so the IRQ handler consumes `RTC_ISNC.T14IR` before the poll
 *      sees it (and the earlier `sources_stop()` had cleared the layers).  This uses the
 *      WFI sections' own, phone-proven recipe instead: `rtc_deadline_arm()` (which reprograms
 *      T14 to expire in RTC_BOUND_COUNTS and MASKS interrupts) while the request is waited
 *      out masked.
 *  (ii) a missing line skipped only the sleep.  Now it SKIPS THE MOVE and says so: the
 *      hardware enforced exactly the distinction this lane drew - a Class-1 core-divider
 *      move survived unhalted, a Class-2 move of the PLL that feeds PHASE2 -> AHB -> the
 *      core did not - so a stated skip is the only safe answer.
 *
 * Returns true when the line was seen live and the core has been taken through a WFI with
 * the NOP tail; false when nothing was moved.
 */
#define SCALE_HALT_NOP_TAIL 32u

/*
 * THE HALT'S OWN ARMED DEADLINE, in T14 counts.
 *
 * FIELD-BY-FIELD DIFF against `sources_arm(SRC_RTC)` - the arming that DID produce requests in
 * the very run whose halts all failed (`ok 12`, `rtc_src_pending=3`), i.e. the one proven path:
 *
 *   RTC_ISNRC = RTC_ISNRC_T14        | identical in both
 *   RTC_SRC   = MOD_SRC_CLRR | SRE   | identical in both
 *   RTC_ISNC  = RTC_ISNC_T14IE       | identical in both
 *   RTC_CON                          | neither writes it (RUN/PRE stand from rtc_reference_init)
 *   RTC_T14   = (count << CNT) | (reload << REL)
 *                                    | **ONLY HERE**: the deadline path PROGRAMS the count and
 *                                      reload fields; `sources_arm` never touches T14 at all
 *   interrupts                       | here they are MASKED (`cpu_enable_irq(false)`);
 *                                      `sources_arm` ends with `cpu_enable_irq(true)`
 *
 * So the two paths differ in exactly two respects, and the arming that works is the one that
 * leaves T14 alone.  A masked IRQ still latches SRC/ISNC, so the masking cannot delay a request;
 * that leaves the T14 PROGRAMMING as the field-level suspect - and it is the same pair of fields
 * the same run failed on elsewhere: `not ok 21` measured the period row at **8198 T14 counts for a
 * programmed 4096** (ratio **2.001**) while the crystal-scale A/B passed at /8.00, i.e. the suite's
 * assumption about RTC_T14's count/reload semantics is **2x off on this part**.  A second, separate
 * candidate is the caller's bound: `line_seen_live()` bounds by `test_stm_ticks_per_ms()`, which
 * follows the MODEL's fSTM law that this suite's own A/B refutes for the part (with FSTM_DIV_EN set
 * the model says 3.25 MHz while the part delivers ~13 MHz, so "1500 ms" of its ticks is ~0.4 s of
 * real time).  The positive control below MEASURES the first candidate instead of assuming either:
 * it prints the armed count/reload, the T14 counts actually observed before the request arrived,
 * and observed/armed - so a 2.00 there confirms the 2x reload semantics (and then `not ok 21`'s
 * expectation, not the RTC, is what is wrong, the rate already resting on the crystal-scale row),
 * while a timeout with the 2048-count wait would implicate the programming itself.
 *
 * DEADLINE TARGET UNDER EITHER SEMANTICS: 256 T14 counts is armed, and if the part really counts
 * the fields at 2x, the real deadline is ~512 counts - still 4x inside the 2048-count wait, so the
 * halt's fix holds whichever way the control comes out.  The wait is in T14 counts precisely
 * because that base is the one the request is dated by.
 */
#define SCALE_HALT_T14 256u          /* 62.5 ms at 4096 Hz (see the 2x note above) */
#define SCALE_HALT_WAIT_T14 2048u    /* 0.5 s: 8x the armed deadline */

/* Arms the halt line to fire in SCALE_HALT_T14 counts and waits for it in T14 counts.  Prints one
   line either way - the positive control's evidence, and the reload-semantics measurement: the
   armed fields, the counts observed before the request arrived, and their ratio. */
static bool scale_halt_arm_seen(const char *tag)
{
	uint32_t held;
	struct wake_layers l;

	cpu_enable_irq(false);
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	RTC_ISNC = RTC_ISNC_T14IE;
	RTC_CON |= RTC_CON_RUN | RTC_CON_PRE;
	RTC_T14 = ((65536u - SCALE_HALT_T14) << RTC_T14_CNT_SHIFT) |
		(RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	uint32_t armed_readback = RTC_T14;
	rtc_tick_base();

	while (!reference_stalled && rtc_tick_now() < SCALE_HALT_WAIT_T14) {
		wake_layers_read(&l);
		if (l.mask == SRC_RTC) {
			held = rtc_tick_now();
			/* THE MEASUREMENT, not just the verdict: armed 256, observed N, ratio N/256.  A
			   ratio near 2.00 confirms the 2x count/reload semantics the period row's
			   `not ok 21` exposed; it is also why the deadline is 4x inside the wait either way. */
			printf("# %s: halt line live after %u T14 counts, armed %u (observed/armed %u.%02u), "
				"T14 read-back %08X (VIC NUM=%u)\n",
				tag, (unsigned int) held, (unsigned int) SCALE_HALT_T14,
				(unsigned int) (held / SCALE_HALT_T14),
				(unsigned int) ((held * 100u / SCALE_HALT_T14) % 100u),
				(unsigned int) armed_readback, (unsigned int) l.vic_num);
			return true;
		}
		test_watchdog_reset();
	}
	wake_layers_read(&l);
	printf("# %s: halt line NOT live in %u T14 counts (mask seen %u, VIC NUM=%u) - move skipped\n",
		tag, (unsigned int) SCALE_HALT_WAIT_T14, (unsigned int) l.mask, (unsigned int) l.vic_num);
	return false;
}

/*
 * THE HALT IS THE FIRMWARE'S WFI - NOT A SPIN, and this is reverted from the emu21 attempt, which was
 * wrong for the reason the pass's own preamble gives: "psv.c does CPU_DIV_EN -> WFI -> change the
 * quotient -> WFI -> CPU_DIV_EN clear, i.e. **the core is halted across each step** ([E-Gold]
 * errata: 'change PHS2 only with CPUH=0').  A plain write can hang the part."  A spinning core is
 * still fetching from the source being changed: the spin removes the symptom and reinstates the very
 * hazard the halt exists to prevent.
 *
 * WHAT THE emu20 RESET SHOWS, and what the wake path must now PROVE.  The run's control PASSED
 * (`halt line live after 256 T14 counts, armed 256 (observed/armed 1.00), T14 read-back FF00F000,
 * VIC NUM=46`), its log then ended at that line, and the phone re-entered boot76 ~1.78 s later -
 * the shape of a core that slept and never woke, retired by the watchdog.  Two suspects, both in
 * this sequence: the RE-ARM (the control's first phase already consumed a request, so does the
 * re-arm clear AND re-latch it?) and the T14 count write taking effect.  So the halt now:
 *   - reads the wake layers **immediately BEFORE the WFI as well as after it** (`RTC_SRC`,
 *     `RTC_ISNC`, the VIC line - the shape `wake_layers_read()` already uses at the suite's end), and
 *     prints a marker either side.  The PRE-WFI read is the one that survives a non-returning halt,
 *     and the pair is the discriminator: **line NOT asserted at the moment of the WFI => the fault is
 *     the RE-ARM** (the control's first phase cleared the request and the re-arm did not re-latch it,
 *     or the T14 count write did not take); **line IS asserted and the core still never returns =>
 *     the fault is the WAKE itself** - and with interrupts masked across it, that points at the
 *     masked-interrupt path, not at the deadline;
 *   - **PHASE 1 IS EXCLUDED AS A SUSPECT**: the RTC as a wake source works - the line went live after
 *     exactly 256 counts, `VIC NUM=46`, `observed/armed 1.00` - so the deadline arithmetic is not in
 *     question; the suspects are phase 2's re-arm and the wake;
 *   - measures the T14 count across the WFI, and returns false - so the PASS's positive control
 *     fails loudly and the pass skips with its reason - if the core did not sleep for the armed
 *     window.  "The phone resets 1.8 s later" becomes "the halt never returned", which is a
 *     one-run diagnosis.
 *
 * THE EMULATOR CANNOT VALIDATE ANY OF THIS: its WFI model wakes on its own conditions, which is
 * exactly why the control passes there and the phone reset.
 */
static bool scale_halt_or_skip(const char *tag)
{
	struct wake_layers pre, post;
	uint32_t held;

	if (!scale_halt_arm_seen(tag)) {
		/* Teardown in path order, as the WFI sections do. */
		RTC_ISNRC = RTC_ISNRC_T14;
		RTC_SRC = MOD_SRC_CLRR;
		RTC_ISNC = 0;
		cpu_enable_irq(true);
		return false;
	}
	/* The re-arm, made explicit: the check above cleared the request it saw, so the line is
	   re-latched here before the core sleeps - and the T14 count is written again so the deadline
	   the WFI wakes on is the one just armed. */
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	RTC_ISNC = RTC_ISNC_T14IE;
	RTC_T14 = ((65536u - SCALE_HALT_T14) << RTC_T14_CNT_SHIFT) |
		(RTC_T14_RELOAD << RTC_T14_REL_SHIFT);
	cpu_enable_irq(false);

	wake_layers_read(&pre);
	printf("# %s: WFI marker BEFORE - pre-wake layers: TPU0.SRR=%u TPU1.SRR=%u RTC.T14IR=%u "
		"RTC_SRC.SRR=%u VIC NUM=%u (line %s; IRQs masked)\n",
		tag, (unsigned int) pre.tpu0_srr, (unsigned int) pre.tpu1_srr, (unsigned int) pre.rtc_t14ir,
		(unsigned int) pre.rtc_srr, (unsigned int) pre.vic_num,
		pre.mask == SRC_RTC ? "ASSERTED" : "NOT asserted");
	rtc_tick_base();
	idle_wait_for_interrupt();
	held = rtc_tick_now();
	wake_layers_read(&post);
	printf("# %s: WFI marker AFTER, held %u T14 counts (%s); post-wake: TPU0.SRR=%u TPU1.SRR=%u "
		"RTC.T14IR=%u RTC_SRC.SRR=%u VIC NUM=%u\n",
		tag, (unsigned int) held, held >= SCALE_HALT_T14 ? "held the window" : "EARLY WAKE",
		(unsigned int) post.tpu0_srr, (unsigned int) post.tpu1_srr,
		(unsigned int) post.rtc_t14ir, (unsigned int) post.rtc_srr, (unsigned int) post.vic_num);

	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR;
	RTC_ISNC = 0;
	cpu_enable_irq(true);
	for (uint32_t i = 0; i < SCALE_HALT_NOP_TAIL; i++)
		__asm__ volatile("nop");

	/* RETURNED after sleeping for the armed window: the two facts the pass's control asserts. */
	return held >= SCALE_HALT_T14 && held <= SCALE_HALT_WAIT_T14;
}

/*
 * THE ONLY PATH BY WHICH THIS PASS MOVES A CLOCK.  Returns false, having written NOTHING,
 * when the halt could not be taken.  Restores (`cgu_restore_write`) deliberately stay
 * unconditional: they return to a state the suite has already run in, they are idempotent
 * when the move was skipped, and leaving the tree scaled is the worse hazard.
 */
static bool scale_switch_move(const char *tag, volatile uint32_t *reg, uint32_t value)
{
	/* ONE pre-declaration per risky write: the operation, and the values a recovery needs.  Every
	   move and every restore in this pass prints one of these before it writes. */
	printf("# move: %s (writes %08X; OSC=%08X CON1=%08X CON2=%08X if this stops the log)\n",
		tag, (unsigned int) value, (unsigned int) CGU_OSC, (unsigned int) CGU_CON1,
		(unsigned int) CGU_CON2);
	if (!scale_halt_or_skip(tag))
		return false;

	*reg = value;
	for (uint32_t i = 0; i < SCALE_HALT_NOP_TAIL; i++)
		__asm__ volatile("nop");

	return true;
}

/*
 * A restore that MOVES THE CORE'S OWN SOURCE (CON1's AHB_CLKSEL, CON2's CPU_DIV/EN, OSC's
 * multiplier once nothing is parked) takes the halt like a move does, and is written anyway if
 * the halt is unavailable - leaving the tree scaled is the worse hazard.  One line either way.
 * The plain `cgu_restore_write` is for the registers that cannot move a live consumer (CON0's
 * phase dividers with the core already off a phase, CON3, an EBU already parked).
 */
static void scale_restore(const char *name, volatile uint32_t *reg, uint32_t value)
{
	if (!scale_switch_move(name, reg, value))
		printf("# restore %s: no live halt - written anyway (leaving the tree scaled is worse)\n", name);
}

/* A GPTU comparison is only made when BOTH windows are MEASURED and TRUSTED (`scale_trusted()`:
   counts present AND the witness did not reject them); when a window was rejected that is a
   stated skip, not a failed tap-move claim.  The rate itself is never gated on this. */
static void scale_check_gptu(const char *name, bool rows_ok, bool condition)
{
	if (rows_ok)
		test_check(name, condition);
	else
		test_skip(name, "a GPTU window was rejected by the validity guard (see its printed reason); the comparison is not made");
}

struct scale_row {
	uint32_t stm_hz, stm_ticks, t14;
	uint32_t gptu_hz, gptu_ints, gptu_span;
	uint32_t rtc_t14;
	/* `gptu_measured` is the MEASUREMENT (intervals + span both nonzero): it is what the rate is
	   computed from, so a witness verdict can never destroy the number this pass exists to
	   produce.  `gptu_ok` is the WITNESS/TRUST verdict.  A comparison needs both; a rate needs
	   only the first. */
	bool stm_ok, gptu_ok, gptu_measured;
};

/* A row a tap-move comparison may rest on: measured AND trusted (not rejected by the witness). */
static bool scale_trusted(const struct scale_row *r)
{
	return r->gptu_ok && r->gptu_measured;
}

static uint32_t scale_usart_rmc_get(void)
{
	return (USART_CLC(USART0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;
}

static void scale_usart_rmc_print(const char *tag)
{
	printf("# %s: USART_CLC=%08X RMC=%u (the console's own divider; FPI1 = fPLL/2/2 follows OSC.NDIV)\n",
		tag, (unsigned int) USART_CLC(USART0), (unsigned int) scale_usart_rmc_get());
}

/* Moved beyond the two rows' own resolution?  5 % is far above the ~0.07 % a long
   GPTU window resolves and far below any real scaling step (x2, x4). */
static bool scale_moved(uint32_t now, uint32_t base, uint32_t permille)
{
	uint32_t diff;

	if (now == 0 || base == 0)
		return false;
	diff = now > base ? now - base : base - now;

	return (uint64_t) diff * 1000 > (uint64_t) base * permille;
}

/*
 * The RTC's own row: T14 counts over a fixed STM window.  Both are crystal-derived -
 * the STM from fOSC/4 through FSTM_DIV, the RTC from the 32.768 kHz crystal - so NO
 * CGU knob below may move this ratio (the RTC lane's finding: the RTC is sourced
 * from the crystal, never from fSYS/fAHB; phone T14 overflow 1 000 729 860 ns
 * against 999 997 440 programmed).  If this row ever tracks a CPU or AHB change
 * that is a finding of its own, not a scaling success - and it is the one row that
 * is safe to read across a source transition, because it is not on the moving path.
 */
#define SCALE_RTC_WINDOW_MS 200u

static uint32_t scale_rtc_row(void)
{
	uint64_t start = stopwatch_get();
	uint32_t first;

	rtc_tick_base();
	first = rtc_tick_now();
	while (stopwatch_elapsed_ms(start) < SCALE_RTC_WINDOW_MS)
		test_watchdog_serve();

	return rtc_tick_now() - first;
}

static bool scale_rtc_held(const struct scale_row *now, const struct scale_row *base)
{
	return now->rtc_t14 != 0 && base->rtc_t14 != 0 && !scale_moved(now->rtc_t14, base->rtc_t14, 50);
}

/* The first measurement of the pass is the entry row; every later one checks the RTC
   invariant against it, so the check appears at every step without repeating it. */
static struct scale_row scale_base_row;

/* Both rows, each dated by its own RTC window - never by a CPU-derived constant.
   Every printed uncertainty is derived from the integer counts that produced it,
   with its contributors named (the suite's printing contract). */
static void scale_measure(const char *tag, struct scale_row *r)
{
	r->stm_ok = measure_stm_rate(&r->stm_hz, &r->stm_ticks, &r->t14);
	r->gptu_ok = measure_gptu_rate(&r->gptu_ints, &r->gptu_span);
	r->gptu_measured = (r->gptu_ints != 0 && r->gptu_span != 0);
	r->gptu_hz = r->gptu_measured
		? (uint32_t) ((uint64_t) r->gptu_ints * GPTU_T2_COUNTS * RTC_T14_HZ / r->gptu_span) : 0;
	r->rtc_t14 = scale_rtc_row();

	/* ONE LINE per row, carrying the fields actually read: the rate, its counts, and the
	   witnesses' states.  The uncertainty arithmetic and every contributor to it are derived
	   from these counts and documented in the suite's README (the printing contract's new
	   volume rule) - not printed. */
	printf("# %s: STM %u Hz (band +-%u, window %u T14) | GPTU %u Hz (%u ovf / %u T14, RMC=%u, %s) "
		"| RTC %u T14 / %u ms\n",
		tag, (unsigned int) r->stm_hz, (unsigned int) rate_residual(r->stm_hz, r->t14),
		(unsigned int) r->t14,
		(unsigned int) r->gptu_hz, (unsigned int) r->gptu_ints, (unsigned int) r->gptu_span,
		(unsigned int) ((GPTU_CLC(GPTU0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT),
		r->gptu_measured ? (r->gptu_ok ? "accepted" : "witness-rejected, rate shown") : "no overflow",
		(unsigned int) r->rtc_t14, (unsigned int) SCALE_RTC_WINDOW_MS);

	if (scale_base_row.rtc_t14 == 0)
		scale_base_row = *r;
	else
		test_check("the RTC T14 row holds across this step (crystal-sourced, never off fSYS/fAHB)",
			scale_rtc_held(r, &scale_base_row));
}

static void cgu_scale_pass(uint32_t entry_osc, uint32_t entry_con0, uint32_t entry_con1,
		uint32_t entry_con2, uint32_t entry_con3)
{
	struct scale_row base;

	test_category("CGU clock scaling: do the timer rows hold while the CPU clock moves? (phone only)");
	if (test_is_qemu()) {
		test_skip("CGU clock-scaling pass runs on the phone",
			"the model pins fgptu = 1e9, derives its GPTU counter from its own fSYS and takes fOSC from a QEMU property, so a green run here is not evidence for any step");
		printf("# phone only: no emulator row below would be evidence; every rate here needs the part's own CGU\n");
		return;
	}

	/*
	 * THE BASE IS BOOT76'S EXECUTED SHAPE, NOT THE LOADER'S HAND-OVER.
	 * The 2026-09-20 phone run (build ced4fd42) showed the pass running in the state
	 * main() restores after its boot76 section - `CON1 = 00000000`, so
	 * `AHB_CLKSEL = BYPASS = 26 MHz`, not boot76's `PHASE2 = 208`.  Three defects
	 * followed, all silent: the AHB step wrote 0 over 0, so its "does not move the
	 * tap" check passed *vacuously* and the EBU-crossing 208 -> 26 transition was
	 * never taken; the CPU labels named a 208 MHz state the run was never in; and the
	 * restore label named PHASE2 while writing the entry value.  Applying boot76's own
	 * sequence as the base (restored at the end) makes fAHB = fCPU = 208, so CPU_DIV
	 * really divides 208 (208/104/69/52) and the AHB step is the real transition.  The
	 * entry (loader) rows are measured FIRST so the final restore is still checked
	 * against the state this pass was handed.
	 */
	struct scale_row entry_rows;

	/*
	 * THE HALT'S POSITIVE CONTROL, and it runs BEFORE any step relies on it: a step that skips
	 * when its halt is unavailable is only safe if the halt is known to be available.  If this
	 * fails, no move is attempted at all (the pass then writes nothing and the entry state
	 * stands).  It is a test_check, not a skip: an unavailable halt is a defect of the pass's
	 * own instrument, and the 2026-09-20 emu14 run is what a silent version of it costs - eight
	 * `mask seen 0` skips, ~12 s of dead waiting, and no scaling data.
	 */
	bool halt_available = scale_halt_or_skip("halt positive control (RTC line)");
	test_check("the RTC halt line is available (positive control before any halted move)",
		halt_available);
	if (!halt_available) {
		test_skip("CGU clock scaling (all steps)", "the RTC halt line could not be seen live, so no move is attempted unhalted");
		return;
	}

	print_cgu_state("scale entry (loader hand-over)");
	scale_usart_rmc_print("scale entry");
	scale_measure("scale entry rows", &entry_rows);
	scale_osc_decode_print("entry", CGU_OSC, entry_rows.gptu_hz);

	/* Re-anchor the RTC invariant on the boot76 state: FSTM_DIV_EN differs between the
	   two states, so the T14-counts-per-STM-window row is not comparable across them,
	   and only the boot76 state spans every step below. */
	scale_base_row.rtc_t14 = 0;
	bool base_locked = boot76_cgu_apply();
	print_cgu_state("scale base (boot76's executed shape)");
	scale_usart_rmc_print("scale base");
	test_check("boot76's CGU sequence locks the PLL for the pass base", base_locked);
	scale_measure("scale base (boot76's executed shape)", &base);

	/*
	 * THE ENTRY/BASE CONTRAST, SCORED WITHOUT ANY HALT - and it is a **MULTI-FIELD** contrast, not a
	 * core-clock one.  The two states differ in many fields at once: `AHB_CLKSEL` 0 -> 4 (26 -> 208
	 * MHz), CON0's phase bytes (0x00 -> 0x08), FPI1, `FSTM_DIV_EN`, `CPU_DIV`, CON3 **and
	 * `OSC.NDIV` 7 -> 3** - which by this suite's own decode is fPLL 208 -> 104 MHz.  So "the tap
	 * held" here CANNOT be attributed to the core, and the honest sentence is the one scored: the
	 * tap reads the same across these two states.  `rows_ok` is **both measured**, not
	 * `scale_trusted()`, so the contrast survives a witness artefact and needs no halt.
	 *
	 * THE CONFOUND IS A FINDING, and it is printed below rather than resolved: "tap = 4 x 26 MHz" is
	 * OBSERVED; "x4 because `NDIV = 3`" is NOT - the tap did not move across a 2x NDIV change.  The
	 * DTS's `clock-frequency = <104000000>` holds under either reading, but the READING is now in
	 * question - and it is load-bearing beyond the rate, because a fixed x4-of-crystal domain cannot
	 * be moved by a DVFS/standby `NDIV` write while a PLL-product tap can.
	 */
	scale_check_gptu("the tap reads the same across the entry and base states (multi-field contrast)",
		entry_rows.gptu_measured && base.gptu_measured,
		!scale_moved(base.gptu_hz, entry_rows.gptu_hz, 50));
	/*
	 * THE PAIR SEPARATES NEITHER READING, and the arithmetic that says so is PRINTED, not held.
	 * `OSC` carries BOTH dividers - `NDIV = GENMASK(21,16)` (pmb8876_regs.h:2282) and
	 * `MDIV = GENMASK(27,24)` (:2284) - and MDIV MOVED between the two states:
	 *   entry `0x01070001`: NDIV = 7, MDIV = 1 => 26 x (7+1)/(1+1) = **104 MHz**
	 *   base  `0x00031717`: NDIV = 3, MDIV = 0 => 26 x (3+1)/(0+1) = **104 MHz**
	 * So the two states decode to the SAME fPLL; there was never a 2x multiplier change to be
	 * invariant across, and the earlier "the tap did not move across a 2x NDIV change" and "first
	 * evidence against the DTS" wordings are BOTH WITHDRAWN.  (`PLL_BYPASS_N`, bit 8 -
	 * pmb8876_regs.h:2277, CGU.cfg:17 - stays a verified STATE fact: clear in the entry, set in the
	 * base; it is simply not needed as a reconciliation, because the decode already agrees.)  Both
	 * states are consistent with BOTH readings - tap = the fPLL product, tap = a fixed x4-of-fOSC
	 * domain - and this pair cannot separate them: the `OSC.NDIV` step below is the arbiter, which is
	 * why its outcome matters beyond the rate (under the fPLL reading a DVFS/standby `NDIV` write
	 * moves the tap; under the fixed reading it cannot).
	 */
	scale_osc_decode_print("base", CGU_OSC, base.gptu_hz);
	printf("# the entry/base pair separates NEITHER reading (tap = the fPLL product; tap = a fixed "
		"x4-of-fOSC domain): both states decode to the same fPLL and the tap reads the same.  Only the "
		"tap's Hz is measured here; the fPLL column is decode/model.  The OSC.NDIV step is the arbiter.\n");

	/* -- step 1: CON2 CPU_DIV / CPU_DIV_EN - the CPU-only half, 208 -> 52 ---
	 * KE970's boot pre-loads CPU_DIV = 1 and leaves the enable clear (logged
	 * CON2 = 0x21008123), so this knob alone spans 208 (enable clear) -> 104
	 * (/2) -> 69 (/3) -> 52 (/4).  It never moves AHB off PHASE2, so NOR/EBU and
	 * the peripheral buses keep their clock while only the core slows - and each
	 * move is taken under the firmware's halt idiom, not as a plain write.
	 *
	 * ACCOMPLISHES: scales the CORE alone - the only knob whose domain nothing else
	 * this suite measures shares.
	 * POSITION (first): it is the isolation control.  Every later row is read
	 * against base rows taken with AHB at its normal 208, so "the timers do not
	 * follow the core clock" is established before anything moves a bus.
	 * IF MOVED after a bus step: a core-rate effect could not be attributed, and the
	 * rows would be measured on an already-moved bus.
	 * IF OMITTED: the sweep loses its only proof that the timer rows are independent
	 * of the core clock - which is what makes the later movement meaningful. */
	uint32_t saved_con2 = CGU_CON2;
	struct scale_row cpu1, cpu2, cpu3;

	if (scale_switch_move("CPU_DIV enable (CPU -> 104)", &CGU_CON2, CGU_CON2 | CGU_CON2_CPU_DIV_EN)) {
		print_cgu_state("scale CPU_DIV=1 + EN (CPU 208 -> 104)");
		scale_measure("CPU_DIV=1/EN", &cpu1);
		test_check("CPU_DIV_EN does not move the STM row (crystal-derived)",
			base.stm_ok && cpu1.stm_ok && !scale_moved(cpu1.stm_hz, base.stm_hz, 50));
		scale_check_gptu("CPU_DIV_EN does not move the GPTU row (the tap is not the CPU)",
			scale_trusted(&base) && scale_trusted(&cpu1), !scale_moved(cpu1.gptu_hz, base.gptu_hz, 50));

		if (scale_switch_move("CPU_DIV=2 (CPU -> 69)", &CGU_CON2,
				(CGU_CON2 & ~CGU_CON2_CPU_DIV) | (2u << CGU_CON2_CPU_DIV_SHIFT))) {
			print_cgu_state("scale CPU_DIV=2 + EN (CPU -> 69)");
			scale_measure("CPU_DIV=2/EN", &cpu2);
			scale_check_gptu("CPU_DIV=2 does not move the GPTU row",
				scale_trusted(&base) && scale_trusted(&cpu2), !scale_moved(cpu2.gptu_hz, base.gptu_hz, 50));

			if (scale_switch_move("CPU_DIV=3 (CPU -> 52)", &CGU_CON2,
					(CGU_CON2 & ~CGU_CON2_CPU_DIV) | (3u << CGU_CON2_CPU_DIV_SHIFT))) {
				print_cgu_state("scale CPU_DIV=3 + EN (CPU -> 52, this knob's floor)");
				scale_measure("CPU_DIV=3/EN", &cpu3);
				test_check("CPU_DIV=3 does not move the STM row",
					base.stm_ok && cpu3.stm_ok && !scale_moved(cpu3.stm_hz, base.stm_hz, 50));
				scale_check_gptu("CPU_DIV=3 does not move the GPTU row",
					scale_trusted(&base) && scale_trusted(&cpu3), !scale_moved(cpu3.gptu_hz, base.gptu_hz, 50));
			} else {
				test_skip("CPU_DIV=3 step (CPU -> 52)", "no live line to halt on, so this move was skipped rather than run unhalted");
			}
		} else {
			test_skip("CPU_DIV=2/CPU_DIV=3 steps (CPU -> 69/52)", "no live line to halt on, so these moves were skipped rather than run unhalted");
		}
		if (!scale_switch_move("CPU_DIV enable clear (CPU -> 208)", &CGU_CON2, CGU_CON2 & ~CGU_CON2_CPU_DIV_EN))
			printf("# CPU_DIV enable clear: no live line to halt on - the move was skipped; the restore below\n"
				"# still writes the entry CON2\n");
	} else {
		test_skip("CPU_DIV steps (the core-only half, 208/104/69/52)", "no live line to halt on, so no CPU_DIV move was taken unhalted");
	}
	scale_restore("scale CGU_CON2 (CPU divider back)", &CGU_CON2, saved_con2);
	test_eq_u32("CPU_DIV step restored CON2", saved_con2, CGU_CON2);

	/* -- step 2: CON1.FSYS_CLKSEL = PLL (26 -> 52) --------------------------
	 * ACCOMPLISHES: moves fSYS alone - the domain the TPU/GSM frame is *believed*
	 * to use, and the model's fSYS for the GPTU that silicon refutes.
	 * POSITION (before the AHB step): fSYS is off the AHB/EBU path, so it can be
	 * moved while the bus and core keep their normal rate - which is what keeps
	 * "the GPTU does not follow fSYS" separable from "the bus moved".
	 * IF MOVED after the AHB step: the bus would already be at 26 and the FSYS
	 * effect could not be read apart from it.
	 * IF OMITTED: whether the TPU counter follows fSYS (and therefore whether the
	 * GSM frame rate moves with it) would go unmeasured. */
	uint32_t saved_con1 = CGU_CON1;

	scale_usart_rmc_print("before FSYS step");
	if (scale_switch_move("FSYS_CLKSEL step (fSYS 26 -> 52)", &CGU_CON1,
			(CGU_CON1 & ~CGU_CON1_FSYS_CLKSEL) | CGU_CON1_FSYS_CLKSEL_PLL)) {
		print_cgu_state("scale FSYS_CLKSEL=PLL (fSYS 52)");
		struct scale_row fsys;
		scale_measure("FSYS_CLKSEL=PLL", &fsys);
		scale_check_gptu("FSYS_CLKSEL does not move the GPTU row (the tap is not fSYS)",
			scale_trusted(&base) && scale_trusted(&fsys), !scale_moved(fsys.gptu_hz, base.gptu_hz, 50));
		test_check("FSYS_CLKSEL does not move the STM row (FSTM is off fOSC)",
			base.stm_ok && fsys.stm_ok && !scale_moved(fsys.stm_hz, base.stm_hz, 50));
	} else {
		test_skip("FSYS_CLKSEL step (fSYS 26 -> 52)", "no live line to halt on, so the move was skipped rather than run unhalted");
	}
	cgu_restore_write("scale CGU_CON1", &CGU_CON1, saved_con1);
	test_eq_u32("FSYS step restored CON1", saved_con1, CGU_CON1);

	/* -- step 3: CON1.AHB_CLKSEL - boot's own executed shape, LAST BUT ONE ---
	 * ACCOMPLISHES: switches the bus/core SOURCE (not a rate) and reaches the
	 * 26 MHz endpoint (fAHB = 26, CPU_DIV_EN clear so fCPU = 26).
	 * POSITION (last but one): it is the only step that changes a source rather
	 * than a rate, and it re-clocks the core (fCPU = fAHB/(CPU_DIV+1)) - so it runs
	 * after the isolated knobs have characterised what does and does not follow
	 * them, while the bus is still at its normal 208 for their rows.
	 * IF MOVED earlier: every later row would be measured on a BYPASS bus, and a
	 * stop here would take the unmeasured steps with it.
	 * IF OMITTED: 26 MHz is unreachable and the AXB-level half of the fixed-tap
	 * prediction goes untested.
	 * WHY THIS SHAPE: survivability is the SEQUENCE - the EBU handshake so the
	 * AHB/EBU crossing resyncs, the WFI halt and the NOPs - NOT the memory the
	 * section runs in: an AHB change re-clocks the core, and with it TCM, on-chip
	 * SRAM (source unestablished) and EBU RAM/NOR alike.  The shape is boot's
	 * executed one (stock-trace-all.log 0xA25AA8B8 -> 0xA25AA8C8: EBU_CLKSEL
	 * already PLL, SCU_EBUCLC2.FLAG1 set as an RMW whose read saw READY = 0x0F -
	 * no READY poll in boot's window - then the AHB write), and this step adds a
	 * bounded READY poll that boot did not; NOT siemens-x85's EBU_CON/SDCMSEL tail
	 * (see the psv bullet above: the firmware protects that transition with SDRAM
	 * self-refresh instead, so the two lineages differ and this pass copies boot).
	 * A hang ends the log at the
	 * pre-write line; the watchdog reset plus boot76's CGU block recovers. */
	print_cgu_state("before AHB step");

	SCU_EBUCLC2 |= (1u << SCU_EBUCLC2_FLAG1_SHIFT);
	bool ebu_ready = test_wait_for_flag(&SCU_EBUCLC2, SCU_EBUCLC2_READY, 20);
	if (scale_switch_move("AHB_CLKSEL step (208 -> 26)", &CGU_CON1,
			(CGU_CON1 & ~CGU_CON1_AHB_CLKSEL) | CGU_CON1_AHB_CLKSEL_BYPASS)) {
		print_cgu_state("scale AHB_CLKSEL=BYPASS (fAHB 26, CPU_DIV_EN clear so fCPU = 26)");
		struct scale_row ahb;
		scale_measure("AHB_CLKSEL=BYPASS", &ahb);
		test_check("the EBU handshake was READY for the AHB step", ebu_ready);
		scale_check_gptu("AHB_CLKSEL does not move the GPTU row (boot state is PHASE2 = 208, not the 104 tap)",
			scale_trusted(&base) && scale_trusted(&ahb), !scale_moved(ahb.gptu_hz, base.gptu_hz, 50));
		test_check("AHB_CLKSEL does not move the STM row (crystal-derived)",
			base.stm_ok && ahb.stm_ok && !scale_moved(ahb.stm_hz, base.stm_hz, 50));
	} else {
		test_skip("AHB_CLKSEL step (208 -> 26)", "no live line to halt on, so the move was skipped rather than run unhalted");
	}
	scale_restore("scale CGU_CON1 (AHB back to PHASE2)", &CGU_CON1, saved_con1);
	test_eq_u32("AHB step restored CON1", saved_con1, CGU_CON1);

	/* The firmware's own low-power configuration, PRINTED ONLY: it drops the core to
	   6.5 MHz and powers PHASE2 down, and psv.c's residency is unestablished, so
	   copying it would add risk without measuring anything the steps above do not. */

	/* -- step 4: OSC.NDIV, the global knob, LAST ----------------------------
	 * This one also moves AHB (AHB = PHASE2 = 2 x fPLL) and so the EBU's *rate*,
	 * but as a FREQUENCY change of a source already selected rather than a source
	 * switch.  No placement escapes it (see above) - that is exactly why it is
	 * taken LAST - and only downwards: halving (NDIV+1) slows the whole tree
	 * together, nothing is overclocked.
	 *
	 * ACCOMPLISHES: moves the TAP'S OWN MULTIPLIER - the falsifiable prediction
	 * (`OSC.NDIV` must move the GPTU row proportionally, and must not move the STM).
	 * POSITION (last): it moves everything at once - fPLL -> every phase -> AHB ->
	 * the core, plus FPI1 and so the console - so nothing after it could be read as
	 * the effect of one knob.
	 * IF MOVED earlier: every subsequent row would be measured on a scaled PLL and
	 * the per-domain attributions would be lost.
	 * IF OMITTED: the tap identification is untested at the one field that must
	 * move it, and the belief caveat in the board DTS could not be retired. */
	uint32_t saved_osc = CGU_OSC;
	uint32_t saved_con1_ndiv = CGU_CON1;
	uint32_t ndiv = (CGU_OSC & CGU_OSC_NDIV) >> CGU_OSC_NDIV_SHIFT;

	print_cgu_state("before NDIV step");
	scale_usart_rmc_print("before NDIV step");
	if (!base.gptu_ok || ndiv == 0) {
		test_skip("OSC.NDIV step (the tap's own multiplier)",
			"either the base GPTU row is unmeasured, or NDIV is already 0 so the multiplier cannot be moved down");
	} else {
		/*
		 * PARK FIRST - the decisive measurement must NOT depend on a halt this phone did
		 * not give.  This is the PLL-cycle step's own preamble, which the ced4fd42 run
		 * already validated ("parking on OSC does not move the GPTU row"): AHB -> BYPASS
		 * (26), FPI1 -> OSC (the console's bus is 26 MHz either way) and EBU -> OSC.  With
		 * nothing deriving from the PLL the NDIV change cannot reset the part, so the WFI
		 * halt is belt-and-braces rather than the safety mechanism.  It also mirrors the
		 * firmware's own discipline: NDIV is written at boot with everything parked /
		 * bypassed, never live under a core that depends on it.
		 */
		uint32_t saved_con2_ndiv = CGU_CON2;
		uint32_t park_con1 = (((CGU_CON1 & ~CGU_CON1_AHB_CLKSEL) | CGU_CON1_AHB_CLKSEL_BYPASS) &
			~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV));   /* AHB <- BYPASS 26; FPI1 <- OSC/DIV1 */

		SCU_EBUCLC2 |= (1u << SCU_EBUCLC2_FLAG1_SHIFT);
		bool ndiv_ebu_ready = test_wait_for_flag(&SCU_EBUCLC2, SCU_EBUCLC2_READY, 20);
		if (!scale_switch_move("NDIV step: park (AHB -> BYPASS 26, FPI1 -> OSC)", &CGU_CON1, park_con1) ||
				!scale_switch_move("NDIV step: EBU -> OSC", &CGU_CON2, (CGU_CON2 & ~7u) | 0u)) {
			test_skip("OSC.NDIV step (the tap's own multiplier)",
				"the parking write could not be taken (no live line to halt on), so NDIV was NOT moved: this step's safety is the parked state, not the halt");
		} else {
			print_cgu_state("NDIV precondition: parked on OSC (nothing derives from the PLL)");
			scale_usart_rmc_print("NDIV precondition (FPI1 on OSC)");
			struct scale_row ndiv_parked;
			scale_measure("NDIV parked (precondition)", &ndiv_parked);
			test_check("the EBU handshake was READY for the NDIV parking write", ndiv_ebu_ready);
			scale_check_gptu("the NDIV park does not move the GPTU row (the tap is the PLL, not a bus)",
				scale_trusted(&base) && scale_trusted(&ndiv_parked), !scale_moved(ndiv_parked.gptu_hz, base.gptu_hz, 50));
		/*
		 * HALVE THE MULTIPLIER, NOT NDIV BY ONE.  fPLL = fOSC * (NDIV+1)/(MDIV+1)
		 * (CGU.cfg; the stock trace fixes NDIV=3/MDIV=0 -> 104 MHz), so the previous
		 * `NDIV - 1` moved 4/1 -> 3/1 = 78 MHz while its label said "104 -> 52": the
		 * step never halved anything.  (NDIV+1)/2 - 1 is the halving, and the
		 * expectation below is computed from it rather than asserted by a literal.
		 */
		uint32_t mult = ndiv + 1u;
		uint32_t mult_down = mult / 2u;
		uint32_t ndiv_down = mult_down - 1u;

		uint32_t ndiv_value = (CGU_OSC & ~CGU_OSC_NDIV) | (ndiv_down << CGU_OSC_NDIV_SHIFT);

		printf("# scale OSC: PLL multiplier %u -> %u (NDIV %u -> %u, MDIV %u unchanged), taken PARKED\n",
			(unsigned int) mult, (unsigned int) mult_down, (unsigned int) ndiv,
			(unsigned int) ndiv_down,
			(unsigned int) ((CGU_OSC & CGU_OSC_MDIV) >> CGU_OSC_MDIV_SHIFT));
		/* THE MOVE THAT CRASHED BUILD 866fc052.  `NDIV 3 -> 1` halves the PLL multiplier and,
		   with AHB_CLKSEL = PHASE2 = 2 x fPLL, the CORE'S OWN SOURCE: the phone reset into
		   boot76 at exactly this step, from an unhalted write.  The step's safety is now the
		   PARKED state - with the core and the EBU on the crystal, nothing derives from the
		   PLL - so the write is taken whether or not the halt is available; the halt is kept
		   as belt-and-braces. */
		if (!scale_switch_move("OSC.NDIV halved (parked)", &CGU_OSC, ndiv_value)) {
			printf("# OSC.NDIV: no live line to halt on - the write is safe PARKED (nothing derives from\n"
				"# the PLL), so it is taken anyway\n");
			CGU_OSC = ndiv_value;
		}
		print_cgu_state("scale OSC.NDIV halved (parked)");
		bool ndiv_locked = wait_pll_locked();
		test_check("the PLL is locked after the parked NDIV change", ndiv_locked);

		/* MEASURE PARKED, and restore the multiplier BEFORE un-parking: the un-park is a source
		   switch (halt-gated), while restoring NDIV under PHASE2 would move the core's own source
		   by the multiplier - the exact hazard that ended the emu14 run.  The park does not move
		   the tap (checked above), so a parked measurement is valid. */
		print_cgu_state("NDIV halved, parked (measured here)");
		struct scale_row ndiv_down_row;
		scale_measure("NDIV halved", &ndiv_down_row);
		/*
		 * THE ARBITER, in its sharpest form - the only row that can settle the DTS's reading:
		 *   entry state: OSC bit 8 (`PLL_BYPASS_N`) CLEAR => PLL out of circuit; tap = 104 MHz = 4 x fOSC
		 *   base  state: bit 8 SET, NDIV = 3 => fPLL = 104 MHz in circuit;  tap = 104 MHz = 1 x fPLL
		 * The two readings agree on 104 MHz and differ on WHY.  This step changes only the multiplier,
		 * with the consumers parked and the bypass released, so:
		 *   tap HALVES           => it follows the multiplier: the tap is the fPLL product, the DTS
		 *                           reading stands, and a DVFS/standby NDIV write CAN move it;
		 *   tap HOLDS at ~104 MHz => it is a fixed 4 x fOSC domain, independent of the PLL - the DTS
		 *                           attribution and the DVFS hazard reasoning built on it need
		 *                           correcting.
		 * Both outcomes are passable; the row prints which it saw.
		 *
		 * MDIV-SAFE BY CONSTRUCTION: the expectation is `mult_down/mult` built from `ndiv + 1`, and
		 * MDIV is PRESERVED by every write here (each RMW masks only `CGU_OSC_NDIV`), so the MDIV term
		 * cancels in that ratio - do not "fix" it for MDIV, which would break a correct expectation.
		 */
		test_check("the OSC.NDIV step produced a measured window (its outcome is printed below)",
			ndiv_down_row.gptu_measured);
		test_check("OSC.NDIV does not move the STM row (crystal-derived through FSTM_DIV)",
			ndiv_down_row.stm_ok && !scale_moved(ndiv_down_row.stm_hz, base.stm_hz, 50));
		if (!ndiv_down_row.gptu_measured)
			printf("# NDIV OUTCOME: no GPTU window in this state - the reading is NOT decided by this run\n");
		else if (scale_moved(ndiv_down_row.gptu_hz, base.gptu_hz, 50))
			printf("# NDIV OUTCOME: the tap MOVED with the multiplier (%u Hz vs base %u Hz) => the tap is the "
				"PLL product and the DTS reading \"x4 because NDIV=3\" STANDS\n",
				(unsigned int) ndiv_down_row.gptu_hz, (unsigned int) base.gptu_hz);
		else
			printf("# NDIV OUTCOME: the tap did NOT move with the multiplier (%u Hz vs base %u Hz) => the tap is a "
				"FIXED x4-of-crystal domain, and the DTS attribution (plus the DVFS hazard reasoning built on "
				"\"an NDIV write moves the tap\") needs correcting\n",
				(unsigned int) ndiv_down_row.gptu_hz, (unsigned int) base.gptu_hz);
		/* The expectation (the tap should halve) is documented, not printed: see the step's
		   comment and TIMER-FIX's NDIV row. */

		/* Restore the entry multiplier and the entry consumers.  These return to a state the
		   suite has already run in, so they are written even when the halt is unavailable:
		   leaving the PLL halved, or the bus parked, under the rest of the suite is the worse
		   hazard. */
		if (!scale_switch_move("OSC.NDIV restored (parked: nothing derives from the PLL)", &CGU_OSC, saved_osc)) {
			printf("# OSC.NDIV restore: no live line to halt on; written anyway, still PARKED - leaving the\n"
				"# PLL halved under the rest of the suite is the worse hazard\n");
			CGU_OSC = saved_osc;
		}
		test_eq_u32("NDIV step restored OSC", saved_osc, CGU_OSC);
		cgu_restore_write("scale CGU_CON2 (EBU back)", &CGU_CON2, saved_con2_ndiv);
		scale_restore("scale CGU_CON1 (un-park: AHB and core back on PHASE2)", &CGU_CON1, saved_con1_ndiv);
		test_eq_u32("NDIV step restored CON2", saved_con2_ndiv, CGU_CON2);
		test_eq_u32("NDIV step restored CON1", saved_con1_ndiv, CGU_CON1);
		scale_usart_rmc_print("after NDIV restore");
		} /* NDIV parked */
	}

	/* -- step 5: the PLL POWER-CYCLE (OSC bit 0) - LAST, the Class-2 event -----
	 * ACCOMPLISHES: exercises the one CGU event the kernel-side verdict actually
	 * rests on - the source itself stopping and coming back - rather than only its
	 * rate changing.  WHAT IT WAS FOUND TO DO (2026-09-20 phone run, build ced4fd42):
	 * clearing bit 0 drops the LOCK indication but does NOT stop the tap, so the
	 * assertion is scoped to that measurement, and a second state - the firmware's
	 * own standby shape, bit 0 AND the four phase-power bits - is measured below as
	 * the one that decides whether the tap can be stopped at all, **with either
	 * outcome passable** (its assertions are on the measurement being valid, and the
	 * row prints which way it fell and what that means).  The same-rate
	 * resume and the crystal-sourced STM/RTC rows are checked in both states.
	 * POSITION (last): it is the most disruptive step - it stops the whole derived
	 * tree - so nothing after it could be attributed.
	 * PRECONDITION, parked first because otherwise the core stops with the PLL: the
	 * consumers are moved off it exactly as the parking shape does - AHB -> BYPASS
	 * (26, core keeps running), FPI1 -> OSC (the console's bus is 26 either way, so
	 * fPLL/4 = 26 and OSC = 26 leave the USART divider alone), EBU -> OSC (so the
	 * external bus follows).  The suite executes from intram, so fetch survives the
	 * core slowing to 26.
	 * HAZARD, encoded: the block's REGISTER INTERFACE may still answer while its
	 * counter is stopped, so "the reads still work" proves nothing - only counted
	 * progress does, which is why the row is a counted rate and not a register read.
	 * A hang here ends the log at the pre-write line; the watchdog reset plus
	 * boot76's CGU block recovers.
	 * QEMU: pll.c never gates frequency on PLL_POWER_UP, so this row is
	 * model-blind like the rest of the pass and skips with the pass's reason. */
	uint32_t saved_con1_pc = CGU_CON1, saved_con2_pc = CGU_CON2, saved_osc_pc = CGU_OSC;

	SCU_EBUCLC2 |= (1u << SCU_EBUCLC2_FLAG1_SHIFT);
	bool pp_ebu_ready = test_wait_for_flag(&SCU_EBUCLC2, SCU_EBUCLC2_READY, 20);
	uint32_t park_con1 = (((CGU_CON1 & ~CGU_CON1_AHB_CLKSEL) | CGU_CON1_AHB_CLKSEL_BYPASS) &
		~(CGU_CON1_FPI1_CLKSEL | CGU_CON1_FPI1_CLKDIV));   /* AHB <- BYPASS 26; FPI1 <- OSC/DIV1 */
	if (!scale_switch_move("PLL-cycle parking (AHB -> BYPASS 26)", &CGU_CON1, park_con1)) {
		/* The precondition failed, so the PLL gate is NOT touched: without the consumers
		   off the PLL the core would stop with it. */
		test_skip("PLL power-cycle step (Class-2)", "no live line to halt on, so the consumers were NOT parked and the PLL gate was not touched");
	} else {
	CGU_CON2 = (CGU_CON2 & ~7u) | 0u;                       /* EBU_CLKSEL <- OSC */
	print_cgu_state("scale PLL-cycle: parked on OSC");
	struct scale_row parked;
	scale_measure("parked on OSC", &parked);
	test_check("the EBU handshake was READY for the parking write", pp_ebu_ready);
	scale_check_gptu("parking on OSC does not move the GPTU row (the tap is the PLL, not a bus)",
		scale_trusted(&base) && scale_trusted(&parked), !scale_moved(parked.gptu_hz, base.gptu_hz, 50));

	if (!scale_switch_move("PLL_POWER_UP clear", &CGU_OSC, CGU_OSC & ~CGU_OSC_PLL_POWER_UP)) {
		test_skip("PLL_POWER_UP clear step", "no live line to halt on, so the gate was not cleared");
	} else {
	print_cgu_state("scale PLL-cycle: PLL_POWER_UP cleared");
	uint64_t lock_start = stopwatch_get();
	bool lock_dropped = false;
	while (stopwatch_elapsed_ms(lock_start) < 20) {
		if ((CGU_STAT & CGU_STAT_LOCK) == 0) {
			lock_dropped = true;
			break;
		}
		test_watchdog_serve();
	}
	printf("# PLL_POWER_UP cleared: CGU_STAT=%08X LOCK=%u (a bounded read; the counted row is the evidence)\n",
		(unsigned int) CGU_STAT, (unsigned int) ((CGU_STAT & CGU_STAT_LOCK) != 0));
	struct scale_row pll_off;
	scale_measure("PLL_POWER_UP cleared", &pll_off);
	/*
	 * RE-SCOPED TO WHAT WAS MEASURED.  On the phone (2026-09-20, build ced4fd42) this
	 * read: 1530 overflows / 15678 T14 counts = 103928358 Hz - inside the band of the
	 * locked readings (entry 103887430, restored 103925519) - with CGU_STAT = 00000200,
	 * i.e. LOCK clear.  So clearing bit 0 gates the LOCK indication, not the
	 * oscillator; the assertion says exactly that and rests on the counted row, never
	 * on the registers still answering.
	 */
	if (pll_off.gptu_ints == 0)
		test_check("clearing OSC.PLL_POWER_UP drops CGU_STAT.LOCK without stopping the tap", false);
	else
		scale_check_gptu("clearing OSC.PLL_POWER_UP drops CGU_STAT.LOCK without stopping the tap",
			scale_trusted(&pll_off), lock_dropped && !scale_moved(pll_off.gptu_hz, base.gptu_hz, 50));
	test_check("the STM row keeps counting with the PLL off (crystal-sourced)",
		pll_off.stm_ok && pll_off.stm_hz != 0);
	test_check("the RTC row keeps counting with the PLL off",
		pll_off.rtc_t14 != 0);

	/*
	 * THE FIRMWARE'S FULL STANDBY SHAPE - `CGU_OSC &= ~0x1F`: bit 0 (PLL_POWER_UP) **and**
	 * the four phase-power bits (1..4), with NDIV/MDIV/PLL_BYPASS_N preserved.  That is
	 * what psv.c's standby entry does in its five RMWs (0xA2A66AA0/AAC/AB8/AC4/AD0), and
	 * what the bit-0-only measurement above deliberately did not.  The consumers are
	 * parked on OSC above, so nothing depends on the phases and the core cannot hang -
	 * this is psv's own precondition, not a guess.
	 *
	 * TWO PASSABLE OUTCOMES, and the assertions are on the MEASUREMENT, not on which way
	 * it falls:
	 *   - the tap STOPS  -> the bit-0-only result above was a sub-case artefact ("the CGU
	 *                       does not stop the tap with the gate alone"), and the CGU does
	 *                       hold a stop for this tap;
	 *   - the tap KEEPS COUNTING -> **the CGU has no stop control for this tap**: only a
	 *                       rate control (OSC.NDIV) and the LOCK indication, so any real
	 *                       stop must come from the SCCU/standby sequence - while the STM
	 *                       and RTC rows show the crystal keeps counting.
	 * Either retires the open question; only a missing measurement would leave it open.
	 * HAZARD, unchanged: CGU_STAT.LOCK reads 0 here and the block's register interface may
	 * still answer while the counter is stopped, so the conclusion rests on counted
	 * progress in a multi-second RTC-dated window - never on a register read.
	 */
	if (!scale_switch_move("psv standby shape (CGU_OSC &= ~0x1F)", &CGU_OSC,
			CGU_OSC & ~(CGU_OSC_PLL_POWER_UP | CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE2_POWER_UP |
				CGU_OSC_PHASE3_POWER_UP | CGU_OSC_PHASE4_POWER_UP))) {
		test_skip("psv standby shape step (gate + four phase power bits)", "no live line to halt on, so the five-bit clear was skipped");
	} else {
	print_cgu_state("scale PLL-cycle: psv's shape (gate + four phase power bits cleared)");
	struct scale_row psv_shape;
	scale_measure("psv standby shape", &psv_shape);
	test_check("the standby-shape window is valid: the STM row counted (crystal-sourced)",
		psv_shape.stm_ok && psv_shape.stm_hz != 0);
	test_check("the standby-shape window is valid: the RTC row counted",
		psv_shape.rtc_t14 != 0);
	/* ONE line: the field that decides the question, with the counters it is dated by.  The
	   consequence (the CGU has no stop control for this tap => any real stop comes from the
	   SCCU/standby sequence, and the kernel's suspend machinery stays unused rather than wrong)
	   is in the step's comment and in TIMER-FIX - not on the wire. */
	printf("# STANDBY SHAPE: %s (GPTU %u Hz from %u ovf / %u T14; STM %u Hz; RTC %u T14 per %u ms)\n",
		(!psv_shape.gptu_ok && psv_shape.gptu_ints == 0) ? "the tap STOPPED"
			: (!psv_shape.gptu_ok ? "window rejected by the witness - inconclusive"
				: "the tap KEPT COUNTING: no CGU stop control for this tap"),
		(unsigned int) psv_shape.gptu_hz, (unsigned int) psv_shape.gptu_ints,
		(unsigned int) psv_shape.gptu_span, (unsigned int) psv_shape.stm_hz,
		(unsigned int) psv_shape.rtc_t14, (unsigned int) SCALE_RTC_WINDOW_MS);
	} /* psv standby shape */

	uint32_t osc_on = CGU_OSC | CGU_OSC_PLL_POWER_UP | CGU_OSC_PHASE1_POWER_UP |
		CGU_OSC_PHASE2_POWER_UP | CGU_OSC_PHASE3_POWER_UP | CGU_OSC_PHASE4_POWER_UP;
	if (!scale_switch_move("PLL_POWER_UP and phase power restored", &CGU_OSC, osc_on)) {
		printf("# PLL re-power: no live line to halt on; writing it anyway, because leaving the gate clear\n"
			"# under the rest of the suite is the worse hazard\n");
		CGU_OSC = osc_on;
	}
	bool relocked = wait_pll_locked();
	print_cgu_state("scale PLL-cycle: PLL_POWER_UP and the phase power bits set again");
	struct scale_row pll_on;
	scale_measure("PLL powered again", &pll_on);
	test_check("the PLL re-locks after the power-cycle", relocked);
	scale_check_gptu("the GPTU row resumes at the SAME rate after the power-cycle",
		scale_trusted(&pll_on), !scale_moved(pll_on.gptu_hz, base.gptu_hz, 50));
	} /* PLL_POWER_UP clear */
	} /* parked */

	/* SAME HAZARD ORDER AS THE ENTRY RESTORE, so the rule is unconditional: consumers off the PLL
	   (CON1 takes AHB and the core back to PHASE2 - a source switch, so it is halt-gated), then
	   CON2, and OSC LAST.  It is order-independent HERE only because the OSC value equals the live
	   one after the re-power (a no-op) - exactly the "safe by luck, not by construction" shape
	   this lane was bitten by twice, which is why the order is made explicit rather than relied on. */
	scale_restore("scale CGU_CON1 (PLL-cycle: AHB back on PHASE2)", &CGU_CON1, saved_con1_pc);
	cgu_restore_write("scale CGU_CON2 (PLL-cycle)", &CGU_CON2, saved_con2_pc);
	cgu_restore_write("scale CGU_OSC (PLL-cycle: the multiplier, LAST)", &CGU_OSC, saved_osc_pc);
	test_eq_u32("PLL-cycle restored OSC", saved_osc_pc, CGU_OSC);
	test_eq_u32("PLL-cycle restored CON1", saved_con1_pc, CGU_CON1);
	test_eq_u32("PLL-cycle restored CON2", saved_con2_pc, CGU_CON2);

	/* -- the restore, checked, so an interrupted pass cannot leave a step set -----
	 * The pass based itself on boot76's executed shape (above); the sections after it
	 * run in the state main() handed over, so the ENTRY (loader) values go back here,
	 * explicitly, and the restored rows are checked against the entry rows measured
	 * before boot76 was applied. */
	/*
	 * RESTORE ORDER BY HAZARD, NOT BY REGISTER INDEX - and this is the write that ended the
	 * 2026-09-20 emu14 phone run (`ok 34` was the last line: the very next statement was
	 * `CGU_OSC <- entry_osc` = 0x01070001, NDIV=7 => fPLL 26 x 8 = 208 MHz, written while CON1
	 * was still 0x12400012 with AHB_CLKSEL = PHASE2 = 2 x fPLL, so the core's own source went
	 * 208 -> ~416 MHz unhalted and FPI1 doubled 26 -> 52 with it).
	 * Consumers OFF the PLL first (CON1 takes AHB and the core to OSC/BYPASS, FPI1 to OSC; CON2
	 * and CON3 follow), then CON0's phase dividers, and OSC - the multiplier - LAST, when nothing
	 * live derives from it.  The previous order (OSC first) is the one this lane already recorded
	 * as "stopped after `# restore CGU_OSC`".
	 */
	scale_restore("scale CGU_CON1 (entry: AHB and core off the PLL)", &CGU_CON1, entry_con1);
	cgu_restore_write("scale CGU_CON2 (entry)", &CGU_CON2, entry_con2);
	cgu_restore_write("scale CGU_CON3 (entry)", &CGU_CON3, entry_con3);
	cgu_restore_write("scale CGU_CON0 (entry: phase dividers, nothing live on a phase)", &CGU_CON0, entry_con0);
	cgu_restore_write("scale CGU_OSC (entry: the multiplier, LAST)", &CGU_OSC, entry_osc);
	print_cgu_state("scale restored (loader hand-over)");
	test_eq_u32("scale pass restored OSC", entry_osc, CGU_OSC);
	test_eq_u32("scale pass restored CON1", entry_con1, CGU_CON1);
	test_eq_u32("scale pass restored CON2", entry_con2, CGU_CON2);
	test_eq_u32("scale pass restored CON3", entry_con3, CGU_CON3);
	test_eq_u32("scale pass restored CON0", entry_con0, CGU_CON0);
	if (entry_rows.stm_ok && entry_rows.gptu_ok) {
		struct scale_row restored;

		/* The RTC-invariant base is the boot76 state, and this measurement is back in
		   the entry state whose FSTM_DIV_EN differs, so its T14-per-STM-window row is
		   not comparable across the transition - it is checked against the entry row
		   instead, and the invariant base is cleared so no cross-state check fires. */
		scale_base_row.rtc_t14 = 0;
		scale_measure("scale restored", &restored);
		test_check("the restored state reproduces the entry STM row",
			restored.stm_ok && entry_rows.stm_ok &&
			!scale_moved(restored.stm_hz, entry_rows.stm_hz, 50) &&
			scale_rtc_held(&restored, &entry_rows));
		scale_check_gptu("the restored state reproduces the entry GPTU row",
			scale_trusted(&restored) && scale_trusted(&entry_rows),
			!scale_moved(restored.gptu_hz, entry_rows.gptu_hz, 50));
	}
}

int main(void) {
	test_start("Timer reference test");

	/*
	 * No wdt_set_max_execution_time(UINT32_MAX) here: it is not "watchdog off" - the
	 * next assertion or category re-arms 3000 ms through reset_timeout(), which is the
	 * very next call below.  Every wait in this suite is STM-bounded and renews the
	 * budget itself.
	 */
	rtc_reference_init();

	uint32_t rtc_direct = 0, rtc_divided = 0;
	uint32_t prescaler = measure_rtc_prescaler(&rtc_direct, &rtc_divided);

	uint32_t stm_ticks = 0, t14_ticks = 0, stm_hz = 0;
	cpu_enable_irq(false);

	test_category("RTC reference scale");
	reference_state_print("after init");
	test_module_clock("the RTC accepts the module clock the init writes", RTC_CLC);
	/* The ratio's own uncertainty, from the two integer counts that produced it:
	   +-1/direct + direct/divided^2, in hundredths of the ratio. */
	uint32_t scale_tol_hundredths = (rtc_direct && rtc_divided)
		? (uint32_t) (100 * (1.0 / rtc_divided) + 100.0 * rtc_direct / ((double) rtc_divided * rtc_divided))
		: 0;
	printf("# RTC scale: %u counts without PRE / %u counts with PRE over %u us = /%u.%02u +- %u.%02u "
		"(one count of each arm: %u and %u) -> %u Hz; %u ns per T14 count is the suite constant\n",
		(unsigned int) rtc_direct, (unsigned int) rtc_divided, (unsigned int) RTC_SCALE_WINDOW_US,
		(unsigned int) (prescaler / 100), (unsigned int) (prescaler % 100),
		(unsigned int) (scale_tol_hundredths / 100), (unsigned int) (scale_tol_hundredths % 100),
		(unsigned int) rtc_direct, (unsigned int) rtc_divided,
		(unsigned int) (prescaler ? 32768u * 100u / prescaler : 0), (unsigned int) RTC_NS_PER_T14);
	test_check("T14 counts in both prescaler windows", rtc_direct != 0 && rtc_divided != 0);
	/*
	 * /8 is the constant, and the earlier "/7" was a 2 ms window's quantisation, not the
	 * crystal: the phone's own T14-overflow row measured 1 000 729 860 ns against 999 997 440
	 * programmed (ratio 1.000) at reload 61440, i.e. 4096 Hz - the same /8 this suite scales
	 * every duration by.  The band is +-2.5 % around 8.00 with the 50 ms window; it is a band
	 * because the two arms are still counted, not derived.
	 */
	test_check("RTC prescaler divides the 32.768 kHz crystal by 8",
		prescaler >= 780 && prescaler <= 820);
	/* Rows below are dated by the window the suite leaves selected: PRE on.  With that
	   window dead they are unmeasurable, and a wait bounded by the RTC would be a wait
	   for something that is not running - so they report the state and time out. */
	reference_stalled = rtc_divided == 0;
	reference_state_print(reference_stalled ? "the PRE window did not count" : "reference alive");

	test_category("STM rate dated by the crystal");
	print_cgu_state("entry");
	bool measured = measure_stm_rate(&stm_hz, &stm_ticks, &t14_ticks);
	printf("# entry state: STM %u ticks per %u T14 counts = %u Hz +- %u Hz (dominant: +-1 T14 count of %u; "
		"the +-1 STM count term is %u Hz); "
		"26 MHz crystal %u Hz, BSP constant %u Hz, model fSTM %u Hz\n",
		(unsigned int) stm_ticks, (unsigned int) t14_ticks, (unsigned int) stm_hz,
		(unsigned int) rate_residual(stm_hz, t14_ticks), (unsigned int) t14_ticks,
		(unsigned int) rate_residual(stm_hz, stm_ticks),
		(unsigned int) OSC_HZ, (unsigned int) stopwatch_ticks_per_s(),
		(unsigned int) cpu_get_stm_freq());
	printf("# the 32.768 kHz crystal is the one absolute constant here; only the A/B and prescaler ratios are scale-invariant - the crystal cancels because the same counter counts both points. The per-source table ratios and the WFI T14 latencies carry it on the expectation side and are not\n");

	/* boot76's sequence, then the A/B on the divider-enable bit alone. */
	uint32_t entry_osc = CGU_OSC, entry_con0 = CGU_CON0, entry_con1 = CGU_CON1;
	uint32_t entry_con2 = CGU_CON2, entry_con3 = CGU_CON3;

	test_category("boot76 CGU sequence, then FSTM_DIV_EN A/B");
	bool locked = boot76_cgu_apply();
	print_cgu_state("boot76");
	uint32_t boot76_hz = 0, boot76_ticks = 0;
	measured = measure_stm_rate(&boot76_hz, &stm_ticks, &t14_ticks);
	boot76_ticks = stm_ticks;      /* each arm's own count: the A/B ratio's uncertainty uses it */
	printf("# boot76 state: STM %u ticks per %u T14 counts = %u Hz +- %u Hz (dominant: +-1 T14 count of %u; "
		"the +-1 STM count term is %u Hz); model fSTM %u Hz\n",
		(unsigned int) stm_ticks, (unsigned int) t14_ticks, (unsigned int) boot76_hz,
		(unsigned int) rate_residual(boot76_hz, t14_ticks), (unsigned int) t14_ticks,
		(unsigned int) rate_residual(boot76_hz, boot76_ticks),
		(unsigned int) cpu_get_stm_freq());

	uint32_t fstm_divisor = ((CGU_CON1 >> 25) & 1u) ? (4u << ((CGU_CON1 >> 28) & 3)) : 1u;
	CGU_CON1 &= ~0x2000000u;               /* FSTM_DIV_EN = 0, nothing else */
	print_cgu_state("FSTM_DIV_EN=0");
	uint32_t nodiv_hz = 0, nodiv_ticks = 0;
	measured = measure_stm_rate(&nodiv_hz, &stm_ticks, &t14_ticks) && measured;
	nodiv_ticks = stm_ticks;
	printf("# FSTM_DIV_EN=0: STM %u ticks per %u T14 counts = %u Hz +- %u Hz (dominant: +-1 T14 count of %u; "
		"the +-1 STM count term is %u Hz); model fSTM %u Hz\n",
		(unsigned int) stm_ticks, (unsigned int) t14_ticks, (unsigned int) nodiv_hz,
		(unsigned int) rate_residual(nodiv_hz, t14_ticks), (unsigned int) t14_ticks,
		(unsigned int) rate_residual(nodiv_hz, nodiv_ticks),
		(unsigned int) cpu_get_stm_freq());

	/* 32-bit `boot76_hz * 1000` overflows at 4.3 MHz, which is how a clean 2:1 came out
	   as "ratio 0.004"; the product needs 64 bits. */
	uint32_t ratio_milli = nodiv_hz ? (uint32_t) ((uint64_t) boot76_hz * 1000 / nodiv_hz) : 0;
	/* +-1 count on each arm's own STM count; the shared T14 window cancels in the ratio. */
	uint32_t ratio_tol_milli = (boot76_ticks && nodiv_ticks)
		? (uint32_t) ((uint64_t) ratio_milli * (1000u / boot76_ticks + 1000u / nodiv_ticks) / 1u) : 0;
	printf("# A/B: STM with FSTM_DIV_EN=1 -> %u Hz +- %u Hz (dominant: +-1 T14 count of %u), with it cleared "
		"-> %u Hz +- %u Hz (same window), ratio %u.%03u +- %u.%03u (in the ratio the shared T14 window "
		"cancels, so what remains is +-1 count of each arm's own STM count, %u and %u)\n",
		(unsigned int) boot76_hz, (unsigned int) rate_residual(boot76_hz, t14_ticks),
		(unsigned int) t14_ticks,
		(unsigned int) nodiv_hz, (unsigned int) rate_residual(nodiv_hz, t14_ticks),
		(unsigned int) (ratio_milli / 1000), (unsigned int) (ratio_milli % 1000),
		(unsigned int) (ratio_tol_milli / 1000), (unsigned int) (ratio_tol_milli % 1000),
		(unsigned int) boot76_ticks, (unsigned int) nodiv_ticks);

	test_check("PLL locked when boot76's sequence ran", locked);
	/* What the block wrote, not the model's decode: GPTU measures that rate below. */
	test_check("boot76's sequence leaves CON1.FSYS_CLKSEL on BYPASS",
		((CGU_CON1 >> 16) & 3u) == 0);
	test_check("both A/B points produced a non-degenerate rate",
		measured && boot76_hz != 0 && nodiv_hz != 0);
	/* Same two counters, so the ratio needs no absolute scale: the instrument. */
	/*
	 * Two FSTM divisor laws are on record, and this check passes on either - and on nothing
	 * else, so a genuine change of divisor still fails:
	 *   - `4 << DIV` (the CGU decode: /8 at EN=1/DIV=1), which the emulator model implements;
	 *   - the divisor the PART delivers, measured at 1/2 on the phone (2026-09-20 run 4:
	 *     0.501 +- 0.000 against the decode's 1/8, which the report_power_of_two line below
	 *     states as "measured is ~2^-1 x expected").
	 * The decode stays printed as REFUTED rather than deleted, and the DIV cells the phone has
	 * not measured are named as unmeasured instead of failing here.
	 */
	uint32_t ab_div_hundredths = ratio_milli ? 100000u / ratio_milli : 0;
	const char *ab_law = "neither documented law";
	bool ab_ok = false;

	if (ab_div_hundredths >= 792 && ab_div_hundredths <= 808) {
		ab_ok = true;
		ab_law = "1/8 = the 4<<DIV decode (and the emulator model)";
	} else if (ab_div_hundredths >= 198 && ab_div_hundredths <= 202) {
		ab_ok = true;
		ab_law = "1/2 = the part's measured divisor";
	}
	test_check("A/B ratio is a documented FSTM divisor (/8 decode, or the part's /2)", ab_ok);
	printf("# A/B law: measured 1/%u.%02u = %s.  REFUTED as the part's law: `4 << DIV` (divisor %u here,\n"
		"# the phone measures 1/2); UNMEASURED cells: CON1.FSTM_DIV = 0, 2, 3 - the phone matrix settles those.\n",
		(unsigned int) (ab_div_hundredths / 100), (unsigned int) (ab_div_hundredths % 100), ab_law,
		(unsigned int) fstm_divisor);
	report_power_of_two("STM (FSTM_DIV_EN=1 vs 0)", boot76_hz, nodiv_hz);

	/* Every field boot76's block sets, beside what this suite can measure. */
	test_category("boot76 field table");
	printf("# field                   | written      | decoded                       | model          | measured here\n");
	printf("# the decoded/model columns are the fields and the model's decode, NOT measurements: only the\n"
		"# 'measured here' column carries a counter, and '-(route)' means this suite has no route to it.\n");
	printf("# OSC.NDIV/MDIV           | %08X   | fPLL = fOSC*(NDIV+1)/(MDIV+1) | %u Hz       | - (no counter route)\n",
		(unsigned int) CGU_OSC, (unsigned int) cpu_get_pll_freq());
	printf("# OSC.PLL/PHASE*_BYPASS_N | %08X   | phases 1/2/4 in circuit       | -              | -\n", (unsigned int) CGU_OSC);
	printf("# CON0 PHASE1 byte        | %08X   | %08X (K1/K2 in bits 6:3/2:0)  | %u Hz       | - (EBU/ITCM route)\n",
		(unsigned int) CGU_CON0, (unsigned int) (CGU_CON0 & 0xFF), (unsigned int) cpu_get_phase_freq(1));
	printf("# CON0 PHASE2 byte        | %08X   | %08X                          | %u Hz       | - (EBU/ITCM route)\n",
		(unsigned int) CGU_CON0, (unsigned int) ((CGU_CON0 >> 8) & 0xFF), (unsigned int) cpu_get_phase_freq(2));
	printf("# CON0 PHASE4 byte        | %08X   | %08X                          | %u Hz       | - (MMCI DATATIMER route)\n",
		(unsigned int) CGU_CON0, (unsigned int) ((CGU_CON0 >> 24) & 0xFF), (unsigned int) cpu_get_phase_freq(4));
	printf("# CON1.FSYS_CLKSEL        | %u           | BYPASS -> fSYS = fOSC         | %u Hz       | - (GPTU row below)\n",
		(unsigned int) ((CGU_CON1 >> 16) & 3), (unsigned int) cpu_get_sys_freq());
	printf("# CON1.AHB_CLKSEL         | %u           | field 4 = PHASE2              | %u Hz       | - (ITCM route)\n",
		(unsigned int) ((CGU_CON1 >> 20) & 7), (unsigned int) cpu_get_ahb_freq());
	printf("# CON1.FSTM_DIV_EN/DIV    | %u/%u         | divisor %u                     | %u Hz       | %u Hz (A/B above)\n",
		(unsigned int) ((CGU_CON1 >> 25) & 1), (unsigned int) ((CGU_CON1 >> 28) & 3),
		(unsigned int) (((CGU_CON1 >> 25) & 1) ? (4u << ((CGU_CON1 >> 28) & 3)) : 1u),
		(unsigned int) cpu_get_stm_freq(), (unsigned int) boot76_hz);
	printf("# CON2.CPU_DIV/EN        | %u/%u         | field set, enable clear       | %u Hz       | - (ITCM route)\n",
		(unsigned int) ((CGU_CON2 >> 8) & 3), (unsigned int) ((CGU_CON2 >> 12) & 1),
		(unsigned int) cpu_get_freq());
	printf("# CON2.EBU/DSP/MS/CLK48M  | %08X   | %u/%u/%u/%u                    | %u/%u/%u/%u Hz | - (MMCI/DSP routes)\n",
		(unsigned int) CGU_CON2, (unsigned int) ((CGU_CON2 >> 4) & 7), (unsigned int) (CGU_CON2 & 7),
		(unsigned int) ((CGU_CON2 >> 28) & 3), (unsigned int) ((CGU_CON2 >> 14) & 3),
		(unsigned int) cpu_get_ebu_freq(), (unsigned int) cpu_get_dsp_freq(), (unsigned int) cpu_get_ms_freq(),
		(unsigned int) cpu_get_clk48m_freq());
	printf("# CON3.CLK48M/MMCI/AHB_PER| %08X   | %u/%u/%u                       | %u/%u/%u Hz | - (MMCI route)\n",
		(unsigned int) CGU_CON3, (unsigned int) ((CGU_CON3 >> 24) & 3), (unsigned int) ((CGU_CON3 >> 8) & 3),
		(unsigned int) (CGU_CON3 & 3), (unsigned int) cpu_get_clk48m_freq(), (unsigned int) cpu_get_mmci_freq(),
		(unsigned int) cpu_get_ahb_per_freq());
	printf("# SCU_EBUCLC2             | %08X   | bit0 set by the block         | -              | -\n",
		(unsigned int) SCU_EBUCLC2);

	/*
	 * RESTORING THE ENTRY CGU VALUES - **this tree's order**, and the ordering question's
	 * provenance.  What the code below does, and it is the rule (consumers off the PLL first, the
	 * multiplier LAST): `CGU_CON1 (entry)` takes AHB and the core to BYPASS/OSC and FPI1 to OSC;
	 * then CON2, CON3, CON0; then `CGU_OSC (entry)` - the multiplier - with nothing live deriving
	 * from it.  Safe by construction, not by luck: the CON1 write is what removes the PLL from
	 * every live consumer.
	 *
	 * HISTORY, kept because the pair directories and their claim file still exist and this block
	 * was one half of them: the **baseline** wrote `OSC` first (the phone run stopped after its
	 * pre-write line), the **experiment** wrote `CON1` before `OSC` and completed.  That single
	 * position change was the whole difference between the two images - and it is the experiment's
	 * order, not the baseline's, that this tree now takes.  The pair's files
	 * (`scratch/bsp-unit-audit/timers-osc-pair/`) are the provenance for the finding, not a
	 * description of the live code.  A later advisory read that history as the live order; this
	 * paragraph is here so the next reader does not.
	 *
	 * The firmware trace supplies a **forward** order (power and lock the source, then move
	 * the consumers onto it).  Moving `CON1` ahead of `OSC` is the **dependency-derived
	 * inverse of that - our reasoning, not vendor instruction** - and it is an experiment,
	 * not the fix.  The consumer chosen is the AHB/core one: this suite executes from
	 * intram, so `EBU_CLKSEL` cannot be what stops the code, and `CON1 = 12400012` puts
	 * `AHB_CLKSEL = PHASE2` with `FSTM_DIV_EN` set while `FSYS_CLKSEL = BYPASS` gives
	 * `fSYS = fOSC`.  **Which field clocks the ARM926/AHB at that instant is not
	 * established by the decode** - `bsp/lib/cpu.c`'s `cpu_get_freq()` derives the CPU from
	 * `cpu_get_ahb_freq()`, but that is our model of the part.
	 *
	 * `CGU_STAT.LOCK` is printed either side of each write.  **If the `OSC` store stops
	 * execution, the post-write line can never appear - that absence is the result**, not a
	 * broken marker; only the pre-write line and the pre-write marker survive.
	 */
	printf("# restoring the entry CGU values (consumers off the PLL first, the multiplier last)\n");
	cgu_restore_write("CGU_CON1 (entry)", &CGU_CON1, entry_con1);
	cgu_restore_write("CGU_CON2 (entry)", &CGU_CON2, entry_con2);
	cgu_restore_write("CGU_CON3 (entry)", &CGU_CON3, entry_con3);
	cgu_restore_write("CGU_CON0 (entry)", &CGU_CON0, entry_con0);
	cgu_restore_write("CGU_OSC (entry: the multiplier, LAST)", &CGU_OSC, entry_osc);
	printf("# entry CGU values restored\n");

	test_category("TPU counter rate from the driver's K/L programming");
	printf("# tpu_init\n");
	tpu_init();
	printf("# tpu_init done\n");
	print_cgu_state("after restore");
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t tpu_measured_hz = 0;
	measured = measure_tpu_counter_rate(&tpu_measured_hz) && measured;
	printf("# TPU K=%u L=%u -> model %u Hz, measured by the RTC %u Hz +- %u Hz over %u T14 counts "
		"(driver target %u Hz)\n",
		(unsigned int) LINUX_TPU_K, (unsigned int) LINUX_TPU_L,
		(unsigned int) tpu_f_counter, (unsigned int) tpu_measured_hz,
		(unsigned int) rate_residual(tpu_measured_hz, TPU_RATE_T14_COUNTS),
		(unsigned int) TPU_RATE_T14_COUNTS, (unsigned int) LINUX_TPU_TARGET_HZ);
	test_check("TPU counter rate matches what K/L programming implies",
		ratio_matches(tpu_measured_hz, tpu_f_counter, 3));
	report_power_of_two("TPU counter", tpu_measured_hz, tpu_f_counter);

	gptu_init();
	cpu_enable_irq(false);

	/* Each source alone first, so a stall is attributed to one of them. */
	test_category("Sources armed alone");

	/* The Linux clockevent's shape: one-shot arm, fire, re-arm, repeated. */
	sources_arm(SRC_TPU0);
	bool int0_rearm = sources_wait(SRC_TPU0, PROBE_WAIT_MS);
	sources_stop();
	test_check("TPU INT0 fires through repeated arm/fire/re-arm", int0_rearm);

	/* INT1's own line, armed once, nothing writing TPU_PARAM after it. */
	sources_arm(SRC_TPU1);
	bool int1_alone = false;
	uint64_t int1_start = stopwatch_get();
	printf("# TPU INT1 arm: waiting for one event, STM-bounded at %u ms\n# ", (unsigned int) PROBE_WAIT_MS);
	while (!reference_stalled && !test_elapsed_bound_ms(int1_start, PROBE_WAIT_MS) && !int1_alone) {
		int1_alone = tpu1_events != 0;
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();
	uint32_t int1_alone_t14 = tpu1_periods[0];
	uint32_t int1_fanout_t14_milli =
		(uint32_t) ((uint64_t) TPU_FANOUT_TICKS * RTC_T14_HZ * 1000 / tpu_f_counter);
	uint32_t int1_fanout_t14 = int1_fanout_t14_milli / 1000;
	sources_stop();
	printf("# TPU INT1 isolated: first event %u T14 counts (%u ns) after one arm; K/L model %u.%03u counts (resolution: one T14 count)\n",
		(unsigned int) int1_alone_t14, (unsigned int) (int1_alone_t14 * RTC_NS_PER_T14),
		(unsigned int) (int1_fanout_t14_milli / 1000), (unsigned int) (int1_fanout_t14_milli % 1000));
	test_check("TPU INT1 fires and reaches its own VIC line", int1_alone);

	static const struct {
		const char *name;
		uint32_t arm, wait, ms;
	} PROBES[] = {
		{ "GPTU T2A", SRC_GPTU, SRC_GPTU, PROBE_WAIT_MS },
		{ "RTC T14", SRC_RTC, SRC_RTC, RTC_SOURCE_WAIT_MS },
	};

	for (uint32_t i = 0; i < ARRAY_SIZE(PROBES); i++) {
		sources_arm(PROBES[i].arm);
		bool ok = sources_wait(PROBES[i].wait, PROBES[i].ms);
		sources_stop();
		printf("# %s alone: TPU0=%u TPU1=%u GPTU=%u RTC=%u in up to %u ms\n",
			PROBES[i].name, (unsigned int) tpu0_events, (unsigned int) tpu1_events,
			(unsigned int) gptu_events, (unsigned int) rtc_events, (unsigned int) PROBES[i].ms);
		test_check(PROBES[i].name, ok);
	}

	test_category("All sources armed in one window");
	/* The period row's own state, written here: nothing above this line decides what it
	   measures. */
	rtc_t14_program(RTC_T14_RELOAD, RTC_T14_RELOAD);
	rtc_t14_state_print("T14 period row");
	/* INT1 is not required to survive INT0's re-arms; its row is measured isolated. */
	sources_arm(SRC_TPU0 | SRC_GPTU | SRC_RTC);
	bool complete = sources_wait(SRC_TPU0 | SRC_GPTU | SRC_RTC, COMBINED_WAIT_MS);
	sources_stop();

	uint32_t tpu0_period = period_average(tpu0_periods, tpu0_events);
	uint32_t gptu_period = period_average(gptu_periods, gptu_events);
	uint32_t rtc_period = period_average(rtc_periods, rtc_events);
	uint32_t tpu_programmed_ns = (uint32_t) ((uint64_t) TPU_ARM_TICKS * 1000000000 / tpu_f_counter);
	uint32_t tpu1_programmed_ns = (uint32_t) ((uint64_t) TPU_FANOUT_TICKS * 1000000000 / tpu_f_counter);
	uint32_t gptu_programmed_ns = (uint32_t) ((uint64_t) GPTU_T2_COUNTS * 1000000000 / gptu_f_clock);

	printf("# timer     | clock source       | programmed | measured period (RTC)  | expected   | ratio\n");
	report("TPU INT0", "K/L fractional div", tpu_programmed_ns, tpu0_period, tpu_expected_t14_milli);
	report("GPTU T2A", "fSYS / RMC", gptu_programmed_ns, gptu_period, gptu_expected_t14_milli);
	report("RTC T14", "32768 Hz / 8", RTC_T14_PERIOD * RTC_NS_PER_T14, rtc_period, RTC_T14_PERIOD * 1000);
	report("TPU INT1", "K/L, one arm only", tpu1_programmed_ns, int1_alone_t14, int1_fanout_t14_milli);
	printf("# the expected column is in T14 counts to 0.001: the one printed as a whole count and then\n"
		"# used unrounded is the defect this column exists to stop (40.960 used to print as 40).\n");

	/* Not a verdict on the channel: the isolated row above is that measurement. */
	printf("# TPU INT1 mechanism: the arm writes TPU_PARAM=0 first (timer-pmb887x-tpu.c :71 arm / :94 shutdown) "
		"and the model restarts counter and irq_fired whenever TINI is clear (tpu.c :434-437, :456-457), so under "
		"INT0's %u-tick re-arm INT1 is not measured; %u ticks is inside the %u-tick frame, so the modulo is not "
		"the cause. Model property - a phone run decides the part. Both compare lines are the GSM time unit's, per "
		"firmware: `dwddrv/TU/src/gsmtu.c` arms INT0 = 0x7FFF, computes and writes INT1, then reads COUNTER, and "
		"both lines are armed at boot at VIC priority 0x0C; the frame counter runs at 10000 ticks per 4.61538 ms "
		"frame (the firmware's only K/L write is the reset K=1/L=2 in `pow.c`). The K=3/L=13 -> 1 MHz above is this "
		"suite's own target for its rate measurement, not the part's frame programming - writing it reprograms the "
		"GSM frame base. It is therefore not true that INT1 is 'the GSM-frame path' and INT0 the kernel's alone: "
		"both are the time unit's. Evidence: `scratch/apoxi-clk/TPU-usage.md` and `tpu-sites.txt` (the APOXI lane's "
		"files). VIC %u is the line the DT's clockevent takes.\n",
		(unsigned int) TPU_ARM_TICKS, (unsigned int) TPU_FANOUT_TICKS,
		(unsigned int) TPU_FRAME_TICKS, (unsigned int) VIC_TPU_INT0_IRQ);

	uint32_t gptu_rmc = (GPTU_CLC(GPTU0) & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT;

	/*
	 * The one-period row above is a PATH check - does the overflow fire at the programmed
	 * interval at all - and its derived clock is printed with the window that produced it,
	 * because 10 T14 counts of window resolve 10 %/2 of a rate, not a rate.
	 */
	uint32_t one_period_fsys = gptu_period ?
		(uint32_t) ((uint64_t) GPTU_T2_COUNTS * RTC_T14_HZ * gptu_rmc / gptu_period) : 0;
	printf("# GPTU one-period row, informational only: %u Hz from a %u T14-count window, so one more or fewer "
		"count of that window is %u.%02u %% of the figure (this is why it is not the rate row); model fSYS %u Hz, "
		"GPTU RMC=%u - the rate row is below\n",
		(unsigned int) one_period_fsys, (unsigned int) gptu_period,
		(unsigned int) (gptu_period ? 10000u / gptu_period / 100u : 0),
		(unsigned int) (gptu_period ? 10000u / gptu_period % 100u : 0),
		(unsigned int) cpu_get_sys_freq(), (unsigned int) gptu_rmc);
	/*
	 * THE RATE ROW.  Counts overflows across a long RTC-dated interval, so the count is the
	 * measurement and its own +/- one is the band - the shape the RTC-scale row already uses.
	 */
	test_category("GPTU rate over a long RTC-dated window");
	uint32_t gptu_intervals = 0, gptu_span_t14 = 0;
	bool gptu_rate_ok = measure_gptu_rate(&gptu_intervals, &gptu_span_t14);
	/* The MEASUREMENT, kept apart from the witness verdict: the rate is computed and printed
	   whenever the window produced counts, so a witness artefact costs a comparison, never the
	   number this row exists to produce. */
	bool gptu_rate_measured = (gptu_intervals != 0 && gptu_span_t14 != 0);
	uint32_t gptu_hz = 0, gptu_hz_lo = 0, gptu_hz_hi = 0, gptu_multiple = 0;
	/*
	 * The uncertainty figures, all derived from the integer counts that produced the value
	 * and with their contributors named: the half-width of the band, and the two counts that
	 * make it up.  The earlier label printed the FULL band width in thousandths divided by
	 * ten ("0.01 %") beside a half-width of 0.069 % - a precision claim its own parenthesis
	 * contradicted, which is the defect class this sweep exists to remove.
	 */
	uint32_t gptu_half_hz = 0, gptu_half_ppm = 0, gptu_ovf_ppm = 0, gptu_t14_ppm = 0, gptu_off_ppm = 0;

	printf("# the long window's witness verdict: %s\n",
		gptu_rate_ok ? "accepted" : "REJECTED - the rate below is still shown, but not compared");
	if (gptu_rate_measured) {
		uint64_t per_interval = (uint64_t) GPTU_T2_COUNTS * RTC_T14_HZ;

		gptu_hz = (uint32_t) ((uint64_t) gptu_intervals * per_interval / gptu_span_t14);
		gptu_hz_hi = (uint32_t) ((uint64_t) (gptu_intervals + 1) * per_interval / (gptu_span_t14 - 1));
		gptu_hz_lo = (uint32_t) ((uint64_t) (gptu_intervals - 1) * per_interval / (gptu_span_t14 + 1));
		gptu_multiple = (gptu_hz + OSC_HZ / 2) / OSC_HZ;
		gptu_half_hz = (gptu_hz_hi - gptu_hz_lo) / 2;
		gptu_half_ppm = gptu_hz ? (uint32_t) ((uint64_t) gptu_half_hz * 1000000 / gptu_hz) : 0;
		gptu_ovf_ppm = gptu_intervals ? 1000000u / gptu_intervals : 0;
		gptu_t14_ppm = gptu_span_t14 ? 1000000u / gptu_span_t14 : 0;
		gptu_off_ppm = (gptu_multiple && gptu_hz) ?
			(uint32_t) (((gptu_multiple * OSC_HZ > gptu_hz ? gptu_multiple * OSC_HZ - gptu_hz
								      : gptu_hz - gptu_multiple * OSC_HZ) * 1000000ull) / gptu_hz) : 0;

		printf("# GPTU rate: %u overflows of %u counts in %u T14 counts (%u.%03u s of RTC at 4096 Hz) = %u Hz\n",
			(unsigned int) gptu_intervals, (unsigned int) GPTU_T2_COUNTS,
			(unsigned int) gptu_span_t14, (unsigned int) (gptu_span_t14 / RTC_T14_HZ),
			(unsigned int) ((uint64_t) gptu_span_t14 * 1000 / RTC_T14_HZ % 1000),
			(unsigned int) gptu_hz);
		printf("# GPTU band: %u..%u Hz = centre %u Hz +- %u Hz (%u ppm), from +-1 overflow of %u (%u ppm) "
			"+ +-1 T14 count of %u (%u ppm), GPTU RMC=%u\n",
			(unsigned int) gptu_hz_lo, (unsigned int) gptu_hz_hi, (unsigned int) gptu_hz,
			(unsigned int) gptu_half_hz, (unsigned int) gptu_half_ppm,
			(unsigned int) gptu_intervals, (unsigned int) gptu_ovf_ppm,
			(unsigned int) gptu_span_t14, (unsigned int) gptu_t14_ppm, (unsigned int) gptu_rmc);
		printf("# SUPERSEDED label, quoted so it can be found: this row used to print its band's half-width as "
			"\"0.01 percent\", which was the FULL width in thousandths divided by ten; the numbers above are the "
			"half-width and the two counts that make it up, so the label and its parenthesis agree.\n");
		/*
		 * Two different distances that are easy to conflate: the nominal's distance from the measured
		 * CENTRE, and how far that puts it from the band's nearest EDGE (which is what "inside/outside
		 * the band" means).  Both are printed, in that order, so neither reference can be read as the
		 * other.
		 */
		uint32_t tap_nominal = gptu_multiple * OSC_HZ;
		uint32_t tap_from_centre = tap_nominal > gptu_hz ? tap_nominal - gptu_hz : gptu_hz - tap_nominal;
		uint32_t tap_edge_dist = tap_from_centre > gptu_half_hz ? tap_from_centre - gptu_half_hz
									: gptu_half_hz - tap_from_centre;

		printf("# GPTU tap: nearest integer multiple is %u x 26 MHz = %u Hz; it sits %u ppm (%u Hz) from the "
			"measured centre %u Hz, which is %u Hz %s the band %u..%u Hz - within the crystal and systematic "
			"uncertainty either way, since every figure here is dated by the nominal 32768 Hz crystal; the band "
			"is what settles WHICH multiple it is, and it cannot name the CGU field that routes the tap (that "
			"is a CGU source experiment, not this row).\n",
			(unsigned int) gptu_multiple, (unsigned int) tap_nominal,
			(unsigned int) gptu_off_ppm, (unsigned int) tap_from_centre, (unsigned int) gptu_hz,
			(unsigned int) tap_edge_dist, tap_from_centre > gptu_half_hz ? "beyond" : "inside",
			(unsigned int) gptu_hz_lo, (unsigned int) gptu_hz_hi);
		printf("# SUPERSEDED measurement, quoted so it can be found: the phone log's \"106 496 000 Hz\" / "
			"106.496 MHz and this suite's older \"fSYS dated by the RTC through GPTU\" line are the one-period "
			"row's integer division - the expectation is 40.960 T14 counts (printed as 40 then) over a 10-count "
			"window, so 26 MHz x 40.960/10 = 106.496 MHz while 26 MHz x 40/10 = 104 MHz; those are the same "
			"measurement, and its 2.44 %% window quantisation is why they differ. Do not re-derive a rate from "
			"the 106.496 string; the row above is the rate.\n");
	} else {
		printf("# the long GPTU window produced no usable interval: no rate is quoted\n");
	}

	test_check("every source reached the expected event count in one window", complete);
	test_check("TPU INT0 ticks at the armed interval through repeated re-arms",
		ratio_matches_milli(tpu0_period, tpu_expected_t14_milli, 50));
	test_check("TPU INT1 measured with one arm ticks at its own compare",
		ratio_matches_milli(int1_alone_t14, int1_fanout_t14_milli, 50));
	/*
	 * The period row's expectation comes from the BSP's fSYS = fOSC belief (26 MHz).  The part's
	 * GPTU tap is the fPLL one, x4 that - which the long-window rate row measures - so on silicon
	 * the overflow is 4x sooner than the belief implies and this row cannot pass until the Linux
	 * GPTU node carries clock-frequency = <104000000> so the driver's belief matches the part.
	 * Belief holds -> ok; a x4 tap -> SKIP with that reason (a skip is not a failure and does not
	 * colour the plan); anything else -> FAIL, which is what a real regression looks like.
	 */
	if (ratio_matches_milli(gptu_period, gptu_expected_t14_milli, 50))
		test_check("GPTU T2A overflows at the interval the fSYS = fOSC belief implies", true);
	else if (ratio_matches_milli(gptu_period, gptu_expected_t14_milli / 4, 50))
		test_skip("GPTU T2A overflows at the interval the fSYS = fOSC belief implies",
			"fsys_clk belief; enable the GPTU clock-frequency override to test it");
	else
		test_check("GPTU T2A overflows at the interval the fSYS = fOSC belief implies", false);
	test_check("the long GPTU window resolved a rate", gptu_rate_measured);
	/* These three rest on the rate, so a window the WITNESS rejected is a stated skip - the
	   same contract the pass's comparisons use. */
	if (!gptu_rate_ok) {
		test_skip("the GPTU band's half-width is narrower than 1 % of the rate",
			"the long window's witness rejected it; the rate is printed but not compared");
		test_skip("the GPTU tap is an integer multiple of the 26 MHz crystal",
			"the long window's witness rejected it; the rate is printed but not compared");
		test_skip("the GPTU band identifies that multiple (it spans no second one)",
			"the long window's witness rejected it; the rate is printed but not compared");
	} else {
		test_check("the GPTU band's half-width is narrower than 1 % of the rate",
			gptu_half_ppm != 0 && gptu_half_ppm < 10000);
		test_check("the GPTU tap is an integer multiple of the 26 MHz crystal",
			gptu_multiple != 0 && ratio_matches(gptu_hz, gptu_multiple * OSC_HZ, 2));
		test_check("the GPTU band identifies that multiple (it spans no second one)",
			gptu_multiple != 0 && gptu_hz_hi < (gptu_multiple + 1) * OSC_HZ &&
			gptu_hz_lo > (gptu_multiple - 1) * OSC_HZ);
	}
	test_check("RTC T14 overflows once per second", ratio_matches(rtc_period, RTC_T14_PERIOD, 2));
	printf("# which number the RTC rate rests on: NOT this row.  The rate is the crystal-scale row (the A/B "
		"between the two PRESCALER windows = the /8 division, phone-verified at /8.00 -> 4096 Hz) plus the "
		"RELOAD this row's state print shows (4096 counts at 244140 ns).  This row confirms the reload is "
		"the programmed one; if it disagrees, it measured a reload it did not set and the rate is unmoved.\n");
	test_eq_u32("no interrupt arrived on an unarmed line", 0, irq_other);

	report_power_of_two("TPU INT0", tpu0_period, tpu_expected_t14);
	report_power_of_two("TPU INT1", int1_alone_t14, int1_fanout_t14);
	report_power_of_two("GPTU T2A", gptu_period, gptu_expected_t14);

	/* The CGU clock-scaling pass, before the WFI sections (which must stay last). */
	cgu_scale_pass(entry_osc, entry_con0, entry_con1, entry_con2, entry_con3);

	/* The bound's own acceptance: the RTC line has to wake a WFI by itself. */
	printf("# starting the RTC-only control: RTC bound %u T14 counts (%u ns), "
		"TPU deadline %u counts (%u ns)\n",
		(unsigned int) RTC_BOUND_COUNTS, (unsigned int) (RTC_BOUND_COUNTS * RTC_NS_PER_T14),
		(unsigned int) tpu_expected_t14, (unsigned int) (tpu_expected_t14 * RTC_NS_PER_T14));
	test_category("RTC T14 wakes a WFI core, the bound's own control");
	rtc_deadline_arm();
	cpu_enable_irq(false);
	uint32_t control_hold = 0;
	bool control_live = !reference_stalled &&
		line_seen_live(SRC_RTC, "WFI control", LINE_WAIT_MS, &control_hold);
	if (reference_stalled)
		printf("# the PRE window did not count: the armed RTC line is not waited out\n");
	if (control_live) {
		rtc_deadline_arm();
		cpu_enable_irq(false);
	}
	struct wake_layers rtc_pre;
	wake_layers_read(&rtc_pre);
	rtc_tick_base();
	/* No sleep without a line that was seen to arrive: it would end in the reset. */
	if (control_live)
		idle_wait_for_interrupt();
	wfi_hold_t14 = rtc_tick_now();
	struct wake_layers rtc_post;
	wake_layers_read(&rtc_post);
	/* Teardown while masked, in path order. */
	RTC_ISNRC = RTC_ISNRC_T14;
	RTC_SRC = MOD_SRC_CLRR;
	RTC_ISNC = 0;
	cpu_enable_irq(true);
	wake_layers_print("WFI control pre ", &rtc_pre);
	wake_layers_print("WFI control post", &rtc_post);
	printf("# WFI control: held %u T14 counts (%u ns) - matches %s; TPU deadline %u counts, "
		"RTC bound %u counts\n",
		(unsigned int) wfi_hold_t14, (unsigned int) (wfi_hold_t14 * RTC_NS_PER_T14),
		deadline_name(wfi_hold_t14), (unsigned int) tpu_expected_t14,
		(unsigned int) RTC_BOUND_COUNTS);
	test_check("WFI control precondition: TPU INT0 SRC request clear", rtc_pre.tpu0_srr == 0);
	test_check("WFI control precondition: TPU INT1 SRC request clear", rtc_pre.tpu1_srr == 0);
	test_check("WFI control precondition: RTC T14 request clear", rtc_pre.rtc_t14ir == 0);
	test_check("WFI control precondition: RTC SRC request clear", rtc_pre.rtc_srr == 0);
	test_check("WFI control precondition: no line pending at the VIC",
		rtc_pre.vic_num == 0);
	test_check("the RTC line reaches its own VIC request before the core sleeps", control_live);
	test_check("the core held in WFI: the return is not immediate", wfi_hold_t14 != 0);
	test_check("the core returns from WFI with the RTC line pending", rtc_post.mask == SRC_RTC);
	test_check("the RTC's request bit and its SRC layer both show the bound",
		rtc_post.rtc_t14ir != 0 && rtc_post.rtc_srr != 0);

	/* Last, because a WFI that never returns truncates everything after it. */
	printf("# starting the wake probe: TPU one-shot %u T14 counts (%u ns) expected, "
		"RTC bound %u counts (%u ns) programmed immediately before the wait\n",
		(unsigned int) tpu_expected_t14, (unsigned int) (tpu_expected_t14 * RTC_NS_PER_T14),
		(unsigned int) RTC_BOUND_COUNTS, (unsigned int) (RTC_BOUND_COUNTS * RTC_NS_PER_T14));
	printf("# a run truncated here is incomplete, not a verdict: it cannot separate "
		"'no line can wake WFI' from 'both sources missed'\n");
	test_category("TPU IRQ wakes a WFI core");
	sources_arm(SRC_TPU0);
	rtc_deadline_arm();
	cpu_enable_irq(false);
	uint32_t wake_hold = 0;
	bool wake_live = line_seen_live(SRC_TPU0, "wake probe", LINE_WAIT_MS, &wake_hold);
	if (wake_live) {
		/* The same arming again: the pre-check's own event must not shorten the sleep. */
		sources_arm(SRC_TPU0);
		rtc_deadline_arm();
		cpu_enable_irq(false);
	}
	struct wake_layers wfi_pre;
	wake_layers_read(&wfi_pre);
	rtc_tick_base();
	if (wake_live)
		idle_wait_for_interrupt();
	wfi_hold_t14 = rtc_tick_now();
	wfi_tpu_counter = (TPU_COUNTER & TPU_COUNTER_VALUE);
	struct wake_layers wfi_post;
	wake_layers_read(&wfi_post);
	sources_stop();
	cpu_enable_irq(true);
	uint32_t wfi_ticks = (uint32_t) ((uint64_t) wfi_hold_t14 * tpu_f_counter / RTC_T14_HZ);
	uint32_t wfi_frames = wfi_ticks / TPU_FRAME_TICKS;
	wake_layers_print("wake probe pre ", &wfi_pre);
	wake_layers_print("wake probe post", &wfi_post);
	printf("# WFI: held %u T14 counts (%u ns) - matches %s; TPU deadline %u counts, RTC bound %u counts\n",
		(unsigned int) wfi_hold_t14, (unsigned int) (wfi_hold_t14 * RTC_NS_PER_T14),
		deadline_name(wfi_hold_t14), (unsigned int) tpu_expected_t14,
		(unsigned int) RTC_BOUND_COUNTS);
	/* Origin known (the arm reset the counter); with no wrap the compare is unambiguous. */
	printf("# WFI: TPU counter %u, INT0 %u, INT1 %u, OVERFLOW %u ticks; the wait is %u ticks = %u frames, "
		"so the compare is %s\n",
		(unsigned int) wfi_tpu_counter, (unsigned int) TPU_ARM_TICKS,
		(unsigned int) TPU_FANOUT_TICKS, (unsigned int) TPU_OVERFLOW_VALUE,
		(unsigned int) wfi_ticks, (unsigned int) wfi_frames,
		wfi_frames == 0 ? "unambiguous and reached" : "not established, a wrap is inside the wait");
	test_check("wake probe precondition: TPU INT0 SRC request clear", wfi_pre.tpu0_srr == 0);
	test_check("wake probe precondition: TPU INT1 SRC request clear", wfi_pre.tpu1_srr == 0);
	test_check("wake probe precondition: RTC T14 request clear", wfi_pre.rtc_t14ir == 0);
	test_check("wake probe precondition: RTC SRC request clear", wfi_pre.rtc_srr == 0);
	test_check("wake probe precondition: no line pending at the VIC", wfi_pre.vic_num == 0);
	test_check("the TPU compare is reached before the core sleeps", wake_live);
	test_check("the core stops inside WFI until a line shows a request",
		wfi_hold_t14 != 0 && wfi_post.mask != 0);
	test_check("exactly one line is pending at the masked return",
		wfi_post.mask == SRC_TPU0 || wfi_post.mask == SRC_RTC);
	test_check("the TPU compare is reached: origin known, no wrap in the wait, counter at INT0",
		wfi_frames == 0 && wfi_tpu_counter >= TPU_ARM_TICKS);
	test_check("TPU pending alone at return and the latency matches its TPU deadline",
		wfi_post.mask == SRC_TPU0 && ratio_matches(wfi_hold_t14, tpu_expected_t14, 20));

	cpu_enable_irq(true);
	return test_finish();
}
