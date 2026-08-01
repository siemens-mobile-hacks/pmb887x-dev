#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define RESULT_BASE 0x0400
#define RESULT_LAST 0x0434
#define RESULT_WORDS (RESULT_LAST - RESULT_BASE + 1)
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A

#ifdef PMB8875
#include "interrupt-functional-8875.inc"
#define DSP_INTERRUPT_IMAGE DSP_INTERRUPT_IMAGE_8875
#else
#include "interrupt-functional-8876.inc"
#define DSP_INTERRUPT_IMAGE DSP_INTERRUPT_IMAGE_8876
#endif

static uint16_t first_record[RESULT_WORDS];

static void print_record(size_t pass) {
	printf("# DSPINT,%u", (uint32_t) pass);
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		printf(",%04X", (uint32_t) dsp_hw_shared_memory[address]);
	printf("\n");
}

static void validate_disabled_pending(void) {
	test_category("Interrupt controller / Disabled pending sources");
	test_eq_u32("A0 source flag latches while disabled", 1, dsp_hw_shared_memory[0x0411] & 1);
	test_eq_u32("disabled A0 source does not enter INT0 handler", 0, dsp_hw_shared_memory[0x0412]);
	test_eq_u32("enabling pending A0 source enters INT0 handler", 1, dsp_hw_shared_memory[0x0413]);
	test_eq_u32("B0 source flag latches while disabled", BIT(7), dsp_hw_shared_memory[0x0414] & BIT(7));
	test_eq_u32("disabled B0 source does not enter INT0 handler", 1, dsp_hw_shared_memory[0x0415]);
	test_eq_u32("enabling pending B0 source enters INT0 handler", 2, dsp_hw_shared_memory[0x0416]);
	test_eq_u32("INT1 source flag latches while disabled", 1, dsp_hw_shared_memory[0x0417] & 1);
	test_eq_u32("disabled INT1 source does not enter handler", 0, dsp_hw_shared_memory[0x0418]);
	test_eq_u32("enabling pending INT1 source enters handler", 1, dsp_hw_shared_memory[0x0419]);
	test_eq_u32("INT2 source flag latches while disabled", 1, dsp_hw_shared_memory[0x041A] & 1);
	test_eq_u32("disabled INT2 source does not enter handler", 0, dsp_hw_shared_memory[0x041B]);
	test_eq_u32("enabling pending INT2 source enters handler", 1, dsp_hw_shared_memory[0x041C]);
}

static void validate_independent_acknowledgement(void) {
	test_category("Interrupt controller / Independent acknowledgement");
	test_eq_u32("simultaneous A0 source is pending on first INT0 entry", 1, dsp_hw_shared_memory[0x041D] & 1);
	test_eq_u32("simultaneous B0 source is pending on first INT0 entry", BIT(7),
		dsp_hw_shared_memory[0x041E] & BIT(7));
	test_eq_u32("acknowledging A0 clears its source", 0, dsp_hw_shared_memory[0x041F] & 1);
	test_eq_u32("acknowledging A0 leaves B0 pending", BIT(7), dsp_hw_shared_memory[0x0420] & BIT(7));
	test_eq_u32("remaining B0 source causes a second INT0 entry", 4, dsp_hw_shared_memory[0x0421]);
}

static void validate_priority_and_repeat(void) {
	test_category("Interrupt controller / Simultaneous line priority");
	test_eq_u32("INT0 source is pending before global enable", 1, dsp_hw_shared_memory[0x0432] & 1);
	test_eq_u32("INT1 source is pending before global enable", 1, dsp_hw_shared_memory[0x0433] & 1);
	test_eq_u32("INT2 source is pending before global enable", 1, dsp_hw_shared_memory[0x0434] & 1);
	test_eq_u32("INT0 wins simultaneous INT0/INT1/INT2 arbitration", 0, dsp_hw_shared_memory[0x0422]);
	test_eq_u32("INT1 is serviced after INT0", 1, dsp_hw_shared_memory[0x0423]);
	test_eq_u32("INT2 is serviced after INT1", 2, dsp_hw_shared_memory[0x0424]);

	test_category("Interrupt controller / Repeated delivery after acknowledgement");
	test_eq_u32("first INT2 event enters handler once", 1, dsp_hw_shared_memory[0x0426]);
	test_eq_u32("first acknowledgement clears INT2 source", 0, dsp_hw_shared_memory[0x0428] & BIT(1));
	test_eq_u32("same INT2 source enters handler a second time", 2, dsp_hw_shared_memory[0x0427]);
	test_eq_u32("second acknowledgement clears INT2 source", 0, dsp_hw_shared_memory[0x0429] & BIT(1));
}

static void validate_cleanup(void) {
	test_category("Interrupt controller / Cleanup");
	test_eq_u32("test reaches final phase", 7, dsp_hw_shared_memory[0x0402]);
	test_eq_u32("A0 flags are clear", 0, dsp_hw_shared_memory[0x042A]);
	test_eq_u32("B0 flags are clear", 0, dsp_hw_shared_memory[0x042B]);
	test_eq_u32("INT1 flags are clear", 0, dsp_hw_shared_memory[0x042C]);
	test_eq_u32("INT2 flags are clear", 0, dsp_hw_shared_memory[0x042D]);
	test_eq_u32("A0 enables are clear", 0, dsp_hw_shared_memory[0x042E]);
	test_eq_u32("B0 enables are clear", 0, dsp_hw_shared_memory[0x042F]);
	test_eq_u32("INT1 enables are clear", 0, dsp_hw_shared_memory[0x0430]);
	test_eq_u32("INT2 enables are clear", 0, dsp_hw_shared_memory[0x0431]);
}

static bool run_pass(size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		dsp_hw_shared_memory[address] = 0xDEAD;
	if (!test_check("boot commands load interrupt-controller test", dsp_hw_load_image(DSP_INTERRUPT_IMAGE,
		sizeof(DSP_INTERRUPT_IMAGE))))
		return false;
	if (!test_check("BRANCH starts interrupt-controller test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("interrupt-controller test becomes ready", dsp_hw_wait_shared(0x0400, READY_MARKER, 100)))
		return false;
	if (!test_check("interrupt-controller scenarios complete", dsp_hw_wait_shared(0x0401, COMPLETE_MARKER, 500)))
		return false;

	validate_disabled_pending();
	validate_independent_acknowledgement();
	validate_priority_and_repeat();
	validate_cleanup();
	print_record(pass);
	if (pass == 1) {
		for (size_t i = 0; i < RESULT_WORDS; i++)
			first_record[i] = dsp_hw_shared_memory[RESULT_BASE + i];
	} else {
		test_category("Interrupt controller / Determinism");
		for (size_t i = 0; i < RESULT_WORDS; i++) {
			char name[64];

			tfp_sprintf(name, "result word %u repeats after DSP reset", (uint32_t) i);
			test_eq_u32(name, first_record[i], dsp_hw_shared_memory[RESULT_BASE + i]);
		}
	}

	return true;
}

int main(void) {
	test_start("DSP interrupt controller functional test");
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
