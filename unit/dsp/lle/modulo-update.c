#include <pmb887x.h>

#include "dsp/dsp-hw.h"
#include "test.h"

#ifdef PMB8875
#include "modulo-update-8875.inc"
#define DSP_MODULO_UPDATE_IMAGE DSP_MODULO_UPDATE_8875
#else
#include "modulo-update-8876.inc"
#define DSP_MODULO_UPDATE_IMAGE DSP_MODULO_UPDATE_8876
#endif

#define DSP_MODULO_UPDATE_COMPLETE_MARKER 0x600D
#define DSP_MODULO_UPDATE_COMPLETE_OFFSET 0x0400
#define DSP_MODULO_UPDATE_SET_OFFSET 0x0401
#define DSP_MODULO_UPDATE_ST2_CLEAR_OFFSET 0x0402
#define DSP_MODULO_UPDATE_ST2_SET_OFFSET 0x0403
#define DSP_MODULO_UPDATE_CFGJ_OFFSET 0x0404
#define DSP_MODULO_UPDATE_CFGI_OFFSET 0x0405
#define DSP_MODULO_UPDATE_RING_START 0x0100
#define DSP_MODULO_UPDATE_RING_NEXT 0x0104

int main(void) {
	test_start("DSP modulo update test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = DSP_MODULO_UPDATE_COMPLETE_OFFSET; offset <= DSP_MODULO_UPDATE_CFGI_OFFSET; offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_MODULO_UPDATE_IMAGE, sizeof(DSP_MODULO_UPDATE_IMAGE));
	if (!test_check("boot commands load the modulo update runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the modulo update runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(DSP_MODULO_UPDATE_COMPLETE_OFFSET, DSP_MODULO_UPDATE_COMPLETE_MARKER, 100);
	printf("# DSP_MODULO_UPDATE,set=%04lX,st2_clear=%04lX,st2_set=%04lX,cfgj=%04lX,cfgi=%04lX\n",
		(uint32_t) dsp_hw_shared_memory[DSP_MODULO_UPDATE_SET_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_MODULO_UPDATE_ST2_CLEAR_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_MODULO_UPDATE_ST2_SET_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_MODULO_UPDATE_CFGJ_OFFSET],
		(uint32_t) dsp_hw_shared_memory[DSP_MODULO_UPDATE_CFGI_OFFSET]);
	if (!test_check("the runner completes", completed))
		return test_finish();

	test_eq_u32("SET ST2 enables R5 modulo wrapping", DSP_MODULO_UPDATE_RING_START,
		dsp_hw_shared_memory[DSP_MODULO_UPDATE_SET_OFFSET]);
	test_eq_u32("MOV #0 to ST2 disables R5 modulo wrapping", DSP_MODULO_UPDATE_RING_NEXT,
		dsp_hw_shared_memory[DSP_MODULO_UPDATE_ST2_CLEAR_OFFSET]);
	test_eq_u32("MOV #imm to ST2 enables R5 modulo wrapping", DSP_MODULO_UPDATE_RING_START,
		dsp_hw_shared_memory[DSP_MODULO_UPDATE_ST2_SET_OFFSET]);
	test_eq_u32("MOV #imm to CFGJ changes the R5 modulo size", DSP_MODULO_UPDATE_RING_NEXT,
		dsp_hw_shared_memory[DSP_MODULO_UPDATE_CFGJ_OFFSET]);
	test_eq_u32("MOV #imm to CFGI changes the R0 modulo size", DSP_MODULO_UPDATE_RING_NEXT,
		dsp_hw_shared_memory[DSP_MODULO_UPDATE_CFGI_OFFSET]);

	return test_finish();
}
