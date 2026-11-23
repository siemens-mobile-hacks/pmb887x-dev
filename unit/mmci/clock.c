#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define MCI_TIMEOUT_TARGET_HZ 200
#define MCI_TIMEOUT_MIN_CYCLES 512
#define MCI_TIMEOUT_MAX_CYCLES 262144
#define MCI_TIMEOUT_WAIT_MS 2000
#define MCI_WRITE_GAP_MCLK_CYCLES 4
#define MCI_COMMAND_BITS 48
#define MCI_COMMAND_MAX_CYCLES 56
#define MCI_RMC_TIMEOUT_CYCLES 32768
#define MCI_RMC_SETTLE_US 5000

#ifdef PMB8876
/* PL180 TRM: DataCtrl and Command take no new write for three MCLK plus two PCLK periods after a write. */
static void mci_wait_write_gap(uint32_t mclk_hz) {
	stopwatch_usleep_wd(MCI_WRITE_GAP_MCLK_CYCLES * 1000000 / mclk_hz + 1);
}

static uint32_t benchmark_stm_ticks(void) {
	test_rtc_init();
	stopwatch_t first_stm = stopwatch_get();
	test_rtc_wait_second();

	return (uint32_t) stopwatch_elapsed(first_stm);
}

static bool measure_mmci_frequency(
	uint32_t expected_hz,
	uint32_t stm_counter_hz,
	uint32_t stm_rtc_hz,
	uint32_t *measured_hz,
	uint32_t *rtc_referenced_hz,
	uint32_t *elapsed_ticks
) {
	uint32_t timeout_cycles = expected_hz / MCI_TIMEOUT_TARGET_HZ;
	if (timeout_cycles < MCI_TIMEOUT_MIN_CYCLES)
		timeout_cycles = MCI_TIMEOUT_MIN_CYCLES;
	if (timeout_cycles > MCI_TIMEOUT_MAX_CYCLES)
		timeout_cycles = MCI_TIMEOUT_MAX_CYCLES;

	MCI_DATACTRL = 0;
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	MCI_DATATIMER = timeout_cycles;
	MCI_DATALENGTH = 1;
	mci_wait_write_gap(expected_hz);
	stopwatch_t start = stopwatch_get();
	MCI_DATACTRL = MCI_DATACTRL_DIRECTION_READ | MCI_DATACTRL_ENABLE;
	while ((MCI_STATUS & MCI_STATUS_DATATIMEOUT) == 0 && stopwatch_elapsed_ms(start) < MCI_TIMEOUT_WAIT_MS)
		test_watchdog_serve();

	*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	bool completed = (MCI_STATUS & MCI_STATUS_DATATIMEOUT) != 0;
	MCI_DATACTRL = 0;
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	if (!completed)
		return false;
	if (*elapsed_ticks == 0)
		return false;

	*measured_hz = (uint64_t) timeout_cycles * stm_counter_hz / *elapsed_ticks;
	*rtc_referenced_hz = stm_rtc_hz
		? (uint32_t) ((uint64_t) timeout_cycles * stm_rtc_hz / *elapsed_ticks)
		: 0;
	return true;
}

/* Check the latched stopwatch rate before converting MMCI timer ticks. */
static bool mmci_measure_time_base(uint32_t *stm_counter_hz, uint32_t *stm_rtc_hz) {
	uint32_t counter_hz = stopwatch_ticks_per_s();

	*stm_rtc_hz = benchmark_stm_ticks();
	*stm_counter_hz = counter_hz;

	printf("# STM counter: stopwatch=%lu Hz RTC=%lu Hz\n", counter_hz, *stm_rtc_hz);

	if (counter_hz != cpu_get_stm_freq())
		return false;

	return test_u32_in_interval(*stm_rtc_hz, (uint64_t) counter_hz * 98 / 100,
		(uint64_t) counter_hz * 102 / 100);
}

static bool probe_mci_command_progress(uint32_t mclk_hz, uint32_t *elapsed_ticks) {
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	mci_wait_write_gap(mclk_hz);
	stopwatch_t start = stopwatch_get();
	MCI_COMMAND = MCI_COMMAND_ENABLE;
	while ((MCI_STATUS & MCI_STATUS_CMDSENT) == 0 && stopwatch_elapsed_ms(start) < MCI_TIMEOUT_WAIT_MS)
		test_watchdog_serve();
	*elapsed_ticks = (uint32_t) stopwatch_elapsed(start);
	bool completed = (MCI_STATUS & MCI_STATUS_CMDSENT) != 0;
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
	return completed;
}

static void test_mmci_source_frequencies(void) {
	uint32_t stm_counter_hz = 0;
	uint32_t stm_rtc_hz = 0;
	bool time_base_valid = mmci_measure_time_base(&stm_counter_hz, &stm_rtc_hz);
	test_check("STM time base permits MMCI measurement", time_base_valid);
	if (!time_base_valid)
		return;

	cgu_pll_enable();
	const struct {
		const char *name;
		uint32_t source;
		uint32_t frequency_hz;
	} sources[] = {
		{ "OSC", CGU_CON3_MMCI_CLKSEL_OSC, CPU_OSC_FREQ },
		{ "CLK32K", CGU_CON3_MMCI_CLKSEL_CLK32K, CPU_CLK32K_FREQ },
		{ "PHASE4", CGU_CON3_MMCI_CLKSEL_PHASE4, cpu_get_phase_freq(4) },
	};
	static const uint32_t DIVIDERS[] = {
		CGU_CON3_MMCI_CLKDIV_DIV1,
		CGU_CON3_MMCI_CLKDIV_DIV2,
		CGU_CON3_MMCI_CLKDIV_DIV4,
		CGU_CON3_MMCI_CLKDIV_DIV8,
	};
	MMCI_CLC = (1 << MOD_CLC_RMC_SHIFT);
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	MCI_CLOCK = MCI_CLOCK_BYPASS | MCI_CLOCK_ENABLE;
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_TCR = MCI_TCR_ITEN;
	MCI_ITIP = MCI_ITIP_DATIN | MCI_ITIP_CMDIN;

	for (size_t source = 0; source < ARRAY_SIZE(sources); source++) {
		for (size_t divider = 0; divider < ARRAY_SIZE(DIVIDERS); divider++) {
			CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
				sources[source].source | DIVIDERS[divider];
			stopwatch_usleep_wd(2000);
			uint32_t mmci_hz = sources[source].frequency_hz >> divider;
			uint32_t expected_hz = mmci_hz / 8;
			uint32_t measured_hz = 0;
			uint32_t rtc_referenced_hz = 0;
			uint32_t elapsed_ticks = 0;
			bool measured = measure_mmci_frequency(expected_hz, stm_counter_hz, stm_rtc_hz,
				&measured_hz, &rtc_referenced_hz, &elapsed_ticks);

			printf("# MMCI %s /%lu: source=%lu Hz MCLK=%lu Hz measured=%lu Hz RTC=%lu Hz, %lu STM ticks\n",
				sources[source].name, (uint32_t) (1U << divider), mmci_hz,
				expected_hz, measured_hz,
				rtc_referenced_hz, elapsed_ticks);
			bool is_slowest_mode = sources[source].source == CGU_CON3_MMCI_CLKSEL_CLK32K &&
				DIVIDERS[divider] == CGU_CON3_MMCI_CLKDIV_DIV8;
			uint32_t tolerance_hz = expected_hz / 50;
			bool frequency_matches = measured &&
				test_u32_in_interval(measured_hz, expected_hz - tolerance_hz, expected_hz + tolerance_hz);
			if (is_slowest_mode) {
				test_check("MMCI CLK32K DIV8 data timer follows the CGU model", frequency_matches);
				uint32_t command_ticks = 0;
				bool command_completed = probe_mci_command_progress(expected_hz, &command_ticks);
				uint32_t command_cycles = (uint64_t) command_ticks * expected_hz / stm_counter_hz;
				printf("# MCI CLK32K DIV8 command: completed=%lu, %lu STM ticks = %lu MCLK cycles\n",
					(uint32_t) command_completed, command_ticks, command_cycles);
				bool is_command_on_mclk = command_completed &&
					test_u32_in_interval(command_cycles, MCI_COMMAND_BITS, MCI_COMMAND_MAX_CYCLES);
				test_check("MMCI CLK32K DIV8 command shifts its 48 bits at MCLK", is_command_on_mclk);
				continue;
			}
			char check[64];
			snprintf(check, sizeof(check), "MMCI follows %s /%lu", sources[source].name,
				(uint32_t) (1U << divider));
			test_check(check, frequency_matches);
		}
	}
}

static uint32_t measure_mci_timer_hz(uint32_t cycles) {
	MCI_DATACTRL = 0;
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	MCI_DATATIMER = cycles;
	MCI_DATALENGTH = 1;
	stopwatch_usleep_wd(MCI_RMC_SETTLE_US);
	stopwatch_t start = stopwatch_get();
	MCI_DATACTRL = MCI_DATACTRL_DIRECTION_READ | MCI_DATACTRL_ENABLE;
	while ((MCI_STATUS & MCI_STATUS_DATATIMEOUT) == 0 && stopwatch_elapsed_ms(start) < MCI_TIMEOUT_WAIT_MS)
		test_watchdog_serve();
	uint32_t ticks = (uint32_t) stopwatch_elapsed(start);
	bool completed = (MCI_STATUS & MCI_STATUS_DATATIMEOUT) != 0;
	MCI_DATACTRL = 0;
	if (!completed || ticks == 0)
		return 0;
	return (uint64_t) cycles * cpu_get_stm_freq() / ticks;
}

static void test_mmci_clc_divider(void) {
	static const struct mmci_clc_case {
		uint32_t rmc;
		uint32_t divider;
	} CASES[] = {
		{ 1, 8 }, { 2, 8 }, { 3, 8 }, { 4, 4 }, { 5, 5 }, { 6, 8 }, { 7, 8 }, { 8, 8 }, { 12, 8 },
	};
	CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
		CGU_CON3_MMCI_CLKSEL_OSC | CGU_CON3_MMCI_CLKDIV_DIV1;

	for (size_t i = 0; i < ARRAY_SIZE(CASES); i++) {
		MMCI_CLC = CASES[i].rmc << MOD_CLC_RMC_SHIFT;
		uint32_t expected_hz = CPU_OSC_FREQ / CASES[i].divider;
		uint32_t measured_hz = measure_mci_timer_hz(MCI_RMC_TIMEOUT_CYCLES);
		printf("# MMCI_CLC RMC=%lu: OSC/%lu=%lu Hz, measured=%lu Hz\n", CASES[i].rmc, CASES[i].divider,
			expected_hz, measured_hz);

		char check[64];
		snprintf(check, sizeof(check), "MMCI_CLC RMC=%lu clocks the data timer at OSC/%lu", CASES[i].rmc,
			CASES[i].divider);
		uint32_t tolerance_hz = expected_hz / 50;
		test_check(check, test_u32_in_interval(measured_hz, expected_hz - tolerance_hz, expected_hz + tolerance_hz));
	}
}

#endif

int main(void) {
	test_start("CGU MMCI clock test");

#ifdef PMB8876
	test_category("MMCI source frequencies");
	test_mmci_source_frequencies();
	test_category("MMCI_CLC divider");
	test_mmci_clc_divider();
#else
	test_skip("MMCI clock", "PMB8876 only");
#endif
	return test_finish();
}
