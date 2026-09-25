#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

int main(void) {
	test_start("CGU DSP clock test");

	DSP_CLC = (1 << MOD_CLC_RMC_SHIFT);
	CGU_OSC |= CGU_OSC_PHASE1_POWER_UP | CGU_OSC_PHASE1_BYPASS_N;
	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_PHASE1;

	uint16_t timer_max;
	bool active = dsp_hw_reset() && dsp_hw_read_reg(TEAK_TMR2_MAX, &timer_max);
	bool stopped = false;
	if (active) {
		CGU_CON2 = (CGU_CON2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_DISABLE;
		stopped = !dsp_hw_read_reg(TEAK_TMR2_MAX, &timer_max);
	}

	CGU_CON2 = (CGU_CON2 & ~CGU_CON2_DSP_CLKSEL) | CGU_CON2_DSP_CLKSEL_PHASE1;
	bool resumed = dsp_hw_reset() && dsp_hw_read_reg(TEAK_TMR2_MAX, &timer_max);

	test_check("DSP BootROM responds with phase 1 clock", active);
	test_check("DSP BootROM stops when its clock is disabled", stopped);
	test_check("DSP BootROM resumes when its clock returns", resumed);
	return test_finish();
}
