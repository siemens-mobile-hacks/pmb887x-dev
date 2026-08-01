#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A

#ifdef PMB8875
#include "modulator-functional-8875.inc"
#define DSP_MODULATOR_IMAGE DSP_MODULATOR_FUNCTIONAL_8875
#else
#include "modulator-functional-8876.inc"
#define DSP_MODULATOR_IMAGE DSP_MODULATOR_FUNCTIONAL_8876
#endif

static void print_record(void) {
	printf("# MOD status reset=%04X irq1=%04X irq2=%04X active=%04X stopped=%04X ctrl=%04X "
		"seen=%04X pending=%04X ram0=%04X ram511=%04X\n",
		(uint32_t) dsp_hw_shared_memory[0x0020], (uint32_t) dsp_hw_shared_memory[0x0021],
		(uint32_t) dsp_hw_shared_memory[0x0022], (uint32_t) dsp_hw_shared_memory[0x0023],
		(uint32_t) dsp_hw_shared_memory[0x0024], (uint32_t) dsp_hw_shared_memory[0x0025],
		(uint32_t) dsp_hw_shared_memory[0x0026], (uint32_t) dsp_hw_shared_memory[0x002A],
		(uint32_t) dsp_hw_shared_memory[0x0028], (uint32_t) dsp_hw_shared_memory[0x0029]);
}

static void validate_record(void) {
	test_eq_u32("modulator starts inactive", 0, dsp_hw_shared_memory[0x0020] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("MSWACT makes the digital modulator active", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0023] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("interrupt address 4 is reached while the modulator is active", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0021] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("interrupt address 2 is reached after the 512-word ring wraps", TEAK_MOD_STAT_MSTAT,
		dsp_hw_shared_memory[0x0022] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("clearing MSWACT stops the digital modulator", 0, dsp_hw_shared_memory[0x0024] & TEAK_MOD_STAT_MSTAT);
	test_eq_u32("MSWACT remains clear after the stop", 0, dsp_hw_shared_memory[0x0025] & TEAK_MOD_CTRL_MSWACT);
	test_eq_u32("both programmed positions enter the real INT0 handler", 2, dsp_hw_shared_memory[0x0027]);
	test_eq_u32("modulator consumption raises MODU", TEAK_INT_FINTA0_MODU,
		dsp_hw_shared_memory[0x0026] & TEAK_INT_FINTA0_MODU);
	test_eq_u32("the handler acknowledges MODU", 0, dsp_hw_shared_memory[0x002A] & TEAK_INT_FINTA0_MODU);
	test_eq_u32("modulator RAM entry 0 remains intact after consumption", 1, dsp_hw_shared_memory[0x0028]);
	test_eq_u32("modulator RAM entry 511 remains intact after wrap", 1, dsp_hw_shared_memory[0x0029]);
}

int main(void) {
	test_start("DSP modulator functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("digital modulator RAM consumption, wrap, status, and IRQ");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load modulator runner",
		dsp_hw_load_image(DSP_MODULATOR_IMAGE, sizeof(DSP_MODULATOR_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts modulator runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(0, COMPLETE_MARKER, 500);
	print_record();
	if (!test_check("modulator runner completes", completed))
		return test_finish();

	validate_record();

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
