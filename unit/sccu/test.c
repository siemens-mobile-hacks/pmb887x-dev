#include <pmb887x.h>

#include "test.h"

#define SCCU_TDMA_FRAME_US 4615
#define SCCU_NQTZ_VALUE 151
#define SCCU_TIMER_TIMEOUT_MS 200
#define SCCU_CALIBRATION_TIMEOUT_MS 200
#define SCCU_DURATION_TOLERANCE_PERCENT 20
#define SCCU_FREQUENCY_MEASUREMENT_FRAMES 32
#define SCCU_SYSTEM_SLEEP_FRAMES 32
#define SCCU_POWER_DOWN_CONTROL (SCCU_SPCR_DPDN | SCCU_SPCR_DREN)

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;
static volatile uint32_t irq_tdmout;
static volatile uint32_t irq_slpctrl;

static bool wait_mask(volatile uint32_t *reg, uint32_t mask, uint32_t expected, uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();

	while ((*reg & mask) != expected && stopwatch_elapsed_ms(start) < timeout_ms)
		test_watchdog_serve();

	return (*reg & mask) == expected;
}

static void sccu_reset_timer(void) {
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPRST;
	wait_mask(&SCCU_TDMOUT, SCCU_TDMOUT_TDMAOUT, 0, SCCU_TIMER_TIMEOUT_MS);
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI;
	SCCU_WAKE_SRC = MOD_SRC_CLRR;
}

static uint32_t sccu_run_timer(uint32_t frames) {
	sccu_reset_timer();
	SCCU_TDMINI = frames - 1;
	stopwatch_t start = stopwatch_get();
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPEN;
	wait_mask(&SCCU_SLPCTRL, SCCU_SLPCTRL_SLPEN, 0, SCCU_TIMER_TIMEOUT_MS);

	return stopwatch_elapsed_us(start);
}

static bool duration_matches(uint32_t actual_us, uint32_t frames) {
	uint32_t expected_us = frames * SCCU_TDMA_FRAME_US;
	uint32_t tolerance = expected_us * SCCU_DURATION_TOLERANCE_PERCENT / 100;

	return actual_us >= expected_us - tolerance && actual_us <= expected_us + tolerance;
}

static void cpu_wait_for_interrupt(void) {
	uint32_t value = 0;

	__asm__ volatile("mcr p15, 0, %0, c7, c0, 4" : : "r" (value) : "memory");
}

static void test_registers(void) {
	SCCU_TDMINI = SCCU_TDMINI_TDMAIN;
	test_eq_u32("TDMAIN field readback", SCCU_TDMINI_TDMAIN, SCCU_TDMINI & SCCU_TDMINI_TDMAIN);
	SCCU_TDMINI = 0;

	SCCU_NQTZ = SCCU_NQTZ_NQTZ;
	test_eq_u32("NQTZ field readback", SCCU_NQTZ_NQTZ, SCCU_NQTZ & SCCU_NQTZ_NQTZ);
	SCCU_NQTZ = SCCU_NQTZ_VALUE;

	uint32_t wait = SCCU_WAIT_PREWUP | SCCU_WAIT_WAIT;
	SCCU_WAIT = wait;
	test_eq_u32("WAIT fields readback", wait, SCCU_WAIT & wait);
	SCCU_WAIT = 0;

	SCCU_REFIN = SCCU_REFIN_REFIN;
	test_eq_u32("REFIN field readback", SCCU_REFIN_REFIN, SCCU_REFIN & SCCU_REFIN_REFIN);
	SCCU_REFIN = 0;

	uint32_t wakeup = (
		SCCU_HWWAKEUP_RTC_EN | SCCU_HWWAKEUP_KPD_EN | SCCU_HWWAKEUP_SIM_EN |
		SCCU_HWWAKEUP_EXT_EN
	);
	SCCU_HWWAKEUP = wakeup | BIT(0);
	test_eq_u32("hardware wakeup fields readback", wakeup, SCCU_HWWAKEUP & wakeup);
	test_eq_u32("HWWAKEUP bit 0 is reserved", 0, SCCU_HWWAKEUP & BIT(0));
	SCCU_HWWAKEUP = 0;

	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI;
	test_eq_u32("HWACTDI readback", SCCU_SLPCTRL_HWACTDI, SCCU_SLPCTRL & SCCU_SLPCTRL_HWACTDI);
	test_check("CPU clock is active", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_CPUCLK) != 0);
	test_check("GSM clock is active", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_GSMCLK) != 0);
	test_check("state machine reports MCU on", (SCCU_SCCUMSTA & SCCU_SCCUMSTA_UC_ON) != 0);
}

static void test_timer_duration(void) {
	SCCU_NQTZ = SCCU_NQTZ_VALUE;
	SCCU_WAIT = 0;
	uint32_t one_frame = sccu_run_timer(1);
	uint32_t two_frames = sccu_run_timer(2);
	uint32_t four_frames = sccu_run_timer(4);

	printf(
		"# sleep timer: 1 frame %u us, 2 frames %u us, 4 frames %u us\n",
		(unsigned int) one_frame,
		(unsigned int) two_frames,
		(unsigned int) four_frames
	);
	test_check("one-frame duration", duration_matches(one_frame, 1));
	test_check("two-frame duration", duration_matches(two_frames, 2));
	test_check("four-frame duration", duration_matches(four_frames, 4));
	test_eq_u32("completed timer reports four frames", 3, SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT);
	test_check("completed timer clears SLPEN", (SCCU_SLPCTRL & SCCU_SLPCTRL_SLPEN) == 0);
	test_check("completed timer restores GSM clock", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_GSMCLK) != 0);

	uint32_t frequency_duration = sccu_run_timer(SCCU_FREQUENCY_MEASUREMENT_FRAMES);
	uint32_t timer_millihz = (
		(uint64_t) SCCU_FREQUENCY_MEASUREMENT_FRAMES * 1000000000ULL / frequency_duration
	);
	uint32_t standby_hz = (
		(uint64_t) SCCU_FREQUENCY_MEASUREMENT_FRAMES * SCCU_NQTZ_VALUE * 1000000ULL / frequency_duration
	);
	printf(
		"# sleep timer frequency: %u.%03u Hz, estimated standby clock: %u Hz (%u frames, %u us)\n",
		(unsigned int) (timer_millihz / 1000),
		(unsigned int) (timer_millihz % 1000),
		(unsigned int) standby_hz,
		SCCU_FREQUENCY_MEASUREMENT_FRAMES,
		(unsigned int) frequency_duration
	);
	test_check(
		"frequency measurement duration",
		duration_matches(frequency_duration, SCCU_FREQUENCY_MEASUREMENT_FRAMES)
	);
	test_eq_u32(
		"frequency measurement frame count",
		SCCU_FREQUENCY_MEASUREMENT_FRAMES - 1,
		SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT
	);
}

static void test_timer_stop(void) {
	sccu_reset_timer();
	SCCU_NQTZ = SCCU_NQTZ_VALUE;
	SCCU_TDMINI = 15;
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPEN;
	test_check("sleep counter starts", wait_mask(&SCCU_TDMOUT, SCCU_TDMOUT_TDMAOUT, 2, SCCU_TIMER_TIMEOUT_MS));
	printf(
		"# timer active: CLKSTA=%08X, MSTA=%08X\n",
		(unsigned int) SCCU_SCCUCLKSTA,
		(unsigned int) SCCU_SCCUMSTA
	);
	test_check("sleep timer stops GSM clock", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_GSMCLK) == 0);
	test_check("sleep timer keeps CPU clock active", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_CPUCLK) != 0);
	SCCU_SLPCTRL = SCCU_SLPCTRL_SLPSTP;
	test_check("SLPSTP stops timer", wait_mask(&SCCU_SLPCTRL, SCCU_SLPCTRL_SLPEN, 0, SCCU_TIMER_TIMEOUT_MS));
	uint32_t stopped = SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT;
	stopwatch_usleep_wd(SCCU_TDMA_FRAME_US * 2);
	test_eq_u32("stopped counter remains stable", stopped, SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT);
	test_check("early stop occurs before programmed duration", stopped < 15);
	test_check("early stop restores GSM clock", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_GSMCLK) != 0);

	SCCU_SLPCTRL = SCCU_SLPCTRL_SLPRST;
	test_check("SLPRST clears sleep counter", wait_mask(
		&SCCU_TDMOUT,
		SCCU_TDMOUT_TDMAOUT,
		0,
		SCCU_TIMER_TIMEOUT_MS
	));
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI;
}

static void test_switch_control(void) {
	SCCU_SCCTRL = SCCU_SCCTRL_UCWUP;
	test_check("UCWUP command clears", wait_mask(&SCCU_SCCTRL, SCCU_SCCTRL_UCWUP, 0, SCCU_TIMER_TIMEOUT_MS));
	test_check("UCWUP leaves MCU on", (SCCU_SCCUMSTA & SCCU_SCCUMSTA_UC_ON) != 0);

	SCCU_SCCTRL = SCCU_SCCTRL_SSCRST;
	test_check("SSCRST command clears", wait_mask(&SCCU_SCCTRL, SCCU_SCCTRL_SSCRST, 0, SCCU_TIMER_TIMEOUT_MS));
	test_check("SSCRST returns state machine to MCU on", (SCCU_SCCUMSTA & SCCU_SCCUMSTA_UC_ON) != 0);
}

static void test_calibration(void) {
	sccu_reset_timer();
	SCCU_NQTZ = SCCU_NQTZ_VALUE;
	SCCU_SLPCTRL = SCCU_SLPCTRL_REFEN | SCCU_SLPCTRL_HWACTDI;
	bool completed = wait_mask(&SCCU_SLPCTRL, SCCU_SLPCTRL_REFEN, 0, SCCU_CALIBRATION_TIMEOUT_MS);
	uint32_t ref = SCCU_REF;
	uint32_t refout = (ref & SCCU_REF_REFOUT) >> SCCU_REF_REFOUT_SHIFT;
	uint32_t refpos = (ref & SCCU_REF_REFPOS) >> SCCU_REF_REFPOS_SHIFT;

	printf("# calibration: REFPOS=%u, REFOUT=%u\n", (unsigned int) refpos, (unsigned int) refout);
	test_check("calibration completes", completed);
	test_check("calibration has no reference error", (SCCU_SLPCTRL & SCCU_SLPCTRL_REFERR) == 0);
	test_eq_u32("calibration covers 16 TDMA frames", 128, refpos);
	test_check("calibration fine result is valid", refout > 16);
}

static void test_wakeup_interrupt(void) {
	sccu_reset_timer();
	irq_count = 0;
	irq_number = 0;
	irq_tdmout = 0;
	irq_slpctrl = 0;
	SCCU_NQTZ = SCCU_NQTZ_VALUE;
	SCCU_WAIT = SCCU_WAIT_PREWUP;
	SCCU_WAKE_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_SCCU_WAKE_IRQ) = 1;
	cpu_enable_irq(true);

	SCCU_TDMINI = 7;
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPEN;
	test_check("wakeup IRQ fires", wait_mask(&irq_count, UINT32_MAX, 1, SCCU_TIMER_TIMEOUT_MS));
	test_check("timer completes after wakeup IRQ", wait_mask(
		&SCCU_SLPCTRL,
		SCCU_SLPCTRL_SLPEN,
		0,
		SCCU_TIMER_TIMEOUT_MS
	));
	cpu_enable_irq(false);

	printf("# wakeup IRQ: TDMAOUT=%u\n", (unsigned int) irq_tdmout);
	test_eq_u32("wakeup IRQ fires once", 1, irq_count);
	test_eq_u32("wakeup IRQ number", VIC_SCCU_WAKE_IRQ, irq_number);
	test_eq_u32("wakeup IRQ occurs on final frame", 7, irq_tdmout);
	test_check("wakeup IRQ sees active timer", (irq_slpctrl & SCCU_SLPCTRL_SLPEN) != 0);
	test_check("wakeup SRC is cleared", (SCCU_WAKE_SRC & MOD_SRC_SRR) == 0);
}

static void test_system_sleep(uint32_t power_control) {
	sccu_reset_timer();
	irq_count = 0;
	irq_number = 0;
	irq_tdmout = 0;
	irq_slpctrl = 0;
	SCCU_SPCR = power_control;
	SCCU_NQTZ = SCCU_NQTZ_VALUE;
	SCCU_WAIT = SCCU_WAIT_PREWUP;
	SCCU_WAKE_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(VIC_SCCU_WAKE_IRQ) = 1;
	cpu_enable_irq(true);

	SCCU_TDMINI = SCCU_SYSTEM_SLEEP_FRAMES - 1;
	stopwatch_t start = stopwatch_get();
	SCCU_SLPCTRL = SCCU_SLPCTRL_HWACTDI | SCCU_SLPCTRL_SLPEN;
	SCCU_SCCTRL = SCCU_SCCTRL_UCSLP;
	bool powered_down = wait_mask(
		&SCCU_SCCUMSTA,
		SCCU_SCCUMSTA_TCXO_OFF,
		SCCU_SCCUMSTA_TCXO_OFF,
		SCCU_TIMER_TIMEOUT_MS
	);
	uint32_t sleep_clksta = SCCU_SCCUCLKSTA;
	uint32_t sleep_msta = SCCU_SCCUMSTA;
	cpu_wait_for_interrupt();
	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	bool irq_received = wait_mask(&irq_count, UINT32_MAX, 1, SCCU_TIMER_TIMEOUT_MS);
	bool timer_completed = wait_mask(
		&SCCU_SLPCTRL,
		SCCU_SLPCTRL_SLPEN,
		0,
		SCCU_TIMER_TIMEOUT_MS
	);
	cpu_enable_irq(false);

	printf(
		"# system sleep: active-clock time %u us, SPCR=%08X, CLKSTA=%08X, MSTA=%08X\n",
		(unsigned int) elapsed_us,
		(unsigned int) SCCU_SPCR,
		(unsigned int) sleep_clksta,
		(unsigned int) sleep_msta
	);
	test_eq_u32("standby power control", power_control, SCCU_SPCR & SCCU_POWER_DOWN_CONTROL);
	test_check("system enters TCXO-off state", powered_down);
	test_check("system sleep switches CPU to standby clock", (sleep_clksta & SCCU_SCCUCLKSTA_CPUCLK) == 0);
	test_check("system sleep stops GSM clock", (sleep_clksta & SCCU_SCCUCLKSTA_GSMCLK) == 0);
	test_check("WFI returns on SCCU IRQ", irq_received);
	test_eq_u32("system sleep wakeup IRQ count", 1, irq_count);
	test_eq_u32("system sleep wakeup IRQ number", VIC_SCCU_WAKE_IRQ, irq_number);
	test_eq_u32("system sleep completes programmed frames", SCCU_SYSTEM_SLEEP_FRAMES - 1, irq_tdmout);
	test_check("sleep timer completes after WFI", timer_completed);
	test_check("wakeup restores CPU clock", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_CPUCLK) != 0);
	test_check("wakeup restores GSM clock", (SCCU_SCCUCLKSTA & SCCU_SCCUCLKSTA_GSMCLK) != 0);
	test_check("wakeup returns state machine to MCU on", (SCCU_SCCUMSTA & SCCU_SCCUMSTA_UC_ON) != 0);
}

int main(void) {
	test_start("SCCU peripheral test");

	SCU_CLC = 2 << MOD_CLC_RMC_SHIFT;
	test_module_clock("module clock is enabled", SCU_CLC);

	test_category("Registers");
	test_registers();

	test_category("Sleep timer");
	test_timer_duration();
	test_timer_stop();
	test_switch_control();

	test_category("Standby clock calibration");
	test_calibration();

	test_category("Wakeup interrupt");
	test_wakeup_interrupt();

	test_category("System sleep");
	test_system_sleep(0);

	test_category("Standby power-down");
	test_system_sleep(SCCU_POWER_DOWN_CONTROL);

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	if (irq_number == VIC_SCCU_WAKE_IRQ) {
		irq_count++;
		irq_tdmout = SCCU_TDMOUT & SCCU_TDMOUT_TDMAOUT;
		irq_slpctrl = SCCU_SLPCTRL;
		SCCU_WAKE_SRC = MOD_SRC_CLRR;
	}
	VIC_IRQ_ACK = 1;
}
