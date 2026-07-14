#include <pmb887x.h>

#include "test.h"

#define TPU_COUNTER_FREQUENCY 2166666
#define TPU_FREQUENCY_TOLERANCE_PERCENT 5
#define TPU_MEASURE_US 2000
#define TPU_TIMEOUT_MS 100

static void tpu_configure_clock(uint32_t rmc, uint32_t k, uint32_t l) {
	TPU_CLC = rmc << MOD_CLC_RMC_SHIFT;
	TPU_GSMCLK1 = k << TPU_GSMCLK1_K_SHIFT;
	TPU_GSMCLK2 = l << TPU_GSMCLK2_L_SHIFT;
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD | TPU_GSMCLK3_INIT;
}

static bool wait_compare_request(unsigned int index) {
	stopwatch_t start = stopwatch_get();

	while ((TPU_SRC(index) & MOD_SRC_SRR) == 0 && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return (TPU_SRC(index) & MOD_SRC_SRR) != 0;
}

static uint32_t measure_wrap_us(void) {
	uint32_t previous = TPU_COUNTER;
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS) {
		uint32_t current = TPU_COUNTER;
		if (current < previous)
			return stopwatch_elapsed_us(start);
		previous = current;
		test_watchdog_serve();
	}

	return 0;
}

static bool frequency_matches(uint32_t actual, uint32_t expected) {
	uint32_t tolerance = expected * TPU_FREQUENCY_TOLERANCE_PERCENT / 100;

	return actual >= expected - tolerance && actual <= expected + tolerance;
}

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, TPU_CLC);
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("RFCON1 reset value", 0, TPU_RFCON1);
	test_eq_u32("RFCON2 reset value", 0, TPU_RFCON2);
	test_eq_u32("CORRECTION reset value", 0, TPU_CORRECTION);
	test_eq_u32("OVERFLOW reset value", 0x270F, TPU_OVERFLOW);
	for (uint32_t index = 0; index < 2; index++)
		test_eq_u32("INT reset value", TPU_INT_VALUE, TPU_INT(index));
	test_eq_u32("OFFSET reset value", 0, TPU_OFFSET);
	test_eq_u32("SKIP reset value", 0, TPU_SKIP);
	test_eq_u32("COUNTER reset value", 0, TPU_COUNTER);
	test_eq_u32("CEAP reset value", 0, TPU_CEAP);
	test_eq_u32("EAPT reset value", 0, TPU_EAPT);
	test_eq_u32("EAPB reset value", 0, TPU_EAPB);
	test_eq_u32("TGER reset value", 0, TPU_TGER);
	test_eq_u32("PARAM reset value", 0, TPU_PARAM);
	test_eq_u32("FADE reset value", 0x700, TPU_FADE);
	test_eq_u32("GSMCLK1 reset value", 1 << TPU_GSMCLK1_K_SHIFT, TPU_GSMCLK1);
	test_eq_u32("GSMCLK2 reset value", 2 << TPU_GSMCLK2_L_SHIFT, TPU_GSMCLK2);
	test_eq_u32("GSMCLK3 reset value", 0, TPU_GSMCLK3);
	test_eq_u32("unknown register reset value", 0, TPU_UNK);
	uint32_t src_config = MOD_SRC_SRPN | MOD_SRC_TOS | MOD_SRC_SRE;
	test_eq_u32("RF SSC SRC routing reset value", 0, TPU_RFSSC_SRC & src_config);
	for (uint32_t index = 0; index < 6; index++)
		test_eq_u32("GP SRC routing reset value", 0, TPU_GP_SRC(index) & src_config);
	for (uint32_t index = 0; index < 2; index++)
		test_eq_u32("compare SRC routing reset value", 0, TPU_SRC(index) & src_config);
}

static uint32_t measure_frequency(uint32_t rmc, uint32_t k, uint32_t l) {
	TPU_PARAM = 0;
	tpu_configure_clock(rmc, k, l);
	TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	stopwatch_t start = stopwatch_get();
	uint32_t first = TPU_COUNTER;
	stopwatch_usleep_wd(TPU_MEASURE_US);
	uint32_t last = TPU_COUNTER;
	uint32_t elapsed_us = stopwatch_elapsed_us(start);
	TPU_PARAM = 0;

	return (uint32_t) ((uint64_t) (last - first) * 1000000 / elapsed_us);
}

static uint32_t measure_running_frequency(void) {
	stopwatch_t start = stopwatch_get();
	uint32_t first = TPU_COUNTER;
	stopwatch_usleep_wd(TPU_MEASURE_US);
	uint32_t last = TPU_COUNTER;

	return (uint32_t) ((uint64_t) (last - first) * 1000000 / stopwatch_elapsed_us(start));
}

static void test_clock(void) {
	test_category("Counter clock");
	uint32_t base = measure_frequency(1, 1, 2);
	uint32_t divided_rmc = measure_frequency(2, 1, 2);
	uint32_t divided_fractional = measure_frequency(1, 1, 4);
	uint32_t multiplied_fractional = measure_frequency(1, 2, 4);
	printf(
		"# counter: base %u Hz, RMC/2 %u Hz, L/2 %u Hz, K*2 %u Hz\n",
		(unsigned int) base,
		(unsigned int) divided_rmc,
		(unsigned int) divided_fractional,
		(unsigned int) multiplied_fractional
	);
	test_check("K/L divider produces GSM counter clock", frequency_matches(base, TPU_COUNTER_FREQUENCY));
	test_check("RMC divides counter clock", frequency_matches(divided_rmc, TPU_COUNTER_FREQUENCY / 2));
	test_check("fractional denominator divides counter clock", frequency_matches(
		divided_fractional, TPU_COUNTER_FREQUENCY / 2
	));
	test_check("fractional numerator multiplies counter clock", frequency_matches(
		multiplied_fractional, TPU_COUNTER_FREQUENCY
	));
	test_eq_u32("GSMCLK LOAD and INIT are self-clearing", 0, TPU_GSMCLK3);

	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = TPU_OVERFLOW_VALUE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	uint32_t active = measure_running_frequency();
	TPU_GSMCLK1 = 2;
	uint32_t before_load = measure_running_frequency();
	TPU_GSMCLK3 = TPU_GSMCLK3_LOAD;
	uint32_t after_load = measure_running_frequency();
	printf(
		"# clock update: active %u Hz, without LOAD %u Hz, after LOAD %u Hz\n",
		(unsigned int) active,
		(unsigned int) before_load,
		(unsigned int) after_load
	);
	test_check("K/L write without LOAD keeps old clock", frequency_matches(before_load, active));
	test_check("LOAD activates new K/L clock", frequency_matches(after_load, active * 2));
	TPU_PARAM = 0;
}

static void test_control_and_overflow(void) {
	test_category("Counter control and modulo overflow");
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = 99;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("TINI starts counter", measure_wrap_us() != 0);
	test_check("counter stays within overflow value", TPU_COUNTER <= 99);
	TPU_PARAM = 0;
	stopwatch_usleep_wd(1000);
	test_eq_u32("clearing TINI resets and stops counter", 0, TPU_COUNTER);
}

static void test_correction(void) {
	test_category("Counter correction");
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_CORRECTION = 199;
	uint32_t corrected_current = measure_wrap_us();
	uint32_t regular_after = measure_wrap_us();
	printf(
		"# correction CTRL=0: current %u us, following %u us\n",
		(unsigned int) corrected_current,
		(unsigned int) regular_after
	);
	test_check("CTRL=0 corrects current frame", corrected_current >= 1000 && corrected_current <= 2500);
	test_check("regular overflow follows current correction", regular_after >= 6000 && regular_after <= 9000);

	TPU_PARAM = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_CORRECTION = TPU_CORRECTION_CTRL | 199;
	uint32_t regular_current = measure_wrap_us();
	uint32_t corrected_next = measure_wrap_us();
	printf(
		"# correction CTRL=1: current %u us, next %u us\n",
		(unsigned int) regular_current,
		(unsigned int) corrected_next
	);
	test_check("CTRL=1 keeps current frame regular", regular_current >= 6000 && regular_current <= 9000);
	test_check("CTRL=1 corrects next frame", corrected_next >= 1000 && corrected_next <= 2500);
	TPU_PARAM = 0;
}

static void test_offset(void) {
	test_category("CTDMA offset");
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_OFFSET = 199;
	uint32_t direct_offset = measure_wrap_us();
	uint32_t following_frame = measure_wrap_us();
	printf(
		"# offset CTRL=0: first reset %u us, following %u us\n",
		(unsigned int) direct_offset,
		(unsigned int) following_frame
	);
	test_check("CTRL=0 applies offset in current frame", direct_offset >= 1000 && direct_offset <= 2500);
	test_check("offset reset repeats every frame", following_frame >= 6000 && following_frame <= 9000);

	TPU_PARAM = 0;
	TPU_OFFSET = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_OFFSET = TPU_OFFSET_CTRL | 199;
	uint32_t regular_current = measure_wrap_us();
	uint32_t regular_before_offset = measure_wrap_us();
	uint32_t delayed_offset = measure_wrap_us();
	printf(
		"# offset CTRL=1: current %u us, next %u us, delayed reset %u us\n",
		(unsigned int) regular_current,
		(unsigned int) regular_before_offset,
		(unsigned int) delayed_offset
	);
	test_check("CTRL=1 shifts reset phase after overflow", regular_current >= 8000 && regular_current <= 10000);
	test_check("shifted offset keeps regular frame period", (
		regular_before_offset >= 6000 && regular_before_offset <= 9000
	));
	test_check("shifted offset remains periodic", delayed_offset >= 6000 && delayed_offset <= 9000);
	TPU_PARAM = 0;
}

static void test_frame_skip(void) {
	test_category("CTDMA frame skip");
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = 999;
	TPU_OFFSET = 199;
	TPU_SKIP = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	measure_wrap_us();
	TPU_SKIP = TPU_SKIP_SKIPN;
	uint32_t validation_period = measure_wrap_us();
	uint32_t skipped_period = measure_wrap_us();
	printf(
		"# frame skip: validation %u us, skipped reset %u us\n",
		(unsigned int) validation_period,
		(unsigned int) skipped_period
	);
	test_check("SKIPN is validated after one frame", validation_period >= 6000 && validation_period <= 9000);
	test_check("SKIPN skips one CTDMA reset", skipped_period >= 13000 && skipped_period <= 17000);
	test_eq_u32("skip state clears after skipped reset", 0, TPU_SKIP & (TPU_SKIP_SKIPN | TPU_SKIP_SKIPC));
	TPU_PARAM = 0;
}

static void test_compare_interrupts(void) {
	test_category("Counter compare interrupts");
	TPU_PARAM = 0;
	tpu_configure_clock(1, 1, 32);
	TPU_OVERFLOW = 999;
	TPU_INT(0) = 200;
	TPU_INT(1) = 700;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("INT0 compare reaches SRC0", wait_compare_request(0));
	test_check("INT0 occurs before INT1", (TPU_SRC(1) & MOD_SRC_SRR) == 0);
	test_check("INT1 compare reaches SRC1", wait_compare_request(1));
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
	test_check("compare requests repeat after overflow", wait_compare_request(0));
	test_check("second INT1 compare reaches SRC1", wait_compare_request(1));
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;

	TPU_INT(0) = 1000;
	TPU_INT(1) = 999;
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("compare at overflow value reaches SRC1", wait_compare_request(1));
	test_eq_u32("compare above overflow does not fire", 0, TPU_SRC(0) & MOD_SRC_SRR);
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;

	TPU_INT(0) = 200;
	TPU_INT(1) = 200;
	TPU_SRC(0) = MOD_SRC_SRE;
	TPU_SRC(1) = MOD_SRC_SRE;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	test_check("equal compares reach SRC0", wait_compare_request(0));
	test_check("equal compares also reach SRC1", wait_compare_request(1));
	TPU_PARAM = 0;
	TPU_SRC(0) = MOD_SRC_CLRR;
	TPU_SRC(1) = MOD_SRC_CLRR;
}

int main(void) {
	test_start("TPU peripheral test");
	test_reset_values();
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_id("module ID", 0xF021C000, TPU_ID);
	test_module_clock("module clock is enabled", TPU_CLC);
	test_clock();
	test_control_and_overflow();
	test_compare_interrupts();
	test_correction();
	test_offset();
	test_frame_skip();

	return test_finish();
}
