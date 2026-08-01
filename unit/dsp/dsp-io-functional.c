#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "dsp-io-functional-8876.inc"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define COMMAND_OFFSET 0x0010
#define COMMAND_VALUE_OFFSET 0x0011
#define WRITE_RESULT_OFFSET 0x0020
#define READ_RESULT_OFFSET 0x0021
#define IRQ_SOURCE_OFFSET 0x0022
#define IRQ_COUNT_OFFSET 0x0023
#define IRQ_PENDING_OFFSET 0x0024
#define COMMAND_WRITE_DSPOUT 1
#define COMMAND_READ_STATUS 2
#define COMMAND_FINISH UINT16_MAX

static bool run_command(uint16_t command) {
	dsp_hw_shared_memory[COMMAND_OFFSET] = command;
	return dsp_hw_wait_shared(COMMAND_OFFSET, 0, UINT32_MAX);
}

static bool write_dspout(uint16_t value) {
	dsp_hw_shared_memory[COMMAND_VALUE_OFFSET] = value;
	return run_command(COMMAND_WRITE_DSPOUT);
}

static void configure_dspout0(uint32_t output_mode, uint32_t pull) {
	GPIO_PIN(GPIO_DSPOUT0) = GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN | output_mode | pull;
	test_spin(1000);
}

static void configure_dspin0(uint32_t pull) {
	GPIO_PIN(GPIO_DSPIN0) = GPIO_IS_ALT0 | GPIO_PS_ALT | GPIO_DIR_IN | pull;
	test_spin(1000);
}

static void test_dspout0(uint16_t initial_outputs) {
	uint32_t released_level;
	uint16_t preserved_outputs = initial_outputs & ~TEAK_DSP_DSPOUT_DSPOUT0;

	test_category("DSPOUT0 push-pull and open-drain pad behavior");
	GPIO_PIN(GPIO_DSPOUT0) = GPIO_PS_MANUAL | GPIO_DIR_IN | GPIO_PDPU_PULLUP | GPIO_ENAQ_TRISTATE;
	test_spin(1000);
	released_level = GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA;
	printf("# DSPOUT0 tristate level with internal pull-up: %u\n", released_level != 0);

	configure_dspout0(GPIO_PPEN_PUSHPULL, GPIO_PDPU_NONE);
	if (!test_check("DSP writes DSPOUT0 low in push-pull mode", write_dspout(preserved_outputs)))
		return;
	test_eq_u32("DSP readback reports DSPOUT0 low", 0,
		dsp_hw_shared_memory[WRITE_RESULT_OFFSET] & TEAK_DSP_DSPOUT_DSPOUT0);
	test_eq_u32("GPIO input sampler observes the push-pull low level", 0, GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA);

	if (!test_check("DSP writes DSPOUT0 high in push-pull mode",
		write_dspout(preserved_outputs | TEAK_DSP_DSPOUT_DSPOUT0)))
	{
		return;
	}
	test_eq_u32("DSP readback reports DSPOUT0 high", TEAK_DSP_DSPOUT_DSPOUT0,
		dsp_hw_shared_memory[WRITE_RESULT_OFFSET] & TEAK_DSP_DSPOUT_DSPOUT0);
	test_eq_u32("GPIO input sampler observes the push-pull high level", GPIO_DATA,
		GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA);

	configure_dspout0(GPIO_PPEN_OPENDRAIN, GPIO_PDPU_PULLUP);
	if (!test_check("DSP drives DSPOUT0 low in open-drain mode", write_dspout(preserved_outputs)))
		return;
	test_eq_u32("open-drain zero actively pulls the pad low", 0, GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA);
	if (!test_check("DSP releases DSPOUT0 in open-drain mode",
		write_dspout(preserved_outputs | TEAK_DSP_DSPOUT_DSPOUT0)))
	{
		return;
	}
	test_eq_u32("released open-drain output matches the tristated pad level", released_level,
		GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA);

	configure_dspout0(GPIO_PPEN_OPENDRAIN, GPIO_PDPU_PULLDOWN);
	test_eq_u32("the internal pull-down lowers the same released open-drain pad", 0,
		GPIO_PIN(GPIO_DSPOUT0) & GPIO_DATA);
}

static void test_dspin0(void) {
	test_category("DSPIN0 pad latch and rising/falling INT1 delivery");
	if (!test_check("DSP reads the pulled-down DSPIN0 pad", run_command(COMMAND_READ_STATUS)))
		return;
	test_eq_u32("GPIO input sampler observes the DSPIN0 pull-down", 0, GPIO_PIN(GPIO_DSPIN0) & GPIO_DATA);
	test_eq_u32("DSPIN0 latch reads low with the internal pull-down", 0,
		dsp_hw_shared_memory[READ_RESULT_OFFSET] & TEAK_DSP_DSPOUT_DSPIN0);

	configure_dspin0(GPIO_PDPU_PULLUP);
	if (!test_check("DSPIN0 rising edge enters the real INT1 handler",
		dsp_hw_wait_shared(IRQ_COUNT_OFFSET, 1, UINT32_MAX)))
	{
		return;
	}
	test_eq_u32("INT1 identifies the DSPIN0 rising edge", TEAK_INT_FINT1_DSPIN0HI,
		dsp_hw_shared_memory[IRQ_SOURCE_OFFSET]);
	if (!test_check("DSP reads DSPIN0 after the rising edge", run_command(COMMAND_READ_STATUS)))
		return;
	test_eq_u32("GPIO input sampler observes the DSPIN0 pull-up", GPIO_DATA,
		GPIO_PIN(GPIO_DSPIN0) & GPIO_DATA);
	test_eq_u32("DSPIN0 latch reads high with the internal pull-up", TEAK_DSP_DSPOUT_DSPIN0,
		dsp_hw_shared_memory[READ_RESULT_OFFSET] & TEAK_DSP_DSPOUT_DSPIN0);
	test_eq_u32("the rising-edge handler acknowledges DSPIN0HI", 0,
		dsp_hw_shared_memory[IRQ_PENDING_OFFSET] & TEAK_INT_FINT1_DSPIN0HI);

	configure_dspin0(GPIO_PDPU_PULLDOWN);
	if (!test_check("DSPIN0 falling edge enters the real INT1 handler",
		dsp_hw_wait_shared(IRQ_COUNT_OFFSET, 2, UINT32_MAX)))
	{
		return;
	}
	test_eq_u32("INT1 identifies the DSPIN0 falling edge", TEAK_INT_FINT1_DSPIN0LO,
		dsp_hw_shared_memory[IRQ_SOURCE_OFFSET]);
	if (!test_check("DSP reads DSPIN0 after the falling edge", run_command(COMMAND_READ_STATUS)))
		return;
	test_eq_u32("DSPIN0 latch returns low after the falling edge", 0,
		dsp_hw_shared_memory[READ_RESULT_OFFSET] & TEAK_DSP_DSPOUT_DSPIN0);
	test_eq_u32("the falling-edge handler acknowledges DSPIN0LO", 0,
		dsp_hw_shared_memory[IRQ_PENDING_OFFSET] & TEAK_INT_FINT1_DSPIN0LO);
	test_eq_u32("exactly two DSPIN0 edges enter INT1", 2, dsp_hw_shared_memory[IRQ_COUNT_OFFSET]);
}

int main(void) {
	test_start("DSP pad I/O functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_is_qemu()) {
		GPIO_CLC = 1 << MOD_CLC_RMC_SHIFT;
		configure_dspin0(GPIO_PDPU_PULLDOWN);
	}

	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load the DSP pad runner",
		dsp_hw_load_image(DSP_IO_FUNCTIONAL_8876, sizeof(DSP_IO_FUNCTIONAL_8876))))
	{
		return test_finish();
	}
	if (!test_check("BRANCH starts the DSP pad runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("DSP pad runner accepts its first command", run_command(COMMAND_READ_STATUS)))
		return test_finish();

	uint16_t initial_outputs = dsp_hw_shared_memory[READ_RESULT_OFFSET] &
		(TEAK_DSP_DSPOUT_DSPOUT0 | TEAK_DSP_DSPOUT_DSPOUT1 | TEAK_DSP_DSPOUT_DSPOUT2);
	if (test_is_qemu()) {
		test_skip("DSPOUT0 physical pad modes", "requires an accurate GPIO pad model");
		test_skip("DSPIN0 pad levels and edge interrupts", "requires an accurate GPIO pad model");
	} else {
		test_dspout0(initial_outputs);
		test_dspin0();
	}

	(void) write_dspout(initial_outputs);
	dsp_hw_shared_memory[COMMAND_OFFSET] = COMMAND_FINISH;
	if (!test_check("DSP pad runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, UINT32_MAX)))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
