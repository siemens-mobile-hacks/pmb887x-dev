#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp/dsp-hw.h"
#include "tpu/tpu-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define INTERRUPT_COUNT_OFFSET 0x0010
#define CODON_READY_OFFSET 0x0011
#define RUNNER_TIMEOUT_MS 100

#ifdef PMB8875
#include "modulator-functional-8875.inc"
#define DSP_MODULATOR_IMAGE DSP_MODULATOR_FUNCTIONAL_8875
#define MOD_CTRL_RESET 0x0000
#define MOD_CTRL_WRITTEN_WITHOUT_MSWACT 0x0001
#define MSWACT_MODU 0
#else
#include "modulator-functional-8876.inc"
#define DSP_MODULATOR_IMAGE DSP_MODULATOR_FUNCTIONAL_8876
#define MOD_CTRL_RESET 0x0600
#define MOD_CTRL_WRITTEN_WITHOUT_MSWACT 0x061F
#define MSWACT_MODU TEAK_INT_FINTA0_MODU
#endif

static void print_record(void) {
	printf("# MOD ctrl reset=%04lX written=%04lX, MSWACT status=%04lX finta0=%04lX stopped=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[0x0030], (uint32_t) dsp_hw_shared_memory[0x0031],
		(uint32_t) dsp_hw_shared_memory[0x0032], (uint32_t) dsp_hw_shared_memory[0x0033],
		(uint32_t) dsp_hw_shared_memory[0x0034]);
	printf("# MOD status reset=%04lX irq1=%04lX irq2=%04lX active=%04lX stopped=%04lX "
		"seen=%04lX pending=%04lX ram0=%04lX ram511=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[0x0020], (uint32_t) dsp_hw_shared_memory[0x0021],
		(uint32_t) dsp_hw_shared_memory[0x0022], (uint32_t) dsp_hw_shared_memory[0x0023],
		(uint32_t) dsp_hw_shared_memory[0x0024], (uint32_t) dsp_hw_shared_memory[0x0026],
		(uint32_t) dsp_hw_shared_memory[0x002A], (uint32_t) dsp_hw_shared_memory[0x0028],
		(uint32_t) dsp_hw_shared_memory[0x0029]);
}

static void validate_control(void) {
	test_category("MOD_CTRL and MSWACT");
	test_eq_u32("MOD_CTRL reset value", MOD_CTRL_RESET, dsp_hw_shared_memory[0x0030]);
	test_eq_u32("MOD_CTRL keeps only its implemented bits", MOD_CTRL_WRITTEN_WITHOUT_MSWACT,
		dsp_hw_shared_memory[0x0031]);
	test_eq_u32("modulator starts inactive", 0, dsp_hw_shared_memory[0x0020] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("MSWACT makes the digital modulator active", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0032] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("MSWACT without CODON raises MODU only on E101", MSWACT_MODU,
		dsp_hw_shared_memory[0x0033] & TEAK_INT_FINTA0_MODU);
	test_eq_u32("clearing MSWACT stops the digital modulator", 0, dsp_hw_shared_memory[0x0034] & TEAK_MOD_STAT_MSTAT);
}

static void validate_codon(void) {
	test_category("CODON RAM consumption, wrap, status, and IRQ");
	test_eq_u32("CODON makes the digital modulator active", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0023] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("interrupt address 4 is reached while the modulator is active", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0021] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("interrupt address 2 is reached after the 512-word ring wraps", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0022] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("clearing CODON stops the digital modulator", 0, dsp_hw_shared_memory[0x0024] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("both programmed positions enter the real INT0 handler", 2, dsp_hw_shared_memory[0x0027]);
	test_eq_u32("modulator consumption raises MODU", TEAK_INT_FINTA0_MODU,
		dsp_hw_shared_memory[0x0026] & TEAK_INT_FINTA0_MODU);
	test_eq_u32("the handler acknowledges MODU", 0, dsp_hw_shared_memory[0x002A] & TEAK_INT_FINTA0_MODU);
	test_eq_u32("modulator RAM entry 0 remains intact after consumption", 1, dsp_hw_shared_memory[0x0028]);
	test_eq_u32("modulator RAM entry 511 remains intact after wrap", 1, dsp_hw_shared_memory[0x0029]);
}

static bool run_codon(void) {
	if (!test_check("runner waits for CODON", dsp_hw_wait_shared(CODON_READY_OFFSET, 1, RUNNER_TIMEOUT_MS)))
		return false;
	if (!test_check("TPU decoder 17 raises CODON", tpu_hw_run_decoder(TPU_HW_DECODER_CODON_SET)))
		return false;
	bool interrupted = dsp_hw_wait_shared(INTERRUPT_COUNT_OFFSET, 2, RUNNER_TIMEOUT_MS);
	if (!test_check("TPU decoder 18 clears CODON", tpu_hw_run_decoder(TPU_HW_DECODER_CODON_CLEAR)))
		return false;
	return test_check("both MODU positions are reached while CODON is high", interrupted);
}

int main(void) {
	test_start("DSP modulator functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	TPU_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("modulator runner");
	if (!test_check("TPU decoder 18 clears CODON before the run", tpu_hw_run_decoder(TPU_HW_DECODER_CODON_CLEAR)))
		return test_finish();
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load modulator runner",
		dsp_hw_load_image(DSP_MODULATOR_IMAGE, sizeof(DSP_MODULATOR_IMAGE))))
		return test_finish();
	dsp_hw_shared_memory[CODON_READY_OFFSET] = 0;
	if (!test_check("BRANCH starts modulator runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!run_codon())
		return test_finish();
	bool completed = dsp_hw_wait_shared(0, COMPLETE_MARKER, RUNNER_TIMEOUT_MS);
	print_record();
	if (!test_check("modulator runner completes", completed))
		return test_finish();

	validate_control();
	validate_codon();

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
