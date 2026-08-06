#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "interrupt-sources-1-8876.inc"
#include "interrupt-sources-2-8876.inc"
#include "interrupt-sources-a0-8876.inc"
#include "interrupt-sources-b0-8876.inc"
#include "test.h"

#define READY_OFFSET 0x0500
#define REQUEST_OFFSET 0x0501
#define DONE_OFFSET 0x0502
#define MASK_OFFSET 0x0503
#define PENDING_BEFORE_ENABLE_OFFSET 0x0504
#define ENTRIES_BEFORE_ENABLE_OFFSET 0x0505
#define HANDLER_FLAGS_OFFSET 0x0506
#define FLAGS_AFTER_ACK_OFFSET 0x0507
#define HANDLER_COUNT_OFFSET 0x0508
#define READY_MARKER 0xA55A
#define TEST_TIMEOUT_MS 100

struct interrupt_source {
	const char *name;
	uint16_t mask;
};

struct interrupt_bank {
	const char *name;
	const struct interrupt_source *sources;
	size_t source_count;
	const uint8_t *image;
	size_t image_size;
};

static const struct interrupt_source A0_SOURCES[] = {
	{ "MCU0", TEAK_INT_FINTA0_MCU0 },
	{ "MCU1", TEAK_INT_FINTA0_MCU1 },
	{ "MCU2", TEAK_INT_FINTA0_MCU2 },
	{ "MCU3", TEAK_INT_FINTA0_MCU3 },
	{ "FRAME", TEAK_INT_FINTA0_FRAME },
	{ "CODONHI", TEAK_INT_FINTA0_CODONHI },
	{ "CODONLO", TEAK_INT_FINTA0_CODONLO },
	{ "MODU", TEAK_INT_FINTA0_MODU },
	{ "CHADEC", TEAK_INT_FINTA0_CHADEC },
	{ "EQ", TEAK_INT_FINTA0_EQ },
	{ "BBHI", TEAK_INT_FINTA0_BBHI },
	{ "BBLO", TEAK_INT_FINTA0_BBLO },
	{ "BB_FULL", TEAK_INT_FINTA0_BB_FULL },
};

static const struct interrupt_source B0_SOURCES[] = {
	{ "I2S1TX", TEAK_INT_FINTB0_I2S1TX },
	{ "I2S1RX", TEAK_INT_FINTB0_I2S1RX },
	{ "I2S2TX", TEAK_INT_FINTB0_I2S2TX },
	{ "I2S2RX", TEAK_INT_FINTB0_I2S2RX },
	{ "VBRX", TEAK_INT_FINTB0_VBRX },
	{ "VBTX", TEAK_INT_FINTB0_VBTX },
	{ "SSC1RX", TEAK_INT_FINTB0_SSC1RX },
	{ "SSC1TX", TEAK_INT_FINTB0_SSC1TX },
	{ "SSC1ERR", TEAK_INT_FINTB0_SSC1ERR },
	{ "SYSMCU", TEAK_INT_FINTB0_SYSMCU },
	{ "I2S3TX", TEAK_INT_FINTB0_I2S3TX },
};

static const struct interrupt_source INT1_SOURCES[] = {
	{ "CIPH", TEAK_INT_FINT1_CIPH },
	{ "TMR10", TEAK_INT_FINT1_TMR10 },
	{ "TMR11", TEAK_INT_FINT1_TMR11 },
	{ "TMR2", TEAK_INT_FINT1_TMR2 },
	{ "DSPIN0HI", TEAK_INT_FINT1_DSPIN0HI },
	{ "DSPIN0LO", TEAK_INT_FINT1_DSPIN0LO },
	{ "DSPIN1HI", TEAK_INT_FINT1_DSPIN1HI },
	{ "DSPIN1LO", TEAK_INT_FINT1_DSPIN1LO },
	{ "MONIN1HI", TEAK_INT_FINT1_MONIN1HI },
	{ "MONIN1LO", TEAK_INT_FINT1_MONIN1LO },
	{ "MONIN2HI", TEAK_INT_FINT1_MONIN2HI },
	{ "MONIN2LO", TEAK_INT_FINT1_MONIN2LO },
	{ "MONIN3HI", TEAK_INT_FINT1_MONIN3HI },
	{ "MONIN3LO", TEAK_INT_FINT1_MONIN3LO },
	{ "MONIN4HI", TEAK_INT_FINT1_MONIN4HI },
	{ "MONIN4LO", TEAK_INT_FINT1_MONIN4LO },
};

static const struct interrupt_source INT2_SOURCES[] = {
	{ "FW0", TEAK_INT_FINT2_FW0 },
	{ "FW1", TEAK_INT_FINT2_FW1 },
	{ "FW2", TEAK_INT_FINT2_FW2 },
	{ "FW3", TEAK_INT_FINT2_FW3 },
	{ "FW4", TEAK_INT_FINT2_FW4 },
	{ "FW5", TEAK_INT_FINT2_FW5 },
	{ "FW6", TEAK_INT_FINT2_FW6 },
	{ "FW7", TEAK_INT_FINT2_FW7 },
	{ "FW8", TEAK_INT_FINT2_FW8 },
	{ "FW9", TEAK_INT_FINT2_FW9 },
	{ "FW10", TEAK_INT_FINT2_FW10 },
	{ "FW11", TEAK_INT_FINT2_FW11 },
	{ "FW12", TEAK_INT_FINT2_FW12 },
	{ "FW13", TEAK_INT_FINT2_FW13 },
	{ "FW14", TEAK_INT_FINT2_FW14 },
	{ "FW15", TEAK_INT_FINT2_FW15 },
};

static const struct interrupt_bank BANKS[] = {
	{ "A0 / INT0", A0_SOURCES, ARRAY_SIZE(A0_SOURCES), DSP_INTERRUPT_SOURCES_A0_8876,
		sizeof(DSP_INTERRUPT_SOURCES_A0_8876) },
	{ "B0 / INT0", B0_SOURCES, ARRAY_SIZE(B0_SOURCES), DSP_INTERRUPT_SOURCES_B0_8876,
		sizeof(DSP_INTERRUPT_SOURCES_B0_8876) },
	{ "INT1", INT1_SOURCES, ARRAY_SIZE(INT1_SOURCES), DSP_INTERRUPT_SOURCES_1_8876,
		sizeof(DSP_INTERRUPT_SOURCES_1_8876) },
	{ "INT2", INT2_SOURCES, ARRAY_SIZE(INT2_SOURCES), DSP_INTERRUPT_SOURCES_2_8876,
		sizeof(DSP_INTERRUPT_SOURCES_2_8876) },
};

static uint16_t request_sequence;

static void run_mask(const char *name, uint16_t mask) {
	char check[96];

	dsp_hw_shared_memory[MASK_OFFSET] = mask;
	dsp_hw_shared_memory[REQUEST_OFFSET] = ++request_sequence;
	tfp_sprintf(check, "%s request completes", name);
	if (!test_check(check, dsp_hw_wait_shared(DONE_OFFSET, request_sequence, TEST_TIMEOUT_MS)))
		return;

	tfp_sprintf(check, "%s latches while globally disabled", name);
	test_eq_u32(check, mask, dsp_hw_shared_memory[PENDING_BEFORE_ENABLE_OFFSET]);
	tfp_sprintf(check, "%s does not enter its handler while disabled", name);
	test_eq_u32(check, 0, dsp_hw_shared_memory[ENTRIES_BEFORE_ENABLE_OFFSET]);
	tfp_sprintf(check, "%s enters the bank handler exactly once", name);
	test_eq_u32(check, 1, dsp_hw_shared_memory[HANDLER_COUNT_OFFSET]);
	tfp_sprintf(check, "%s is visible in the handler", name);
	test_eq_u32(check, mask, dsp_hw_shared_memory[HANDLER_FLAGS_OFFSET]);
	tfp_sprintf(check, "%s is cleared by acknowledgement", name);
	test_eq_u32(check, 0, dsp_hw_shared_memory[FLAGS_AFTER_ACK_OFFSET]);
}

static void run_bank(const struct interrupt_bank *bank) {
	test_category(bank->name);
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return;

	DSP_COM_CLEAR = 0xFFFF;
	if (!test_check("boot commands load the interrupt-source runner",
		dsp_hw_load_image(bank->image, bank->image_size))) {
		return;
	}
	if (!test_check("BRANCH starts the interrupt-source runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return;
	if (!test_check("interrupt-source runner becomes ready",
		dsp_hw_wait_shared(READY_OFFSET, READY_MARKER, TEST_TIMEOUT_MS))) {
		return;
	}

	request_sequence = 0;
	uint16_t all_sources = 0;
	for (size_t i = 0; i < bank->source_count; i++) {
		run_mask(bank->sources[i].name, bank->sources[i].mask);
		all_sources |= bank->sources[i].mask;
	}
	run_mask("all documented sources simultaneously", all_sources);
}

int main(void) {
	test_start("DSP interrupt-unit sources");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	for (size_t i = 0; i < ARRAY_SIZE(BANKS); i++)
		run_bank(&BANKS[i]);

	DSP_COM_CLEAR = 0xFFFF;
	(void) dsp_hw_reset();
	return test_finish();
}
