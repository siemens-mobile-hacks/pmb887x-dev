/*
 * Which bring-up step makes the RTC register interface answer.
 */

#include <pmb887x.h>

#include "test.h"

/* The same reload the timers suite uses: 4096 counts/second with PRE, 32768 without. */
#define RTC_T14_RELOAD 61440u
#define T14_WINDOW_MS 20u                /* counts per window; 82 with PRE, 655 without */
#define SETTLE_MS 50u                    /* bound on one "has the write taken effect" wait */
#define RTCIF_ENABLE 0xAAu
#define RTC_CTRL_USE \
	(RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN | RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCINT | RTC_CTRL_CLR_RTCBAD)

/*
 * The two granularity probes.  The alarm compares the packed calendar counter, so its
 * resolution is one second; T14 is the block's sub-second source.  Both are measured here
 * rather than inferred: measured values are the two numbers the phone run is for.
 */
#define ALARM_BOUND_MS 2500u                                      /* > one second + slack */
#define T14_SUB_RELOAD 0xFE00u                                    /* (0x10000-0xFE00)=512 ticks */
#define T14_SUB_PERIOD_US 125000u                                 /* 512 / 4096 Hz = 125 ms */
#define T14_EVENTS 4u
#define T14_BOUND_MS 2000u

/*
 * Every RTC register access in this suite goes through one of these two, so a fault raised by
 * an access is reported (LR/SPSR/FSR/FAR, from the harness) and the step continues instead of
 * ending the run there.  The prior phone run entered the fault reporter during the RTC probe;
 * **the faulting access is unknown** - PC/FAR were never printed - which is exactly what the
 * recovery and the per-field report exist to establish.
 *
 * THE RULE THIS IMPLIES: a recovered access did not complete, so its value is UNKNOWN.  Both
 * helpers log `# RECOVERED:` and return false; a caller must not treat the value it left in
 * *out (0) as a datum about the part, and every check that rests on a count must also require
 * that no access inside its window was recovered.  `rtc_fault_count` is the same-run count of
 * those events, so that requirement is a comparison, not an impression: a row that reads
 * "0 after a recovered fault" is evidence that the access did not complete, not evidence that
 * the register reads 0.
 */
static uint32_t rtc_fault_count;

static bool rtc_write(volatile uint32_t *reg, uint32_t value) {
	if (test_fault_guard()) {
		test_fault_guard_end();
		rtc_fault_count++;
		printf("# RECOVERED: RTC write to %08X faulted; nothing was written and the step is not a datum (fields above)\n",
			(unsigned int) (uintptr_t) reg);
		return false;
	}

	*reg = value;
	test_fault_guard_end();
	return true;
}

static bool rtc_read(volatile uint32_t *reg, uint32_t *out) {
	if (test_fault_guard()) {
		test_fault_guard_end();
		rtc_fault_count++;
		*out = 0;
		printf("# RECOVERED: RTC read of %08X faulted; the 0 left in its cell is not a datum (fields above)\n",
			(unsigned int) (uintptr_t) reg);
		return false;
	}

	*out = *reg;
	test_fault_guard_end();
	return true;
}

/* True when no access since `faults_at_start` was recovered, i.e. the window's counts are a datum. */
static bool rtc_no_recovered_access(uint32_t faults_at_start) {
	return rtc_fault_count == faults_at_start;
}

/*
 * The next calendar second after `cnt`, in the block's packed layout (sec/min/hour/day with
 * the biases of the firmware's own converter): both the model and the part step this counter
 * on a T14 overflow, so "next second" is a field-aware increment, not `cnt + 1`.
 */
static uint32_t rtc_next_second(uint32_t cnt) {
	uint32_t sec = cnt & 0x3FFu, min = (cnt >> 10) & 0x3Fu, hour = (cnt >> 16) & 0x3Fu;
	uint32_t day = cnt >> 22;

	if (sec == 0x3FFu) {
		sec = 964u;                                  /* SEC_REL */
		if (++min == 64u) {
			min = 4u;                            /* MIN_REL */
			if (++hour == 64u) {
				hour = 40u;                  /* HOUR_REL */
				day = (day + 1u) & 0x3FFu;
			}
		}
	} else {
		sec++;
	}

	return sec | (min << 10) | (hour << 16) | (day << 22);
}

/*
 * Arm RTC_ALARM at the counter's current value (`next_unit` false) or at the next packed
 * calendar unit (`next_unit` true), wait for ALARMIR, clear it and return the latency in STM
 * ms.  `*seen` is set false when a bound expires - the caller must not read the number as a
 * measurement then.  Both arming rules are measured because the comparator's semantics are
 * not established (level match vs transition out of the match).
 */
static uint32_t alarm_latency_ms(bool next_unit, bool *seen) {
	uint32_t cnt = 0, isnc = 0;

	rtc_read(&RTC_CNT, &cnt);
	rtc_write(&RTC_ALARM, next_unit ? rtc_next_second(cnt) : cnt);
	rtc_write(&RTC_ISNRC, RTC_ISNRC_ALARM);
	rtc_write(&RTC_SRC, MOD_SRC_CLRR);
	rtc_write(&RTC_ISNC, RTC_ISNC_ALARMIE);

	uint64_t start = stopwatch_get();
	*seen = true;
	while (!test_elapsed_bound_ms(start, ALARM_BOUND_MS)) {
		rtc_read(&RTC_ISNC, &isnc);
		if ((isnc & RTC_ISNC_ALARMIR) != 0) {
			rtc_write(&RTC_ISNRC, RTC_ISNRC_ALARM);
			rtc_write(&RTC_SRC, MOD_SRC_CLRR);
			return (uint32_t) (stopwatch_elapsed(start) / test_stm_ticks_per_ms());
		}
		test_watchdog_reset();
	}

	*seen = false;
	rtc_write(&RTC_ISNRC, RTC_ISNRC_ALARM);
	rtc_write(&RTC_SRC, MOD_SRC_CLRR);
	return (uint32_t) (stopwatch_elapsed(start) / test_stm_ticks_per_ms());
}

static uint32_t t14_count(void) {
	uint32_t value = 0;

	rtc_read(&RTC_T14, &value);

	return (value & RTC_T14_CNT) >> RTC_T14_CNT_SHIFT;
}

static uint32_t rtcif_enable(void) {
	uint32_t value = 0;

	rtc_read(&SCU_RTCIF, &value);

	return value & SCU_RTCIF_RTCIFEN;
}

/* One register line per step, so which step changed what is readable off the log. */
static void state_print(const char *tag) {
	uint32_t id = 0, clc = 0, ctrl = 0, con = 0, t14 = 0, cnt = 0, isnc = 0, rtcif = 0;

	rtc_read(&RTC_ID, &id);
	rtc_read(&RTC_CLC, &clc);
	rtc_read(&RTC_CTRL, &ctrl);
	rtc_read(&RTC_CON, &con);
	rtc_read(&RTC_T14, &t14);
	rtc_read(&RTC_CNT, &cnt);
	rtc_read(&RTC_ISNC, &isnc);
	rtc_read(&SCU_RTCIF, &rtcif);

	printf("# %s: RTC_ID=%08X CLC=%08X CTRL=%08X CON=%08X T14=%08X CNT=%08X ISNC=%08X | SCU_RTCIF=%08X RTCIFEN=%02X\n",
		tag, (unsigned int) id, (unsigned int) clc, (unsigned int) ctrl,
		(unsigned int) con, (unsigned int) t14, (unsigned int) cnt,
		(unsigned int) isnc, (unsigned int) rtcif, (unsigned int) (rtcif & SCU_RTCIF_RTCIFEN));
}

/* T14 counts in an STM-dated window: the reference's rate, against a base that works. */
static uint32_t t14_window(const char *tag, uint32_t *counts_out) {
	uint64_t start = stopwatch_get();
	uint32_t first = t14_count();

	printf("# %s: counting for %u ms of STM\n# ", tag, (unsigned int) T14_WINDOW_MS);
	while (!test_elapsed_bound_ms(start, T14_WINDOW_MS)) {
		test_heartbeat();
		test_watchdog_reset();
	}
	test_heartbeat_end();

	uint32_t elapsed_ms = (uint32_t) (stopwatch_elapsed(start) / test_stm_ticks_per_ms());
	uint32_t counts = (t14_count() - first) & 0xFFFFu;

	*counts_out = counts;
	printf("# %s: %u T14 counts in %u ms of STM = %u Hz (4096 with PRE, 32768 without)\n",
		tag, (unsigned int) counts, (unsigned int) elapsed_ms,
		(unsigned int) (elapsed_ms ? counts * 1000u / elapsed_ms : 0));

	return counts;
}

/* The suite's decode of "the module clock is enabled": RMC set, DISR and DISS clear. */
static bool clc_enabled(uint32_t clc) {
	return (clc & (MOD_CLC_DISR | MOD_CLC_DISS)) == 0 && (clc & MOD_CLC_RMC) != 0;
}

static void clc_print(const char *tag, uint32_t written, uint32_t read) {
	printf("# %s: CLC wrote %08X, read %08X (DISR=%u DISS=%u RMC=%u)\n", tag,
		(unsigned int) written, (unsigned int) read,
		(unsigned int) (read & MOD_CLC_DISR), (unsigned int) (read & MOD_CLC_DISS),
		(unsigned int) ((read & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT));
}

/*
 * The EBU map, read-only.  "Not named by scripts/ke970.idc" is not evidence that an address
 * is empty: the chip-select windows are runtime-configured in ADDRSELn/BUSCONn, so an
 * unnamed address can still sit inside a live select to NOR, PSRAM or a peripheral, and a
 * store into a live select is corruption, not a probe (the tested probe stores did not
 * fault).  Standing rule: **never touch an address to probe it - neither store nor read**.
 * A read that no decode answers can stall the bus, which is the very hang this suite exists
 * to diagnose.  The question - is the address inside a live CS window - is answered by the
 * decoded map, a print, not by the cell.  The arithmetic is the project's own model of it
 * (qemu/hw/arm/pmb887x/ebu.c:ebu_update_state): base = ADDRSEL.BASE << 12, size = 1 << (27 - MASK).
 */
struct ebu_window {
	uint32_t base, size;
	bool live;
};

/* CS0..CS6: CS7 is the EMU select (EBU_EMUAS/EBU_EMUBC), not a memory window. */
#define EBU_CS_COUNT 7

static struct ebu_window ebu_windows[EBU_CS_COUNT];

static void ebu_map_print(void) {
	printf("# EBU_CLC=%08X EBU_CON=%08X SDRMCON0=%08X SDRMCON1=%08X SDRMOD0=%08X SDRMOD1=%08X SDRSTAT0=%08X SDRSTAT1=%08X\n",
		(unsigned int) EBU_CLC, (unsigned int) EBU_CON,
		(unsigned int) EBU_SDRMCON(0), (unsigned int) EBU_SDRMCON(1),
		(unsigned int) EBU_SDRMOD(0), (unsigned int) EBU_SDRMOD(1),
		(unsigned int) EBU_SDRSTAT(0), (unsigned int) EBU_SDRSTAT(1));

	for (uint32_t cs = 0; cs < EBU_CS_COUNT; cs++) {
		uint32_t addrsel = EBU_ADDRSEL(cs);
		uint32_t buscon = EBU_BUSCON(cs);
		uint32_t mask = (addrsel & EBU_ADDRSEL_MASK) >> EBU_ADDRSEL_MASK_SHIFT;

		ebu_windows[cs].live = (addrsel & EBU_ADDRSEL_REGENAB) != 0;
		ebu_windows[cs].base = ((addrsel & EBU_ADDRSEL_BASE) >> EBU_ADDRSEL_BASE_SHIFT) << 12;
		ebu_windows[cs].size = 1u << (27 - mask);

		printf("# CS%lu ADDRSEL=%08X REGENAB=%u MASK=%X ALTENAB=%u ALTSEG=%X BUSCON=%08X AGEN=%u PORTW=%u CTYPE=%u CMULT=%u WAIT=%u RO=%u\n",
			cs, (unsigned int) addrsel, (unsigned int) (addrsel & EBU_ADDRSEL_REGENAB),
			(unsigned int) mask, (unsigned int) (addrsel & EBU_ADDRSEL_ALTENAB),
			(unsigned int) ((addrsel & EBU_ADDRSEL_ALTSEG) >> EBU_ADDRSEL_ALTSEG_SHIFT),
			(unsigned int) buscon, (unsigned int) ((buscon & EBU_BUSCON_AGEN) >> EBU_BUSCON_AGEN_SHIFT),
			(unsigned int) ((buscon & EBU_BUSCON_PORTW) >> EBU_BUSCON_PORTW_SHIFT),
			(unsigned int) ((buscon & EBU_BUSCON_CTYPE) >> EBU_BUSCON_CTYPE_SHIFT),
			(unsigned int) ((buscon & EBU_BUSCON_CMULT) >> EBU_BUSCON_CMULT_SHIFT),
			(unsigned int) ((buscon & EBU_BUSCON_WAIT) >> EBU_BUSCON_WAIT_SHIFT),
			(unsigned int) ((buscon & EBU_BUSCON_WRITE) != 0));

		if (ebu_windows[cs].live)
			printf("#   CS%lu window %08X-%08X [%u MiB]%s\n", cs,
				(unsigned int) ebu_windows[cs].base,
				(unsigned int) (ebu_windows[cs].base + ebu_windows[cs].size - 1),
				(unsigned int) (ebu_windows[cs].size / 1024 / 1024),
				(buscon & EBU_BUSCON_WRITE) != 0 ? " read-only" : "");
	}
}

static int ebu_claim(uint32_t addr) {
	for (uint32_t cs = 0; cs < EBU_CS_COUNT; cs++) {
		uint32_t size = ebu_windows[cs].size;

		if (ebu_windows[cs].live && size != 0 && addr - ebu_windows[cs].base < size)
			return (int) cs;
	}

	return -1;
}

/* The verdict is the decoded map alone: whether the address falls in a live CS window or in
   none is a property of ADDRSEL/BUSCON, not of the cell, so it is printed and the address is
   never touched - neither stored to nor read. */
static void ebu_verdict(uint32_t addr) {
	int cs = ebu_claim(addr);

	if (cs >= 0)
		printf("# %08X: inside CS%d's live window - see the CS%d row above\n",
			(unsigned int) addr, cs, cs);
	else
		printf("# %08X: in no live EBU window\n", (unsigned int) addr);
}

int main(void) {
	test_start("RTC register interface probe");

	/* The two references that are not under test: the STM, and the last reset cause. */
	printf("# STM: CLC=%08X ID=%08X fSTM=%u Hz RMC=%u ticks_per_ms=%u (the base the phone shows working)\n",
		(unsigned int) STM_CLC, (unsigned int) STM_ID, (unsigned int) cpu_get_stm_freq(),
		(unsigned int) ((STM_CLC & MOD_CLC_RMC) >> MOD_CLC_RMC_SHIFT),
		(unsigned int) test_stm_ticks_per_ms());
	printf("# every RTC access below is fault-recovered; every wait is STM-bounded and renews the watchdog budget\n");

	test_category("EBU map, read-only: which windows are live and what they span");
	ebu_map_print();
	/* The two addresses the named map leaves out: verdict from the decoded map, no access. */
	ebu_verdict(0x90000000u);
	ebu_verdict(0xF9000000u);

	test_category("Register interface, before any write");
	state_print("no write yet, RTCIFEN clear");
	uint32_t before_any = 0;
	t14_window("no write yet", &before_any);
	printf("# the disabled state above is an observation, not an assertion: the enable is what this suite is here to test\n");

	/*
	 * The vendor driver's order, and the enable the corpus was missing: RTC_CLC, then
	 * SCU_RTCIF.RTCIFEN = 0xAA, and only then the block's own registers.  The direct
	 * evidence is the 2026-09-19 phone run of this suite: the same STM-dated window read
	 * 0 T14 counts before the RTCIF write and 84 after it, and RTC_ID went 00000000 ->
	 * F049C011.  A CTRL write made before the enable did not read back; the mechanism is
	 * not established (a block that masks its own registers while the interface is
	 * disabled would look the same), and the order is the measurement, not a preference.
	 */
	test_category("RTC_CLC: does the module clock request take");
	uint32_t clc_written = 1u << MOD_CLC_RMC_SHIFT;
	rtc_write(&RTC_CLC, clc_written);
	uint64_t start = stopwatch_get();
	uint32_t clc_read = 0;
	while (!test_elapsed_bound_ms(start, SETTLE_MS)) {
		rtc_read(&RTC_CLC, &clc_read);
		if ((clc_read & MOD_CLC_DISS) == 0)
			break;
		test_watchdog_reset();
	}
	clc_print("after the enable request", clc_written, clc_read);
	printf("# the DISS settle wait lasted %u us of STM\n",
		(unsigned int) (stopwatch_elapsed(start) * 1000 / test_stm_ticks_per_ms()));
	test_check("RTC_CLC reads the enabled value the suite writes", clc_enabled(clc_read));

	test_category("SCU_RTCIF: the vendor driver's enable, which this bring-up was missing");
	uint32_t rtcif_before = 0;
	rtc_read(&SCU_RTCIF, &rtcif_before);
	/*
	 * Both windows run under the same configured reference - RUN set, PRE selected, the
	 * reload written - so the RTCIF write is the only difference between them.  The phone
	 * run of the previous build measured the after-window with CON.RUN clear and reported
	 * 0 counts, which is exactly what a stopped reference does; the assertion below is
	 * about a reference that was configured first, not about a stopped one.
	 */
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	state_print("configured, before the RTCIF write");
	uint32_t counts_no_rtcif = 0;
	t14_window("before SCU_RTCIF", &counts_no_rtcif);
	rtc_write(&SCU_RTCIF, (rtcif_before & ~SCU_RTCIF_RTCIFEN) | RTCIF_ENABLE);
	uint32_t rtcif_after = 0;
	rtc_read(&SCU_RTCIF, &rtcif_after);
	printf("# SCU_RTCIF %08X -> %08X (RTCIFEN %02X -> %02X)\n",
		(unsigned int) rtcif_before, (unsigned int) rtcif_after,
		(unsigned int) (rtcif_before & SCU_RTCIF_RTCIFEN), (unsigned int) rtcif_enable());
	state_print("after SCU_RTCIF");
	test_check("SCU_RTCIF holds the vendor driver's RTCIFEN value", rtcif_enable() == RTCIF_ENABLE);
	uint32_t id_after = 0;
	rtc_read(&RTC_ID, &id_after);
	test_check("the RTC register interface answers at 0xF4700000 once RTCIFEN is set",
		(id_after & 0xFFFF0000u) == 0xF0490000u);

	uint32_t faults_before_rtcif = rtc_fault_count;
	uint32_t counts_rtcif = 0;
	t14_window("after SCU_RTCIF", &counts_rtcif);
	/* The experiment: everything else is already written and read back, so the only
	   difference from the window above is this one register. */
	printf("# T14 in the same STM-dated window: %u counts with RTCIFEN=%02X, %u counts with RTCIFEN=%02X\n",
		(unsigned int) counts_no_rtcif, (unsigned int) (rtcif_before & SCU_RTCIF_RTCIFEN),
		(unsigned int) counts_rtcif, (unsigned int) rtcif_enable());
	/* A recovered read inside the window leaves a 0 that is not a datum: the count only
	   counts if no access in its window was recovered. */
	test_check("the T14 reference counts while SCU_RTCIF carries RTCIFEN = 0xAA",
		counts_rtcif != 0 && rtc_no_recovered_access(faults_before_rtcif));

	test_category("CTRL.PU32K/CLK32KEN: the 32 kHz domain, after the enable");
	bool ctrl_written = rtc_write(&RTC_CTRL, RTC_CTRL_USE);
	uint32_t ctrl_read = 0;
	rtc_read(&RTC_CTRL, &ctrl_read);
	printf("# CTRL wrote %08X read %08X (PU32K=%u CLK32KEN=%u CLK_SEL=%u)\n",
		(unsigned int) RTC_CTRL_USE, (unsigned int) ctrl_read,
		(unsigned int) ((ctrl_read & RTC_CTRL_PU32K) != 0),
		(unsigned int) ((ctrl_read & RTC_CTRL_CLK32KEN) != 0),
		(unsigned int) ((ctrl_read & RTC_CTRL_CLK_SEL) != 0));
	test_check("RTC_CTRL keeps the 32 kHz bits it was written once RTCIFEN is set",
		ctrl_written &&
		(ctrl_read & (RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN | RTC_CTRL_CLK_SEL)) ==
		(RTC_CTRL_USE & (RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN | RTC_CTRL_CLK_SEL)));

	test_category("CTRL, CON and T14: write and read back");
	bool t14_written = rtc_write(&RTC_T14,
		(RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	uint32_t t14_read = 0;
	rtc_read(&RTC_T14, &t14_read);
	rtc_write(&RTC_CON, RTC_CON_PRE);
	uint32_t con_pre_read = 0;
	rtc_read(&RTC_CON, &con_pre_read);
	printf("# T14 wrote %08X read %08X | CON wrote %08X read %08X\n",
		(unsigned int) ((RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT)),
		(unsigned int) t14_read, (unsigned int) RTC_CON_PRE, (unsigned int) con_pre_read);
	test_check("RTC_T14 keeps the reload value it was written",
		t14_written && (t14_read & RTC_T14_REL) == RTC_T14_RELOAD);

	test_category("The suite's own configuration, with SCU_RTCIF now written");
	rtc_write(&RTC_CON, RTC_CON_RUN);
	state_print("RUN set");
	uint32_t con_run_read = 0;
	rtc_read(&RTC_CON, &con_run_read);
	test_check("CON reads back the RUN bit the suite sets", (con_run_read & RTC_CON_RUN) != 0);

	test_category("ACCPOS: the access protocol on its own");
	uint32_t con_now = 0;
	rtc_read(&RTC_CON, &con_now);
	bool accpos_now = (con_now & RTC_CON_ACCPOS) != 0;
	printf("# CON=%08X ACCPOS=%u on an immediate read\n",
		(unsigned int) con_now, (unsigned int) accpos_now);
	start = stopwatch_get();
	bool accpos_waited = false;
	while (!test_elapsed_bound_ms(start, SETTLE_MS)) {
		rtc_read(&RTC_CON, &con_now);
		if ((con_now & RTC_CON_ACCPOS) != 0) {
			accpos_waited = true;
			break;
		}
		test_watchdog_reset();
	}
	printf("# CON=%08X ACCPOS=%u after waiting %u ms of STM\n",
		(unsigned int) con_now, (unsigned int) accpos_waited,
		(unsigned int) (stopwatch_elapsed(start) / test_stm_ticks_per_ms()));
	test_check("ACCPOS sets once the block answers", accpos_now || accpos_waited);

	test_category("SCU_RST_REQ.RTC: the vendor driver's reset pulse");
	SCU_RST_REQ |= SCU_RST_REQ_RTC;
	SCU_RST_REQ &= ~SCU_RST_REQ_RTC;
	rtc_write(&RTC_CLC, clc_written);
	rtc_write(&RTC_CTRL, RTC_CTRL_USE);
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	state_print("after the reset pulse");
	uint32_t counts_pulse = 0;
	t14_window("after the RTC reset pulse", &counts_pulse);

	test_category("The vendor driver's bring-up, in its own order");
	rtc_write(&RTC_CLC, clc_written);
	rtc_write(&SCU_RTCIF, (rtcif_before & ~SCU_RTCIF_RTCIFEN) | RTCIF_ENABLE);
	uint32_t ctrl_vendor = 0;
	rtc_read(&RTC_CTRL, &ctrl_vendor);
	bool vendor_clk32k = (ctrl_vendor & RTC_CTRL_CLK32KEN) != 0;
	uint32_t clc_vendor = 0;
	rtc_read(&RTC_CLC, &clc_vendor);
	uint32_t rtcif_vendor = 0;
	rtc_read(&SCU_RTCIF, &rtcif_vendor);
	printf("# rtc_enable @0xA25EC9A8: RTC_CLC=%08X SCU_RTCIF=%08X, RTC_CTRL.CLK32KEN=%u\n",
		(unsigned int) clc_vendor, (unsigned int) rtcif_vendor, (unsigned int) vendor_clk32k);
	test_check("the vendor driver's CLK32KEN test reads the 32 kHz clock as on", vendor_clk32k);

	/*
	 * The control for the instrument itself, and now the last risky window: a stopped
	 * reference has to come back as 0 counts in the same STM-dated window, with the wait
	 * ending on its bound.  A window that cannot report a stopped reference cannot report
	 * a dead one either - it would be the wait the phone died in.
	 */
	test_category("Positive control: a stopped reference is reported, not waited out");
	rtc_write(&RTC_CON, 0);
	state_print("RUN cleared");
	uint32_t faults_before_stop = rtc_fault_count;
	uint32_t stopped = 0;
	t14_window("RUN cleared", &stopped);
	/* 0 counts is only the control if the window completed: a recovered read in it would
	   leave the same 0 for the opposite reason. */
	test_check("clearing CON.RUN stops the T14 count with no recovered access in the window",
		stopped == 0 && rtc_no_recovered_access(faults_before_stop));
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	state_print("RUN restored");

	/* What becomes measurable once the reference answers: the same two windows the
	   timers suite's prescaler row uses, dated by the STM instead. */
	test_category("The reference, dated by the STM after the bring-up");
	uint32_t faults_before_prescaler = rtc_fault_count;
	uint32_t rate_pre = 0, rate_direct = 0;
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	state_print("PRE on");
	t14_window("PRE on", &rate_pre);
	rtc_write(&RTC_CON, RTC_CON_RUN);
	t14_window("PRE off", &rate_direct);
	printf("# prescaler: %u counts without PRE / %u with PRE, dated by the STM (the suite's row is /8)\n",
		(unsigned int) rate_direct, (unsigned int) rate_pre);
	test_check("the reference counts in both prescaler windows",
		rate_direct != 0 && rate_pre != 0 && rtc_no_recovered_access(faults_before_prescaler));

	/*
	 * Granularity, measured rather than inferred.  The comparator's *semantics* are not
	 * established: a level comparator fires while `CNT == ALARM` (so `ALARM = CNT` fires at
	 * once), while this model raises ALARMIR on the transition *out of* the match (`rtc.c:85`
	 * evaluates `cnt == alarm` before the step), so `ALARM = CNT` fires at the next unit
	 * boundary.  Both arming rules are measured; the phone run decides which the part is.
	 * The unit itself is the T14 period, so the same arms are run at both prescaler settings
	 * and the 8x between them is the datum.
	 */
	test_category("Alarm granularity: RTC_ALARM against the calendar counter");
	bool alarm_seen = true;
	uint32_t lat_now = 0, lat_next = 0, lat_now_fast = 0, lat_next_fast = 0;

	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	rtc_write(&RTC_ISNC, RTC_ISNC_ALARMIE);
	lat_now = alarm_latency_ms(false, &alarm_seen);
	lat_next = alarm_latency_ms(true, &alarm_seen);
	printf("# PRE on (T14 unit = 1000 ms): ALARM = CNT -> %u ms, ALARM = CNT + 1 -> %u ms\n",
		(unsigned int) lat_now, (unsigned int) lat_next);

	rtc_write(&RTC_CON, RTC_CON_RUN);
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	lat_now_fast = alarm_latency_ms(false, &alarm_seen);
	lat_next_fast = alarm_latency_ms(true, &alarm_seen);
	printf("# PRE off (T14 unit = 125 ms): ALARM = CNT -> %u ms, ALARM = CNT + 1 -> %u ms\n",
		(unsigned int) lat_now_fast, (unsigned int) lat_next_fast);

	test_check("every alarm arm fires inside the bound", alarm_seen);
	/*
	 * The robust claim, independent of which comparison the part uses: the unit tracks the
	 * T14 period.  Compare the same arming rule at the two prescaler settings; the expected
	 * ratio is 8x (32768 Hz -> 4096 Hz).
	 */
	uint32_t unit_with_pre = lat_next ? lat_next : lat_now;
	uint32_t unit_no_pre = lat_next_fast ? lat_next_fast : lat_now_fast;
	printf("# unit ratio: %u ms with PRE / %u ms without = %s (T14 period ratio is 8)\n",
		(unsigned int) unit_with_pre, (unsigned int) unit_no_pre,
		(unit_with_pre >= 4u * unit_no_pre && unit_with_pre <= 16u * unit_no_pre)
			? "~8x, the unit is the T14 period" : "NOT the T14 ratio - see the numbers above");
	test_check("the alarm unit follows T14, not a fixed second",
		unit_with_pre != 0 && unit_no_pre != 0
		&& unit_with_pre >= 4u * unit_no_pre && unit_with_pre <= 16u * unit_no_pre);
	rtc_write(&RTC_ISNRC, RTC_ISNRC_ALARM);
	rtc_write(&RTC_SRC, MOD_SRC_CLRR);
	rtc_write(&RTC_ISNC, 0);
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));

	test_category("T14 granularity: a sub-second reload, timed through its own IRQ");
	uint32_t faults_before_t14 = rtc_fault_count;
	rtc_write(&RTC_T14, (T14_SUB_RELOAD << RTC_T14_CNT_SHIFT) | (T14_SUB_RELOAD << RTC_T14_REL_SHIFT));
	rtc_write(&RTC_ISNRC, RTC_ISNRC_T14);
	rtc_write(&RTC_SRC, MOD_SRC_CLRR);
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);
	rtc_write(&RTC_ISNC, RTC_ISNC_T14IE);
	uint32_t t14_events = 0;
	uint64_t t14_first = 0, t14_last = 0;
	uint64_t t14_start = stopwatch_get();
	uint32_t isnc_t14 = 0;
	while (t14_events < T14_EVENTS && !test_elapsed_bound_ms(t14_start, T14_BOUND_MS)) {
		rtc_read(&RTC_ISNC, &isnc_t14);
		if ((isnc_t14 & RTC_ISNC_T14IR) != 0) {
			uint64_t now = stopwatch_get();
			rtc_write(&RTC_ISNRC, RTC_ISNRC_T14);
			rtc_write(&RTC_SRC, MOD_SRC_CLRR);
			if (t14_events == 0)
				t14_first = now;
			t14_last = now;
			t14_events++;
			continue;
		}
		test_watchdog_reset();
	}
	uint32_t t14_period_us = (t14_events >= 2 && test_stm_ticks_per_ms() != 0)
		? (uint32_t) (((t14_last - t14_first) * 1000u) / (test_stm_ticks_per_ms() * (t14_events - 1u)))
		: 0;
	printf("# T14 reload %04X (512 ticks at 4096 Hz = %u us expected): %u events, measured period %u us\n",
		(unsigned int) T14_SUB_RELOAD, (unsigned int) T14_SUB_PERIOD_US,
		(unsigned int) t14_events, (unsigned int) t14_period_us);
	test_check("T14 divides below a second through its own IRQ",
		t14_events >= 2 && t14_period_us > (T14_SUB_PERIOD_US / 2u) && t14_period_us < (T14_SUB_PERIOD_US * 2u)
		&& rtc_no_recovered_access(faults_before_t14));
	rtc_write(&RTC_ISNC, 0);
	rtc_write(&RTC_ISNRC, RTC_ISNRC_T14);
	rtc_write(&RTC_SRC, MOD_SRC_CLRR);
	rtc_write(&RTC_T14, (RTC_T14_RELOAD << RTC_T14_CNT_SHIFT) | (RTC_T14_RELOAD << RTC_T14_REL_SHIFT));
	rtc_write(&RTC_CON, RTC_CON_RUN | RTC_CON_PRE);

	uint32_t id_end = 0, clc_end = 0;
	rtc_read(&RTC_ID, &id_end);
	rtc_read(&RTC_CLC, &clc_end);
	printf("# RTC register accesses whose result was RECOVERED (value unknown, never a datum): %u\n",
		(unsigned int) rtc_fault_count);
	test_check("the RTC state survives the bring-up as one block",
		(id_end & 0xFFFF0000u) == 0xF0490000u && clc_enabled(clc_end));

	return test_finish();
}
