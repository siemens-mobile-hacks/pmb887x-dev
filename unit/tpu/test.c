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

static bool wait_compare_request(uint32_t index) {
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

static bool wait_counter_at_least(uint32_t value) {
	stopwatch_t start = stopwatch_get();

	while (TPU_COUNTER < value && stopwatch_elapsed_ms(start) < TPU_TIMEOUT_MS)
		test_watchdog_serve();

	return TPU_COUNTER >= value;
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
	for (uint32_t index = 0; index < 5; index++)
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
	uint32_t quarter_rmc = measure_frequency(4, 1, 2);
	uint32_t divided_fractional = measure_frequency(1, 1, 4);
	uint32_t multiplied_fractional = measure_frequency(1, 2, 4);
	printf(
		"# counter: base %lu Hz, RMC/2 %lu Hz, RMC/4 %lu Hz, L/2 %lu Hz, K*2 %lu Hz\n",
		(uint32_t) base,
		(uint32_t) divided_rmc,
		(uint32_t) quarter_rmc,
		(uint32_t) divided_fractional,
		(uint32_t) multiplied_fractional
	);
	test_check("K/L divider produces GSM counter clock", frequency_matches(base, TPU_COUNTER_FREQUENCY));
	test_check("RMC divides counter clock", frequency_matches(divided_rmc, TPU_COUNTER_FREQUENCY / 2));
	test_check("RMC/4 quarters the counter clock", frequency_matches(quarter_rmc, TPU_COUNTER_FREQUENCY / 4));
	test_check("fractional denominator divides counter clock", frequency_matches(divided_fractional, TPU_COUNTER_FREQUENCY / 2));
	test_check("fractional numerator multiplies counter clock", frequency_matches(multiplied_fractional, TPU_COUNTER_FREQUENCY));
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
		"# clock update: active %lu Hz, without LOAD %lu Hz, after LOAD %lu Hz\n",
		(uint32_t) active,
		(uint32_t) before_load,
		(uint32_t) after_load
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
	bool reached_single_early = wait_counter_at_least(100);
	uint32_t first_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 699;
	uint32_t corrected_current = measure_wrap_us();
	uint32_t regular_after = measure_wrap_us();
	printf(
		"# correction CTRL=0 above counter: write_counter=%lu,current_tail_us=%lu,following_us=%lu\n",
		(uint32_t) first_write_counter,
		(uint32_t) corrected_current,
		(uint32_t) regular_after
	);
	test_check("counter reaches the single early correction write point", reached_single_early);
	test_check("CTRL=0 corrects the current frame", corrected_current >= 3000 && corrected_current <= 5500);
	test_check("regular overflow follows current correction", regular_after >= 6000 && regular_after <= 9000);

	TPU_PARAM = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool reached_first_early = wait_counter_at_least(100);
	first_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 699;
	bool reached_second_early = wait_counter_at_least(200);
	uint32_t second_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 299;
	uint32_t correction_after_second_write = TPU_CORRECTION;
	corrected_current = measure_wrap_us();
	regular_after = measure_wrap_us();
	bool first_correction_kept = corrected_current >= 2500 && corrected_current <= 5000;
	printf(
		"# correction CTRL=0 active rewrite: first_counter=%lu,second_counter=%lu,readback=%lu,current_tail_us=%lu,following_us=%lu\n",
		(uint32_t) first_write_counter,
		(uint32_t) second_write_counter,
		(uint32_t) correction_after_second_write,
		(uint32_t) corrected_current,
		(uint32_t) regular_after
	);
	test_check("counter reaches the first early correction write point", reached_first_early);
	test_check("counter reaches the second early correction write point", reached_second_early);
	test_eq_u32("active current-frame correction rejects a second value", 699,
		correction_after_second_write & TPU_CORRECTION_VALUE);
	test_check("active current-frame correction keeps the first overflow", first_correction_kept);
	test_check("regular overflow follows rewritten current correction", regular_after >= 6000 && regular_after <= 9000);

	TPU_PARAM = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool reached_single_late = wait_counter_at_least(700);
	first_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 199;
	uint32_t regular_tail = measure_wrap_us();
	uint32_t corrected_next = measure_wrap_us();
	regular_after = measure_wrap_us();
	printf(
		"# correction CTRL=0 below counter: write_counter=%lu,current_tail_us=%lu,next_us=%lu,following_us=%lu\n",
		(uint32_t) first_write_counter,
		(uint32_t) regular_tail,
		(uint32_t) corrected_next,
		(uint32_t) regular_after
	);
	test_check("counter reaches the single late correction write point", reached_single_late);
	test_check("CTRL=0 keeps the current frame when correction is below the counter",
		regular_tail >= 1000 && regular_tail <= 3500);
	test_check("CTRL=0 corrects the following frame", corrected_next >= 1000 && corrected_next <= 2500);
	test_check("regular overflow follows delayed correction", regular_after >= 6000 && regular_after <= 9000);

	TPU_PARAM = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	bool reached_late_rewrite = wait_counter_at_least(700);
	first_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 199;
	regular_tail = measure_wrap_us();
	bool reached_corrected_frame = wait_counter_at_least(50);
	second_write_counter = TPU_COUNTER;
	TPU_CORRECTION = 399;
	correction_after_second_write = TPU_CORRECTION;
	corrected_next = measure_wrap_us();
	regular_after = measure_wrap_us();
	printf(
		"# correction CTRL=0 delayed rewrite: first_counter=%lu,current_tail_us=%lu,second_counter=%lu,readback=%lu,next_tail_us=%lu,following_us=%lu\n",
		(uint32_t) first_write_counter,
		(uint32_t) regular_tail,
		(uint32_t) second_write_counter,
		(uint32_t) correction_after_second_write,
		(uint32_t) corrected_next,
		(uint32_t) regular_after
	);
	test_check("counter reaches the late correction rewrite point", reached_late_rewrite);
	test_check("counter enters the active corrected frame", reached_corrected_frame);
	test_eq_u32("active delayed correction rejects a second value", 199,
		correction_after_second_write & TPU_CORRECTION_VALUE);
	test_check("active delayed correction keeps the first overflow", corrected_next >= 500 && corrected_next <= 1800);
	test_check("regular overflow follows the delayed rewrite", regular_after >= 6000 && regular_after <= 9000);

	TPU_PARAM = 0;
	TPU_PARAM = TPU_PARAM_TINI | TPU_PARAM_FDIS;
	TPU_CORRECTION = TPU_CORRECTION_CTRL | 199;
	uint32_t regular_current = measure_wrap_us();
	corrected_next = measure_wrap_us();
	printf(
		"# correction CTRL=1: current %lu us, next %lu us\n",
		(uint32_t) regular_current,
		(uint32_t) corrected_next
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
		"# offset CTRL=0: first reset %lu us, following %lu us\n",
		(uint32_t) direct_offset,
		(uint32_t) following_frame
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
		"# offset CTRL=1: current %lu us, next %lu us, delayed reset %lu us\n",
		(uint32_t) regular_current,
		(uint32_t) regular_before_offset,
		(uint32_t) delayed_offset
	);
	bool regular_period_preserved = regular_before_offset >= 6000 && regular_before_offset <= 9000;
	test_check("CTRL=1 shifts reset phase after overflow", regular_current >= 8000 && regular_current <= 10000);
	test_check("shifted offset keeps regular frame period", regular_period_preserved);
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
		"# frame skip: validation %lu us, skipped reset %lu us\n",
		(uint32_t) validation_period,
		(uint32_t) skipped_period
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
