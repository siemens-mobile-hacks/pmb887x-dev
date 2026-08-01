#include <pmb887x.h>
#include <stopwatch.h>

#include "dsp-hw.h"
#include "test.h"

#define RESULT_BASE 0x0300
#define RESULT_LAST 0x030C
#define RESULT_WORDS (RESULT_LAST - RESULT_BASE + 1)
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define SEMAPHORE BIT(5)

#ifdef PMB8875
#include "mcs-functional-8875.inc"
#define DSP_MCS_IMAGE DSP_MCS_IMAGE_8875
#else
#include "mcs-functional-8876.inc"
#define DSP_MCS_IMAGE DSP_MCS_IMAGE_8876
#endif

static uint16_t first_record[RESULT_WORDS];

static bool wait_for_flags(uint32_t mask, uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_COM_STATUS & mask) != expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (DSP_COM_STATUS & mask) == expected;
}

static bool wait_for_semaphore(uint32_t expected) {
	stopwatch_t start = stopwatch_get();

	while ((DSP_SEM_STATUS & SEMAPHORE) != expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return (DSP_SEM_STATUS & SEMAPHORE) == expected;
}

static void print_record(size_t pass) {
	printf("# DSPMCS,%u", (uint32_t) pass);
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		printf(",%04X", (uint32_t) dsp_hw_shared_memory[address]);
	printf("\n");
}

static bool test_communication_flags(void) {
	test_category("MCS / Bidirectional communication flags");
	dsp_hw_shared_memory[0x0301] = 0x1357;
	DSP_COM_SET = BIT(1);
	if (!test_check("MCU request makes DSP set response flag", wait_for_flags(BIT(1) | BIT(2), BIT(2))))
		return false;
	test_eq_u32("DSP consumes MCU request flag", 0, DSP_COM_STATUS & BIT(1));
	test_eq_u32("DSP transforms MCU request payload", 0x1358, dsp_hw_shared_memory[0x0302]);
	DSP_COM_CLEAR = BIT(2);

	if (!test_check("DSP publishes inverse-direction request", wait_for_flags(BIT(3), BIT(3))))
		return false;
	test_eq_u32("DSP request payload is visible to MCU", 0x2468, dsp_hw_shared_memory[0x0304]);
	dsp_hw_shared_memory[0x0305] = 0x369C;
	DSP_COM_CLEAR = BIT(3);
	DSP_COM_SET = BIT(4);
	if (!test_check("DSP consumes MCU inverse response", dsp_hw_wait_shared(0x0303, 0x1111, 100)))
		return false;
	test_eq_u32("DSP transforms inverse response payload", 0x369D, dsp_hw_shared_memory[0x0302]);
	test_eq_u32("DSP clears inverse response flag", 0, DSP_COM_STATUS & BIT(4));

	return true;
}

static bool test_semaphores(void) {
	test_category("MCS / Semaphore ownership transfer");
	dsp_hw_shared_memory[0x0306] = 0;
	DSP_SEM_SET = SEMAPHORE;
	if (!test_check("MCU acquires free semaphore 5", wait_for_semaphore(0)))
		return false;
	DSP_COM_SET = BIT(5);
	stopwatch_usleep_wd(100);
	test_eq_u32("DSP sees semaphore busy while MCU owns it", UINT16_MAX, dsp_hw_shared_memory[0x0307]);
	test_eq_u32("DSP cannot update protected counter while MCU owns semaphore", 0, dsp_hw_shared_memory[0x0306]);
	test_eq_u32("DSP cannot publish completion while MCU owns semaphore", 0, DSP_COM_STATUS & BIT(6));
	DSP_SEM_CLEAR = SEMAPHORE;
	if (!test_check("MCU release transfers semaphore to waiting DSP", wait_for_flags(BIT(6), BIT(6))))
		return false;
	test_eq_u32("DSP reads ownership after MCU release", 0, dsp_hw_shared_memory[0x030C] & SEMAPHORE);
	test_eq_u32("DSP updates protected counter after ownership transfer", 1, dsp_hw_shared_memory[0x0306]);
	DSP_COM_CLEAR = BIT(6);
	if (!test_check("DSP release returns semaphore 5 to free state", wait_for_semaphore(SEMAPHORE)))
		return false;

	DSP_COM_SET = BIT(7);
	if (!test_check("DSP acquires free semaphore 5", dsp_hw_wait_shared(0x0308, 0xA1A1, 100)))
		return false;
	test_eq_u32("DSP reads zero after acquiring semaphore 5", 0, dsp_hw_shared_memory[0x0309] & SEMAPHORE);
	DSP_SEM_SET = SEMAPHORE;
	test_eq_u32("MCU sees semaphore busy while DSP owns it", SEMAPHORE, DSP_SEM_STATUS & SEMAPHORE);
	test_eq_u32("MCU cannot update protected counter while DSP owns semaphore", 1, dsp_hw_shared_memory[0x0306]);
	dsp_hw_shared_memory[0x030B] = 1;
	if (!test_check("DSP release transfers semaphore to waiting MCU", wait_for_semaphore(0)))
		return false;
	if (!test_check("DSP releases semaphore after protected update", wait_for_flags(BIT(8), BIT(8))))
		return false;
	test_eq_u32("DSP performs exactly one protected update", 2, dsp_hw_shared_memory[0x0306]);
	dsp_hw_shared_memory[0x0306]++;
	test_eq_u32("MCU performs one update after ownership transfer", 3, dsp_hw_shared_memory[0x0306]);
	DSP_SEM_CLEAR = SEMAPHORE;
	DSP_COM_CLEAR = BIT(8);
	test_check("DSP reports semaphore scenario completion", dsp_hw_wait_shared(0x030A, COMPLETE_MARKER, 100));

	return true;
}

static bool run_pass(size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		dsp_hw_shared_memory[address] = 0xDEAD;
	if (!test_check("boot commands load MCS test program", dsp_hw_load_image(DSP_MCS_IMAGE, sizeof(DSP_MCS_IMAGE))))
		return false;
	if (!test_check("BRANCH starts MCS test program", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("MCS test program becomes ready", dsp_hw_wait_shared(0x0300, READY_MARKER, 100)))
		return false;
	if (!test_communication_flags() || !test_semaphores())
		return false;

	print_record(pass);
	if (pass == 1) {
		for (size_t i = 0; i < RESULT_WORDS; i++)
			first_record[i] = dsp_hw_shared_memory[RESULT_BASE + i];
	} else {
		test_category("MCS / Determinism");
		for (size_t i = 0; i < RESULT_WORDS; i++) {
			char name[64];

			tfp_sprintf(name, "result word %u repeats after DSP reset", (uint32_t) i);
			test_eq_u32(name, first_record[i], dsp_hw_shared_memory[RESULT_BASE + i]);
		}
	}

	return true;
}

int main(void) {
	test_start("DSP MCS functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t pass = 1; pass <= 2; pass++) {
		char category[32];

		tfp_sprintf(category, "Independent reset pass %u", (uint32_t) pass);
		test_category(category);
		if (!run_pass(pass))
			break;
	}

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
