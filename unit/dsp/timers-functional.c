#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define RESULT_BASE 0x0300
#define RESULT_LAST 0x0340
#define RESULT_WORDS (RESULT_LAST - RESULT_BASE + 1)
#define RESULT_COMPLETE 0xA55A
#define TIMER2_SAMPLES_BASE 0x0314
#define TIMER2_WRAP_SAMPLES_LAST 0x0324
#define TIMER2_ACTIVE_WRITE_SAMPLES_BASE 0x0325
#define TIMER2_SAMPLES_LAST 0x032C

#ifdef PMB8875
#include "timers-functional-8875.inc"
#define DSP_TIMERS_IMAGE DSP_TIMERS_IMAGE_8875
#else
#include "timers-functional-8876.inc"
#define DSP_TIMERS_IMAGE DSP_TIMERS_IMAGE_8876
#endif

static uint16_t first_record[RESULT_WORDS];

static uint16_t result(size_t address) {
	return dsp_hw_shared_memory[address];
}

static void print_record(size_t pass) {
	printf("# DSPTIMERS,%u", (uint32_t) pass);
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		printf(",%04X", (uint32_t) result(address));
	printf("\n");
}

static bool timer2_has_wrap_sequence(void) {
	static const uint16_t sequence[] = { 0, 1, 2, 3, 4, 0 };
	size_t matched = 0;
	uint16_t previous = UINT16_MAX;

	for (size_t address = TIMER2_SAMPLES_BASE; address <= TIMER2_WRAP_SAMPLES_LAST; address++) {
		uint16_t sample = result(address);

		if (sample == previous)
			continue;
		previous = sample;
		if (sample == sequence[matched]) {
			matched++;
			if (matched == ARRAY_SIZE(sequence))
				return true;
		} else {
			matched = sample == sequence[0] ? 1 : 0;
		}
	}

	return false;
}

static void check_timer1(void) {
	test_category("TIMER1 / Functional behavior");
	test_eq_u32("enabled TIMER1 remains at zero without RESTART", 0, result(0x0303));
	test_eq_u32("TIMER1 remains in standby over time", 0, result(0x0304));
	test_check("TIMER1 progresses after RESTART", result(0x0306) > result(0x0305));
	test_check("TIMER1 first snapshot precedes compare 1", result(0x0306) < 0x0020);
	test_check("TIMER1 compare 0 enters the INT1 handler", result(0x0307) >= 1);
	test_check("TIMER1 compare 0 is identified by the handler",
		(result(0x0308) & TEAK_INT_FINT1_TMR10) != 0 && (result(0x0308) & TEAK_INT_FINT1_TMR11) == 0);
	test_check("TIMER1 continues after compare 0", result(0x030A) > result(0x0306));
	test_check("TIMER1 compare 1 enters the INT1 handler", result(0x030B) >= 2);
	test_check("TIMER1 compare 1 is identified by the handler", (result(0x030C) & TEAK_INT_FINT1_TMR11) != 0);
	test_eq_u32("stopped TIMER1 retains its count", result(0x030D), result(0x030E));
	test_eq_u32("TIMER1 stops automatically at terminal count", 0x0FFF, result(0x030F));
	test_eq_u32("terminal TIMER1 is enabled but inactive", TEAK_TMR1_CTRL_DT1ENA,
		result(0x0310) & (TEAK_TMR1_CTRL_DT1ENA | TEAK_TMR1_CTRL_DT1ACT));
	test_eq_u32("terminal TIMER1 raises both compare flags",
		TEAK_INT_FINT1_TMR10 | TEAK_INT_FINT1_TMR11,
		result(0x0311) & (TEAK_INT_FINT1_TMR10 | TEAK_INT_FINT1_TMR11));
	test_eq_u32("TIMER1 retains terminal count during restart synchronization", 0x0FFF, result(0x0312));
	test_check("TIMER1 restarts from zero after synchronization", result(0x0313) < 0x0020);
}

static void check_timer2(void) {
	test_category("TIMER2 / Functional behavior");
	test_check("TIMER2 starts near the programmed zero value", result(0x0314) <= 1);
	for (size_t address = TIMER2_SAMPLES_BASE; address <= TIMER2_WRAP_SAMPLES_LAST; address++) {
		char name[72];

		tfp_sprintf(name, "TIMER2 sample %u remains within programmed MAX", (uint32_t) (address - TIMER2_SAMPLES_BASE));
		test_check(name, result(address) <= 4);
	}
	test_check("TIMER2 produces the sequence 0 -> 1 -> 2 -> 3 -> MAX -> 0", timer2_has_wrap_sequence());
	test_check("TIMER2 continues with a positive count after wrap", result(0x0340) > 0 && result(0x0340) < 4);
	test_eq_u32("active TIMER2 ignores CNT write", 0, result(TIMER2_ACTIVE_WRITE_SAMPLES_BASE) & 0x7000);
	test_eq_u32("active TIMER2 accepts MAX write", 0x7001, result(0x033B));
	test_check("active TIMER2 uses the new MAX without stopping",
		(result(0x033C) & TEAK_TMR2_CTRL_DT2ACT) != 0 && result(TIMER2_SAMPLES_LAST) > 4);
	test_check("TIMER2 compare enters the INT1 handler", result(0x032D) >= 1);
	test_check("TIMER2 handler identifies the compare source", (result(0x032E) & TEAK_INT_FINT1_TMR2) != 0);
	test_eq_u32("stopped TIMER2 retains its count", result(0x032F), result(0x0330));
	test_eq_u32("TIMER2 0xFFFF wrap does not set a false compare flag", 0,
		result(0x0332) & TEAK_INT_FINT1_TMR2);
	test_eq_u32("TIMER2 0xFFFF wrap does not enter the handler", 0, result(0x0333));
	test_check("TIMER2 later MAX match enters the handler", result(0x0335) >= 1);
	test_check("TIMER2 later MAX match is identified", (result(0x0336) & TEAK_INT_FINT1_TMR2) != 0);

	uint16_t timer1_delta = result(0x0339) - result(0x0337);
	uint16_t timer2_delta = result(0x033A) - result(0x0338);
	uint32_t ratio_error = timer2_delta > timer1_delta * 4 ? timer2_delta - timer1_delta * 4 :
		timer1_delta * 4 - timer2_delta;
	printf("# TIMER-RATE,TIMER1=%u,TIMER2=%u,ERROR=%u\n", (uint32_t) timer1_delta,
		(uint32_t) timer2_delta, ratio_error);
	test_check("TIMER2 runs at four times the TIMER1 rate", timer1_delta != 0 && ratio_error <= 8);
}

static bool run_pass(size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		dsp_hw_shared_memory[address] = 0xDEAD;
	if (!test_check("boot commands load timer test program", dsp_hw_load_image(DSP_TIMERS_IMAGE, sizeof(DSP_TIMERS_IMAGE))))
		return false;
	if (!test_check("BRANCH starts timer test program", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("timer test program completes", dsp_hw_wait_shared(RESULT_BASE, RESULT_COMPLETE, 500)))
		return false;

	print_record(pass);
	check_timer1();
	check_timer2();
	if (pass == 1) {
		for (size_t i = 0; i < RESULT_WORDS; i++)
			first_record[i] = result(RESULT_BASE + i);
	} else {
		test_category("Determinism");
		for (size_t i = 0; i < RESULT_WORDS; i++) {
			char name[64];

			tfp_sprintf(name, "result word %u repeats after DSP reset", (uint32_t) i);
			test_eq_u32(name, first_record[i], result(RESULT_BASE + i));
		}
	}

	return true;
}

int main(void) {
	test_start("DSP TIMER1/TIMER2 functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t pass = 1; pass <= 2; pass++) {
		char category[32];

		tfp_sprintf(category, "Independent reset pass %u", (uint32_t) pass);
		test_category(category);
		if (!run_pass(pass))
			break;
	}

	(void) dsp_hw_reset();
	return test_finish();
}
