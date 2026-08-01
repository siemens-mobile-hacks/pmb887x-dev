#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A

#if I2S_INSTANCE == 1
#define TEST_TITLE "DSP I2S1 functional test"
#define TEST_MODULE "I2S1"
#define TEST_CTRL TEAK_I2S1_CTRL
#define TEST_CSEL TEAK_I2S1_CSEL
#define TEST_RWADDR TEAK_I2S1_RWADDR
#define TEST_NUM TEAK_I2S1_NUM0
#define TEST_DEN TEAK_I2S1_DEN0
#define TEST_RXCONF TEAK_I2S1_RXCONF
#define TEST_RXINTADDR TEAK_I2S1_RXINTADDR
#define TEST_TXCONF TEAK_I2S1_TXCONF
#define TEST_TXINTADDR TEAK_I2S1_TXINTADDR
#define TEST_RAM_BASE TEAK_I2S1_RAM_BASE
#define TEST_TX_IRQ TEAK_INT_FINTB0_I2S1TX
#define TEST_RX_IRQ TEAK_INT_FINTB0_I2S1RX
#elif I2S_INSTANCE == 2
#define TEST_TITLE "DSP I2S2 functional test"
#define TEST_MODULE "I2S2"
#define TEST_CTRL TEAK_I2S2_CTRL
#define TEST_CSEL TEAK_I2S2_CSEL
#define TEST_RWADDR TEAK_I2S2_RWADDR
#define TEST_NUM TEAK_I2S2_NUM0
#define TEST_DEN TEAK_I2S2_DEN0
#define TEST_RXCONF TEAK_I2S2_RXCONF
#define TEST_RXINTADDR TEAK_I2S2_RXINTADDR
#define TEST_TXCONF TEAK_I2S2_TXCONF
#define TEST_TXINTADDR TEAK_I2S2_TXINTADDR
#define TEST_RAM_BASE TEAK_I2S2_RAM_BASE
#define TEST_TX_IRQ TEAK_INT_FINTB0_I2S2TX
#define TEST_RX_IRQ TEAK_INT_FINTB0_I2S2RX
#elif I2S_INSTANCE == 3
#define TEST_TITLE "DSP I2S3 functional test"
#define TEST_MODULE "I2S3"
#define TEST_CTRL TEAK_I2S3_CTRL
#define TEST_CSEL TEAK_I2S3_CSEL
#define TEST_RWADDR TEAK_I2S3_RADDR
#define TEST_NUM TEAK_I2S3_NUM
#define TEST_DEN TEAK_I2S3_DEN
#define TEST_TXCONF TEAK_I2S3_TXCONF
#define TEST_TXINTADDR TEAK_I2S3_TXINTADDR
#define TEST_RAM_BASE TEAK_I2S3_RAM_BASE
#define TEST_TX_IRQ TEAK_INT_FINTB0_I2S3TX
#else
#error Unsupported I2S instance
#endif

#ifdef PMB8875
#include "i2s-functional-8875.inc"
#define DSP_I2S_IMAGE DSP_I2S_FUNCTIONAL_8875
#else
#include "i2s-functional-8876.inc"
#define DSP_I2S_IMAGE DSP_I2S_FUNCTIONAL_8876
#endif

#if I2S_INSTANCE != 3 && !defined(PMB8875)
#include "i2s-rx-functional-8876.inc"
#include "i2s-rx-pcm-functional-8876.inc"
#endif

static void load_parameters(void) {
	dsp_hw_shared_memory[0x0000] = 0;
	dsp_hw_shared_memory[0x0010] = TEST_CTRL;
	dsp_hw_shared_memory[0x0011] = TEST_CSEL;
	dsp_hw_shared_memory[0x0012] = TEST_RWADDR;
	dsp_hw_shared_memory[0x0013] = TEST_NUM;
	dsp_hw_shared_memory[0x0014] = TEST_DEN;
	dsp_hw_shared_memory[0x0015] = TEST_TXCONF;
	dsp_hw_shared_memory[0x0016] = TEST_TXINTADDR;
	dsp_hw_shared_memory[0x0017] = TEST_RAM_BASE;
	dsp_hw_shared_memory[0x0018] = TEST_TX_IRQ;
}

static void print_record(void) {
	printf("# %s TX reset=%04X first=%04X stopped=%04X second=%04X wrap=%04X reset2=%04X "
		"irqs=%04X seen=%04X pending=%04X ctrl=%04X\n",
		TEST_MODULE, (uint32_t) dsp_hw_shared_memory[0x0020], (uint32_t) dsp_hw_shared_memory[0x0021],
		(uint32_t) dsp_hw_shared_memory[0x0022], (uint32_t) dsp_hw_shared_memory[0x0023],
		(uint32_t) dsp_hw_shared_memory[0x0024], (uint32_t) dsp_hw_shared_memory[0x0025],
		(uint32_t) dsp_hw_shared_memory[0x002E], (uint32_t) dsp_hw_shared_memory[0x0027],
		(uint32_t) dsp_hw_shared_memory[0x0028], (uint32_t) dsp_hw_shared_memory[0x0029]);
	printf("# %s PCM pointer=%04X ctrl=%04X irqs=%04X stopped=%04X\n", TEST_MODULE,
		(uint32_t) dsp_hw_shared_memory[0x002A], (uint32_t) dsp_hw_shared_memory[0x002B],
		(uint32_t) dsp_hw_shared_memory[0x002C], (uint32_t) dsp_hw_shared_memory[0x002D]);
}

static void validate_record(void) {
	uint16_t reset_pointer = dsp_hw_shared_memory[0x0020] & 0x003F;
	uint16_t first_pointer = dsp_hw_shared_memory[0x0021] & 0x003F;
	uint16_t stopped_pointer = dsp_hw_shared_memory[0x0022] & 0x003F;
	uint16_t second_pointer = dsp_hw_shared_memory[0x0023] & 0x003F;
	uint16_t wrapped_pointer = dsp_hw_shared_memory[0x0024] & 0x003F;
	uint16_t reset_pointer_again = dsp_hw_shared_memory[0x0025] & 0x003F;
	uint16_t observed_source = dsp_hw_shared_memory[0x0027] & TEST_TX_IRQ;
	uint16_t pending_source = dsp_hw_shared_memory[0x0028] & TEST_TX_IRQ;
	uint16_t final_control = dsp_hw_shared_memory[0x0029] & 0x0003;
	uint16_t pcm_pointer = dsp_hw_shared_memory[0x002A] & 0x003F;
	uint16_t pcm_control = dsp_hw_shared_memory[0x002B] & 0x0023;
	uint16_t stopped_pcm_pointer = dsp_hw_shared_memory[0x002D] & 0x003F;

	test_eq_u32("I2SON resets the transmit pointer", 0, reset_pointer);
	test_eq_u32("the first programmed interrupt stops at position 4", 4, first_pointer);
	test_eq_u32("the stopped transmit pointer remains at position 4", 4, stopped_pointer);
	test_eq_u32("restart continues to the second programmed position", 8, second_pointer);
	test_eq_u32("the third transfer wraps and stops at position 2", 2, wrapped_pointer);
	test_eq_u32("three programmed positions enter the real INT0 handler", 3, dsp_hw_shared_memory[0x002E]);
	test_eq_u32("the handler observes the selected I2S TX source", TEST_TX_IRQ, observed_source);
	test_eq_u32("the handler acknowledges the final I2S TX source", 0, pending_source);
	test_eq_u32("the handler stops transmission while keeping the module enabled", 1, final_control);
	test_eq_u32("toggling I2SON resets the transmit pointer again", 0, reset_pointer_again);
	test_eq_u32("PCM transmission stops at the programmed position", 6, pcm_pointer);
	test_eq_u32("PCM automatic stop clears TXSTART and preserves TXPCM", 0x21, pcm_control);
	test_eq_u32("PCM automatic stop enters the real INT0 handler once", 1, dsp_hw_shared_memory[0x002C]);
	test_eq_u32("the PCM pointer remains stable after automatic stop", 6, stopped_pcm_pointer);
}

#if I2S_INSTANCE != 3 && !defined(PMB8875)
static void load_receive_parameters(void) {
	dsp_hw_shared_memory[0x0000] = 0;
	dsp_hw_shared_memory[0x0010] = TEST_CTRL;
	dsp_hw_shared_memory[0x0011] = TEST_CSEL;
	dsp_hw_shared_memory[0x0012] = TEST_RWADDR;
	dsp_hw_shared_memory[0x0013] = TEST_NUM;
	dsp_hw_shared_memory[0x0014] = TEST_DEN;
	dsp_hw_shared_memory[0x0015] = TEST_RXCONF;
	dsp_hw_shared_memory[0x0016] = TEST_RXINTADDR;
	dsp_hw_shared_memory[0x0017] = TEST_TXCONF;
	dsp_hw_shared_memory[0x0018] = TEST_TXINTADDR;
	dsp_hw_shared_memory[0x0019] = TEST_RAM_BASE;
	dsp_hw_shared_memory[0x001A] = TEST_RX_IRQ;
}

static uint16_t receive_pointer(uint16_t value) {
	return value >> 8 & 0x003F;
}

static uint16_t transmit_pointer(uint16_t value) {
	return value & 0x003F;
}

static void print_receive_record(void) {
	printf("# %s RX reset=%04X first=%04X stopped=%04X second=%04X wrap=%04X reset2=%04X "
		"irqs=%04X flags=%04X pending=%04X ctrl=%04X\n",
		TEST_MODULE, (uint32_t) dsp_hw_shared_memory[0x0020], (uint32_t) dsp_hw_shared_memory[0x0021],
		(uint32_t) dsp_hw_shared_memory[0x0022], (uint32_t) dsp_hw_shared_memory[0x0023],
		(uint32_t) dsp_hw_shared_memory[0x0024], (uint32_t) dsp_hw_shared_memory[0x0025],
		(uint32_t) dsp_hw_shared_memory[0x002E], (uint32_t) dsp_hw_shared_memory[0x002F],
		(uint32_t) dsp_hw_shared_memory[0x0028], (uint32_t) dsp_hw_shared_memory[0x0029]);
}

static void validate_receive_record(void) {
	uint16_t reset = dsp_hw_shared_memory[0x0020];
	uint16_t first = dsp_hw_shared_memory[0x0021];
	uint16_t stopped = dsp_hw_shared_memory[0x0022];
	uint16_t second = dsp_hw_shared_memory[0x0023];
	uint16_t wrapped = dsp_hw_shared_memory[0x0024];
	uint16_t reset_again = dsp_hw_shared_memory[0x0025];
	uint16_t normal_source = dsp_hw_shared_memory[0x002F] & TEST_RX_IRQ;

	test_eq_u32("I2SON resets both receive and transmit pointers", 0, reset & 0x3F3F);
	test_eq_u32("synchronous normal-mode receive reaches position 4", 4, receive_pointer(first));
	test_eq_u32("the companion zero-filled transmitter reaches position 4", 4, transmit_pointer(first));
	test_eq_u32("the stopped receive pointer remains at position 4", 4, receive_pointer(stopped));
	test_eq_u32("the stopped companion transmit pointer remains at position 4", 4, transmit_pointer(stopped));
	test_eq_u32("receive restart continues to position 8", 8, receive_pointer(second));
	test_eq_u32("receive wraps and stops at position 2", 2, receive_pointer(wrapped));
	test_eq_u32("three receive positions enter the real INT0 handler", 3, dsp_hw_shared_memory[0x002E]);
	test_eq_u32("normal-mode handler observes the RX source", TEST_RX_IRQ, normal_source);
	test_eq_u32("normal-mode handler stops both directions and keeps I2S enabled", 1,
		dsp_hw_shared_memory[0x0029] & 0x0007);
	test_eq_u32("toggling I2SON resets both pointers again", 0, reset_again & 0x3F3F);
	test_eq_u32("the handler acknowledges the I2S RX source", 0, dsp_hw_shared_memory[0x0028] & TEST_RX_IRQ);
}

static void print_receive_pcm_record(void) {
	printf("# %s RX PCM reset=%04X pointer=%04X ctrl=%04X irqs=%04X stopped=%04X flags=%04X pending=%04X\n",
		TEST_MODULE, (uint32_t) dsp_hw_shared_memory[0x0025], (uint32_t) dsp_hw_shared_memory[0x002A],
		(uint32_t) dsp_hw_shared_memory[0x002B], (uint32_t) dsp_hw_shared_memory[0x002C],
		(uint32_t) dsp_hw_shared_memory[0x002D], (uint32_t) dsp_hw_shared_memory[0x0027],
		(uint32_t) dsp_hw_shared_memory[0x0028]);
}

static void validate_receive_pcm_record(void) {
	uint16_t pcm = dsp_hw_shared_memory[0x002A];
	uint16_t pcm_stopped = dsp_hw_shared_memory[0x002D];
	uint16_t pcm_source = dsp_hw_shared_memory[0x0027] & TEST_RX_IRQ;

	test_eq_u32("I2SON resets both PCM pointers", 0, dsp_hw_shared_memory[0x0025] & 0x3F3F);
	test_eq_u32("PCM receive stops at position 6", 6, receive_pointer(pcm));
	test_eq_u32("PCM companion transmission stops at position 6", 6, transmit_pointer(pcm));
	test_eq_u32("PCM automatic stop clears both START bits and preserves both PCM modes", 0x0061,
		dsp_hw_shared_memory[0x002B] & 0x0067);
	test_eq_u32("PCM receive enters the real INT0 handler once", 1, dsp_hw_shared_memory[0x002C]);
	test_eq_u32("PCM handler observes the RX source", TEST_RX_IRQ, pcm_source);
	test_eq_u32("PCM receive pointer remains stable after automatic stop", 6, receive_pointer(pcm_stopped));
	test_eq_u32("PCM transmit pointer remains stable after automatic stop", 6, transmit_pointer(pcm_stopped));
	test_eq_u32("the PCM handler acknowledges the I2S RX source", 0,
		dsp_hw_shared_memory[0x0028] & TEST_RX_IRQ);
}

static bool run_receive_test(void) {
	test_category("I2S receive ring / pointer, wrap, stop modes, and IRQ");
	if (!test_check("Mask ROM boot dispatcher becomes ready for RX", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load I2S RX runner",
		dsp_hw_load_image(DSP_I2S_RX_FUNCTIONAL_8876, sizeof(DSP_I2S_RX_FUNCTIONAL_8876))))
	{
		return false;
	}
	load_receive_parameters();
	if (!test_check("BRANCH starts I2S RX runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("I2S RX runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, 1000)))
		return false;

	print_receive_record();
	validate_receive_record();
	return true;
}

static bool run_receive_pcm_test(void) {
	test_category("I2S PCM receive auto-stop, pointer stability, and IRQ");
	if (!test_check("Mask ROM boot dispatcher becomes ready for PCM RX", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load I2S PCM RX runner",
		dsp_hw_load_image(DSP_I2S_RX_PCM_FUNCTIONAL_8876, sizeof(DSP_I2S_RX_PCM_FUNCTIONAL_8876))))
	{
		return false;
	}
	load_receive_parameters();
	if (!test_check("BRANCH starts I2S PCM RX runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("I2S PCM RX runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, 1000)))
		return false;

	print_receive_pcm_record();
	validate_receive_pcm_record();
	return true;
}
#endif

int main(void) {
	test_start(TEST_TITLE);
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("I2S transmit ring / pointer, wrap, stop modes, and IRQ");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load I2S runner", dsp_hw_load_image(DSP_I2S_IMAGE, sizeof(DSP_I2S_IMAGE))))
		return test_finish();
	load_parameters();
	if (!test_check("BRANCH starts I2S runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("I2S runner completes", dsp_hw_wait_shared(0, COMPLETE_MARKER, 1000)))
		return test_finish();

	print_record();
	validate_record();

#if I2S_INSTANCE != 3 && !defined(PMB8875)
	(void) run_receive_test();
	(void) run_receive_pcm_test();
#endif

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
