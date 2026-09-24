#include <pmb887x.h>

#include "cgu/clock.h"
#include "test.h"

#define MCI_TIMEOUT_TARGET_HZ 200
#define MCI_TIMEOUT_MIN_CYCLES 512
#define MCI_TIMEOUT_MAX_CYCLES 262144
#define MCI_TIMEOUT_WAIT_MS 2000

#ifdef PMB8876
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
	stopwatch_usleep_wd(1000);
	MCI_CLEAR = MCI_CLEAR_DATATIMEOUTCLR;
	stopwatch_usleep_wd(1000);
	MCI_DATATIMER = timeout_cycles;
	MCI_DATALENGTH = 1;
	stopwatch_usleep_wd(1000);
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

static bool probe_mci_command_progress(uint32_t *elapsed_ticks) {
	MCI_COMMAND = 0;
	MCI_CLEAR = MCI_CLEAR_CMDSENTCLR;
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

	static const struct {
		const char *name;
		const char *test_name;
		uint32_t source;
		uint32_t frequency_hz;
	} SOURCES[] = {
		{ "OSC", "OSC source and divider rates", CGU_CON3_MMCI_CLKSEL_OSC, CPU_OSC_FREQ },
		{ "CLK32K", "CLK32K source and divider rates", CGU_CON3_MMCI_CLKSEL_CLK32K, CPU_CLK32K_FREQ },
		{ "PHASE4", "phase 4 source and divider rates", CGU_CON3_MMCI_CLKSEL_PHASE4, 96000000 },
	};
	static const uint32_t DIVIDERS[] = {
		CGU_CON3_MMCI_CLKDIV_DIV1,
		CGU_CON3_MMCI_CLKDIV_DIV2,
		CGU_CON3_MMCI_CLKDIV_DIV4,
		CGU_CON3_MMCI_CLKDIV_DIV8,
	};
	cgu_pll_set(7, 1);
	cgu_phases_enable(CGU_PHASE4);
	cgu_phase_set_divider(4, 2, 1);
	MMCI_CLC = (1 << MOD_CLC_RMC_SHIFT);
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	MCI_CLOCK = MCI_CLOCK_BYPASS | MCI_CLOCK_ENABLE;
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_TCR = MCI_TCR_ITEN;
	MCI_ITIP = MCI_ITIP_DATIN | MCI_ITIP_CMDIN;

	bool source_matches[ARRAY_SIZE(SOURCES)] = { false };
	bool slowest_data_timer_matches = false;
	bool slowest_command_stopped = false;
	for (size_t source = 0; source < ARRAY_SIZE(SOURCES); source++) {
		source_matches[source] = true;
		for (size_t divider = 0; divider < ARRAY_SIZE(DIVIDERS); divider++) {
			CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
				SOURCES[source].source | DIVIDERS[divider];
			stopwatch_usleep_wd(2000);
			uint32_t mmci_hz = SOURCES[source].frequency_hz >> divider;
			uint32_t expected_hz = mmci_hz / 8;
			uint32_t measured_hz = 0;
			uint32_t rtc_referenced_hz = 0;
			uint32_t elapsed_ticks = 0;
			bool measured = measure_mmci_frequency(expected_hz, stm_counter_hz, stm_rtc_hz,
				&measured_hz, &rtc_referenced_hz, &elapsed_ticks);

			printf("# MMCI %s /%lu: source=%lu Hz MCLK=%lu Hz measured=%lu Hz RTC=%lu Hz, %lu STM ticks\n",
				SOURCES[source].name, (uint32_t) (1U << divider), mmci_hz,
				expected_hz, measured_hz,
				rtc_referenced_hz, elapsed_ticks);
			bool is_slowest_mode = SOURCES[source].source == CGU_CON3_MMCI_CLKSEL_CLK32K &&
				DIVIDERS[divider] == CGU_CON3_MMCI_CLKDIV_DIV8;
			if (is_slowest_mode) {
				uint32_t command_ticks = 0;
				bool command_completed = probe_mci_command_progress(&command_ticks);
				printf("# MCI CLK32K DIV8 command progress: completed=%lu elapsed=%lu STM ticks\n",
					(uint32_t) command_completed, command_ticks);
				uint32_t tolerance_hz = expected_hz / 50;
				slowest_data_timer_matches = measured &&
					test_u32_in_interval(measured_hz, expected_hz - tolerance_hz, expected_hz + tolerance_hz);
				slowest_command_stopped = !command_completed;
				continue;
			}
			uint32_t tolerance_hz = expected_hz / 50;
			bool frequency_matches = measured &&
				test_u32_in_interval(measured_hz, expected_hz - tolerance_hz, expected_hz + tolerance_hz);
			source_matches[source] = source_matches[source] && frequency_matches;
		}
	}

	for (size_t source = 0; source < ARRAY_SIZE(SOURCES); source++)
		test_check(SOURCES[source].test_name, source_matches[source]);
	test_check("MMCI CLK32K DIV8 data timer follows the CGU model", slowest_data_timer_matches);
	test_check("MMCI CLK32K DIV8 command engine stalls", slowest_command_stopped);
}
#endif

int main(void) {
	test_start("CGU MMCI clock test");

#ifdef PMB8876
	test_category("MMCI source frequencies");
	test_mmci_source_frequencies();
#else
	test_skip("MMCI clock", "PMB8876 only");
#endif
	return test_finish();
}
