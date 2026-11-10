#include <pmb887x.h>
#include <stopwatch.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define EVENT_COUNT_OFFSET 0x0010
#define START_ITERATIONS_OFFSET 0x0020
#define FOLLOW_ITERATIONS_OFFSET 0x0021
#define INTERRUPT_ADDRESS_OFFSET 0x0040
#define START_INTERRUPT_ADDRESS 0x030
#define FOLLOW_INTERRUPT_ADDRESS 0x080
#define EVENT_COUNT 3
#define EVENT_TIMEOUT_MS 50
#define TOLERANCE_PERCENT 3
// E71: four GMSK symbols per RAM word at 13 MHz / 48 = 270.833 kHz, so one word per 14.769 us.
#define WORD_PERIOD_NS 14769

#ifdef PMB8875
#include "modulator-clock-8875.inc"
#define DSP_MODULATOR_CLOCK_IMAGE DSP_MODULATOR_CLOCK_8875
#else
#include "modulator-clock-8876.inc"
#define DSP_MODULATOR_CLOCK_IMAGE DSP_MODULATOR_CLOCK_8876
#endif

// MOD_INT_ADDR after each timed MODU; the run stops before reaching 0x1FF.
static const uint16_t INTERRUPT_ADDRESSES[EVENT_COUNT] = { 0x100, 0x100, 0x1FF };

static bool wait_events(stopwatch_t times[EVENT_COUNT]) {
	stopwatch_t start = stopwatch_get();
	uint16_t seen = 0;

	while (seen < EVENT_COUNT) {
		if (stopwatch_elapsed_ms(start) >= EVENT_TIMEOUT_MS)
			return false;
		uint16_t count = dsp_hw_shared_memory[EVENT_COUNT_OFFSET];
		while (seen < count && seen < EVENT_COUNT)
			times[seen++] = stopwatch_get();
	}
	return true;
}

static uint32_t ticks_to_us(stopwatch_t ticks) {
	return (uint32_t) (ticks * 1000 / stopwatch_ticks_per_ms());
}

static uint32_t words_to_us(uint32_t words) {
	return words * WORD_PERIOD_NS / 1000;
}

static bool test_interval_us(const char *name, uint32_t expected_us, uint32_t actual_us) {
	uint32_t tolerance_us = expected_us * TOLERANCE_PERCENT / 100;

	printf("# %s: expected %lu us, measured %lu us\n", name, expected_us, actual_us);
	return test_check(name, test_u32_in_interval(actual_us, expected_us - tolerance_us, expected_us + tolerance_us));
}

int main(void) {
	test_start("DSP modulator clock test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("digital modulator RAM read rate");
#ifdef PMB8875
	test_skip("modulator RAM read rate", "not measured on PMB8875 hardware");
	return test_finish();
#endif
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load modulator clock runner",
		dsp_hw_load_image(DSP_MODULATOR_CLOCK_IMAGE, sizeof(DSP_MODULATOR_CLOCK_IMAGE))))
		return test_finish();
	for (size_t i = 0; i < ARRAY_SIZE(INTERRUPT_ADDRESSES); i++)
		dsp_hw_shared_memory[INTERRUPT_ADDRESS_OFFSET + i] = INTERRUPT_ADDRESSES[i];
	if (!test_check("BRANCH starts modulator clock runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();

	stopwatch_t times[EVENT_COUNT];
	if (!test_check("MODU flags at addresses 0x080, 0x100, and 0x100 are observed", wait_events(times)))
		return test_finish();
	test_check("modulator clock runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, 10));

	uint32_t start_iterations = dsp_hw_shared_memory[START_ITERATIONS_OFFSET];
	uint32_t follow_iterations = dsp_hw_shared_memory[FOLLOW_ITERATIONS_OFFSET];
	uint32_t start_permille = start_iterations * (FOLLOW_INTERRUPT_ADDRESS - START_INTERRUPT_ADDRESS) * 1000 /
		(follow_iterations * START_INTERRUPT_ADDRESS);
	printf("# DSP poll iterations: MSWACT to MODU at 0x030 %lu, 0x030 to 0x080 %lu, ratio to 48:80 %lu permille\n",
		start_iterations, follow_iterations, start_permille);
	test_check("MSWACT starts reading at address 0x000 at the same word rate",
		test_u32_in_interval(start_permille, 1000 - TOLERANCE_PERCENT * 10, 1000 + TOLERANCE_PERCENT * 10));
	test_interval_us("MODU at address 0x100 follows 128 reads after address 0x080", words_to_us(0x080),
		ticks_to_us(times[1] - times[0]));
	test_interval_us("MODU at address 0x100 repeats after the 512-word ring wraps", words_to_us(0x200),
		ticks_to_us(times[2] - times[1]));

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
