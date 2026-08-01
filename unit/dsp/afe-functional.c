#include <pmb887x.h>
#include <gen/dsp.h>

#include "dsp-hw.h"
#include "test.h"

#define COMPLETE_MARKER 0xA55A
#define TX_SNAPSHOT_OFFSET 0x0100
#define AFE_RING_WORDS 64

#ifdef PMB8875
#include "afe-functional-8875.inc"
#define DSP_AFE_IMAGE DSP_AFE_FUNCTIONAL_8875
#else
#include "afe-functional-8876.inc"
#define DSP_AFE_IMAGE DSP_AFE_FUNCTIONAL_8876
#endif

static const uint16_t power_down_tx_vector[AFE_RING_WORDS] = {
	0x85EA, 0x85F3, 0xB12F, 0x8000, 0x9048, 0x8A3B, 0x81C2, 0x8BCF,
	0x8000, 0x8B30, 0x8290, 0x87AF, 0x8556, 0x85B5, 0x86BA, 0x84BF,
	0x875B, 0x8465, 0x8773, 0x848D, 0x870E, 0x8524, 0x8659, 0x85DF,
	0x85AE, 0x8669, 0x854D, 0x869E, 0x8544, 0x8682, 0x857A, 0x863A,
	0x85C9, 0x85ED, 0x860D, 0x85B8, 0x8632, 0x85A6, 0x8633, 0x85B3,
	0x861B, 0x85D1, 0x85FA, 0x85F1, 0x85DE, 0x8606, 0x85D0, 0x860D,
	0x85D1, 0x8606, 0x85DC, 0x85F9, 0x85EA, 0x85EC, 0x85F5, 0x85E3,
	0x85FB, 0x85E1, 0x85FA, 0x85E4, 0x85F5, 0x85EA, 0x85EF, 0x85F0,
};

static uint32_t hash_words(size_t offset, size_t words) {
	uint32_t hash = UINT32_C(2166136261);

	for (size_t i = 0; i < words; i++) {
		hash ^= dsp_hw_shared_memory[offset + i] & 0xFF;
		hash *= UINT32_C(16777619);
		hash ^= dsp_hw_shared_memory[offset + i] >> 8;
		hash *= UINT32_C(16777619);
	}
	return hash;
}

static bool tx_ring_matches_vector(void) {
	for (size_t i = 0; i < AFE_RING_WORDS; i++) {
		if (dsp_hw_shared_memory[TX_SNAPSHOT_OFFSET + i] != power_down_tx_vector[i])
			return false;
	}
	return true;
}

static void print_record(void) {
	printf("# AFE RX reset=%04X first=%04X wrap=%04X restarted=%04X irqs=%04X seen=%04X pending=%04X bcon=%04X\n",
		(uint32_t) dsp_hw_shared_memory[0x0020], (uint32_t) dsp_hw_shared_memory[0x0021],
		(uint32_t) dsp_hw_shared_memory[0x0022], (uint32_t) dsp_hw_shared_memory[0x0023],
		(uint32_t) dsp_hw_shared_memory[0x0024], (uint32_t) dsp_hw_shared_memory[0x0025],
		(uint32_t) dsp_hw_shared_memory[0x0026], (uint32_t) dsp_hw_shared_memory[0x0027]);
	printf("# AFE TX reset=%04X first=%04X wrap=%04X restarted=%04X irqs=%04X seen=%04X pending=%04X bcon=%04X\n",
		(uint32_t) dsp_hw_shared_memory[0x0028], (uint32_t) dsp_hw_shared_memory[0x0029],
		(uint32_t) dsp_hw_shared_memory[0x002A], (uint32_t) dsp_hw_shared_memory[0x002B],
		(uint32_t) dsp_hw_shared_memory[0x002C], (uint32_t) dsp_hw_shared_memory[0x002D],
		(uint32_t) dsp_hw_shared_memory[0x002E], (uint32_t) dsp_hw_shared_memory[0x002F]);
	printf("# AFE power-down TX vector hash=%08X first=%04X last=%04X\n",
		hash_words(TX_SNAPSHOT_OFFSET, AFE_RING_WORDS),
		(uint32_t) dsp_hw_shared_memory[TX_SNAPSHOT_OFFSET],
		(uint32_t) dsp_hw_shared_memory[TX_SNAPSHOT_OFFSET + AFE_RING_WORDS - 1]);
}

static void validate_receive_path(void) {
	test_eq_u32("receive-ring read pointer starts at zero after DSP reset", 0,
		dsp_hw_shared_memory[0x0020] & TEAK_AFE_RWADDR_RDADDR);
	test_eq_u32("receive path reaches the first programmed position", 4,
		dsp_hw_shared_memory[0x0021] & TEAK_AFE_RWADDR_RDADDR);
	test_eq_u32("receive path wraps and reaches the second programmed position", 2,
		dsp_hw_shared_memory[0x0022] & TEAK_AFE_RWADDR_RDADDR);
	test_eq_u32("clearing RXSTART leaves AFE enabled with the receive path stopped", TEAK_AFE_BCON_MODE,
		dsp_hw_shared_memory[0x0027] & (TEAK_AFE_BCON_MODE | TEAK_AFE_BCON_RXSTART));
	test_eq_u32("starting the transmit path completes the receive-ring pointer reset", 0,
		dsp_hw_shared_memory[0x0023] & TEAK_AFE_RWADDR_RDADDR);
	test_eq_u32("both receive-ring positions enter the real INT0 handler", 2, dsp_hw_shared_memory[0x0024]);
	test_eq_u32("receive-ring consumption raises VBRX", TEAK_INT_FINTB0_VBRX,
		dsp_hw_shared_memory[0x0025] & TEAK_INT_FINTB0_VBRX);
	test_eq_u32("the receive-ring handler acknowledges VBRX", 0,
		dsp_hw_shared_memory[0x0026] & TEAK_INT_FINTB0_VBRX);
}

static void validate_transmit_path(void) {
	test_eq_u32("transmit-ring write pointer starts at zero after DSP reset", 0,
		dsp_hw_shared_memory[0x0028] & TEAK_AFE_RWADDR_WRADDR);
	test_eq_u32("transmit path reaches the first programmed position", 0x0400,
		dsp_hw_shared_memory[0x0029] & TEAK_AFE_RWADDR_WRADDR);
	test_eq_u32("transmit path wraps and reaches the second programmed position", 0x0200,
		dsp_hw_shared_memory[0x002A] & TEAK_AFE_RWADDR_WRADDR);
	test_eq_u32("clearing TXSTART leaves AFE enabled with the transmit path stopped", TEAK_AFE_BCON_MODE,
		dsp_hw_shared_memory[0x002F] & (TEAK_AFE_BCON_MODE | TEAK_AFE_BCON_TXSTART));
	test_eq_u32("starting the receive path completes the transmit-ring pointer reset", 0,
		dsp_hw_shared_memory[0x002B] & TEAK_AFE_RWADDR_WRADDR);
	test_eq_u32("both transmit-ring positions enter the real INT0 handler", 2, dsp_hw_shared_memory[0x002C]);
	test_eq_u32("transmit-ring production raises VBTX", TEAK_INT_FINTB0_VBTX,
		dsp_hw_shared_memory[0x002D] & TEAK_INT_FINTB0_VBTX);
	test_eq_u32("the transmit-ring handler acknowledges VBTX", 0,
		dsp_hw_shared_memory[0x002E] & TEAK_INT_FINTB0_VBTX);
}

static void validate_ring_data(void) {
	test_check("power-down decimation produces the PMB8876 TX-ring vector", tx_ring_matches_vector());
}

int main(void) {
	test_start("DSP AFE functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_category("AFE digital rings, power-down data vector, pointer wrap/reset, and IRQ");
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return test_finish();
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load AFE runner", dsp_hw_load_image(DSP_AFE_IMAGE, sizeof(DSP_AFE_IMAGE))))
		return test_finish();
	if (!test_check("BRANCH starts AFE runner", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return test_finish();
	bool completed = dsp_hw_wait_shared(0, COMPLETE_MARKER, 500);
	print_record();
	if (!test_check("AFE runner completes", completed))
		return test_finish();

	validate_receive_path();
	validate_transmit_path();
	validate_ring_data();

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
