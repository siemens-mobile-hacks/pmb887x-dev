#include <pmb887x.h>

#ifdef PMB8875
#include "interrupt-window-8875.inc"
#define DSP_INTERRUPT_WINDOW_IMAGE DSP_INTERRUPT_WINDOW_8875
#endif
#ifdef PMB8876
#include "interrupt-window-8876.inc"
#define DSP_INTERRUPT_WINDOW_IMAGE DSP_INTERRUPT_WINDOW_8876
#endif
#include "dsp/dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMPLETE_OFFSET 0x0400
#define RESULT_OFFSET 0x0410
#define ENTRY_ORDER_OFFSET 0x0405

struct interrupt_window {
	const char *name;
	uint16_t enable_end;
	uint16_t disable_address;
	uint16_t entries;
	uint16_t return_address;
};

static const struct interrupt_window WINDOWS[] = {
	{ "eint opens a four-nop window", 0x0601, 0x0605, 1, 0x0601 },
	{ "set st0 IE opens a four-nop window", 0x0622, 0x0626, 1, 0x0624 },
	{ "mov long immediate st0 opens a four-nop window", 0x0642, 0x0646, 1, 0x0643 },
	{ "pop st0 opens a four-nop window", 0x0661, 0x0665, 1, 0x0663 },
	{ "set st0 IM0 opens a four-nop window", 0x0682, 0x0686, 1, 0x0684 },
	{ "set st2 IM2 opens a four-nop window", 0x06A2, 0x06A6, 1, 0x06A4 },
	{ "source raised inside an enabled window", 0x06C4, 0x06C8, 1, 0x0000 },
	{ "eint directly followed by dint", 0x06E1, 0x06E1, 1, 0x06E1 },
	{ "eint nop dint", 0x0701, 0x0702, 1, 0x0701 },
	{ "set st0 IE directly followed by rst", 0x0722, 0x0722, 0, 0x0000 },
	{ "set st0 IE nop rst", 0x0742, 0x0743, 0, 0x0000 },
	{ "mov long immediate st0 directly followed by mov", 0x0762, 0x0762, 0, 0x0000 },
	{ "pop st0 directly followed by dint", 0x0781, 0x0781, 0, 0x0000 },
	{ "mov long immediate st2 opens an IM2 window", 0x07A2, 0x07A6, 1, 0x07A3 },
	{ "pop st2 opens an IM2 window", 0x07C1, 0x07C5, 1, 0x07C3 },
	{ "mov register st0 opens a window", 0x07E1, 0x07E5, 1, 0x07E3 },
	{ "mov register st2 opens an IM2 window", 0x0801, 0x0805, 1, 0x0803 },
	{ "INT0 INT1 and INT2 pending together", 0x0821, 0x0829, 3, 0x0821 },
};

int main(void) {
	test_start("DSP interrupt window test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t offset = COMPLETE_OFFSET; offset < RESULT_OFFSET + 2 * ARRAY_SIZE(WINDOWS); offset++)
		dsp_hw_shared_memory[offset] = 0;
	bool loaded = dsp_hw_load_image(DSP_INTERRUPT_WINDOW_IMAGE, sizeof(DSP_INTERRUPT_WINDOW_IMAGE));
	if (!test_check("boot commands load the interrupt window runner", loaded))
		return test_finish();
	if (!test_check("BRANCH starts the interrupt window runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("runner completes every window", dsp_hw_wait_shared(COMPLETE_OFFSET, COMPLETE_MARKER, 100)))
		return test_finish();

	for (size_t i = 0; i < ARRAY_SIZE(WINDOWS); i++) {
		const struct interrupt_window *window = &WINDOWS[i];
		uint16_t entries = dsp_hw_shared_memory[RESULT_OFFSET + 2 * i];
		uint16_t return_address = dsp_hw_shared_memory[RESULT_OFFSET + 2 * i + 1];
		printf("# DSP_INTERRUPT_WINDOW,%lu,entries=%lu,return=%04lX,window=%04lX-%04lX\n", (uint32_t) i,
			(uint32_t) entries, (uint32_t) return_address, (uint32_t) window->enable_end,
			(uint32_t) window->disable_address);
		test_category(window->name);
		test_eq_u32("handler entries", window->entries, entries);
		if (window->entries == 0)
			continue;
		if (window->return_address != 0) {
			test_eq_u32("handler returns to the instruction after the documented latency", window->return_address,
				return_address);
		} else {
			test_check("handler returns inside the window",
				test_u32_in_interval(return_address, window->enable_end, window->disable_address));
		}
	}

	printf("# DSP_INTERRUPT_WINDOW,order,int0=%lu,int1=%lu,int2=%lu\n",
		(uint32_t) dsp_hw_shared_memory[ENTRY_ORDER_OFFSET], (uint32_t) dsp_hw_shared_memory[ENTRY_ORDER_OFFSET + 1],
		(uint32_t) dsp_hw_shared_memory[ENTRY_ORDER_OFFSET + 2]);
	test_category("INT0 INT1 and INT2 pending together");
	for (size_t line = 0; line < 3; line++)
		test_eq_u32("handler entry order follows INT0, INT1, INT2 priority", line + 1,
			dsp_hw_shared_memory[ENTRY_ORDER_OFFSET + line]);
	return test_finish();
}
