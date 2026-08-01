#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define RESULT_BASE 0x0500
#define RESULT_LAST 0x0628
#define RESULT_WORDS (RESULT_LAST - RESULT_BASE + 1)
#define DIRECT_RESULT_BASE 0x0510
#define FIFO_RESULT_BASE 0x0600
#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define SSC_RX_IRQ BIT(7)
#define SSC_TX_IRQ BIT(8)
#define SSC_ERROR_IRQ BIT(9)
#define SSC_BUSY BIT(12)
#define SSC_RECEIVE_ERROR BIT(9)
#define SSC_FIFO_ENABLE BIT(0)
#define SSC_FIFO_FLUSH BIT(1)
#define SSC_RX_LEVEL_MASK 0x003F
#define SSC_TX_LEVEL_MASK 0x3F00
#define SSC_TX_LEVEL_SHIFT 8

#ifdef PMB8875
#include "ssc-functional-8875.inc"
#define DSP_SSC_IMAGE DSP_SSC_IMAGE_8875
#else
#include "ssc-functional-8876.inc"
#define DSP_SSC_IMAGE DSP_SSC_IMAGE_8876
#endif

static const uint16_t DIRECT_WORDS[] = {
	0xA55A, 0x3CC3, 0x9695, 0x5AA5, 0xC33C, 0x6996, 0x0F81, 0x81F0,
};
static const uint16_t DIRECT_WIDTHS[] = { 2, 4, 8, 12, 16 };
static const uint16_t FIFO_WORDS[] = {
	0x1001, 0x2112, 0x3223, 0x4334, 0x5445, 0x6556, 0x7667, 0x8778,
	0x9889, 0xA99A, 0xBAAB, 0xCBBC, 0xDCCD, 0xEDDE, 0xFEEF, 0x0FF0,
	0x1357, 0x2468, 0x369C, 0x47AD, 0x58BE, 0x69CF, 0x7AD0, 0x8BE1,
	0x9CF2, 0xAD03, 0xBE14, 0xCF25, 0xD036, 0xE147, 0xF258, 0x0369,
	0x147A, 0x258B, 0x369D, 0x47AE, 0x58BF, 0x69C0, 0x7AD1, 0x8BE2,
	0x9CF3,
};

static uint16_t first_record[RESULT_WORDS];

static void print_record(size_t pass) {
	printf("# DSPSSC,%u", (uint32_t) pass);
	for (size_t address = RESULT_BASE; address <= 0x057F; address++)
		printf(",%04X", (uint32_t) dsp_hw_shared_memory[address]);
	for (size_t address = FIFO_RESULT_BASE; address <= RESULT_LAST; address++)
		printf(",%04X", (uint32_t) dsp_hw_shared_memory[address]);
	printf("\n");
}

static void validate_direct_loopback(void) {
	test_category("SSC / Internal loopback formats");
	for (size_t width_index = 0; width_index < ARRAY_SIZE(DIRECT_WIDTHS); width_index++) {
		uint32_t width = DIRECT_WIDTHS[width_index];
		uint32_t mask = width == 16 ? UINT16_MAX : BIT(width) - 1;

		for (size_t format = 0; format < ARRAY_SIZE(DIRECT_WORDS); format++) {
			char name[72];
			size_t result = DIRECT_RESULT_BASE + width_index * ARRAY_SIZE(DIRECT_WORDS) + format;

			tfp_sprintf(name, "%u-bit format %u loopback data", width, (uint32_t) format);
			test_eq_u32(name, DIRECT_WORDS[format] & mask, dsp_hw_shared_memory[result]);
		}
	}
}

static void validate_progress(void) {
	uint32_t immediate = dsp_hw_shared_memory[0x0540];
	uint32_t progress = dsp_hw_shared_memory[0x0541];
	uint32_t done = dsp_hw_shared_memory[0x0542];

	test_category("SSC / Transfer progress");
	test_eq_u32("immediate read precedes busy-state visibility", 0, immediate & SSC_BUSY);
	test_check("slow transfer remains busy while bits shift", (progress & SSC_BUSY) != 0);
	test_check("bit count advances before transfer completion", (progress & 0xF) != 0);
	test_eq_u32("slow transfer eventually leaves busy state", 0, done & SSC_BUSY);
	test_eq_u32("slow loopback preserves the word", 0x5AA5, dsp_hw_shared_memory[0x0543]);
}

static void validate_interrupts(void) {
	uint32_t transfer_irqs = SSC_RX_IRQ | SSC_TX_IRQ;

	test_category("SSC / Hardware interrupt delivery");
	test_eq_u32("completed transfer raises RX and TX sources", transfer_irqs,
		dsp_hw_shared_memory[0x0550] & transfer_irqs);
	test_eq_u32("pending hardware sources enter INT0 handler", 1, dsp_hw_shared_memory[0x0551]);
	test_eq_u32("INT0 handler observes both SSC sources", transfer_irqs,
		dsp_hw_shared_memory[0x0552] & transfer_irqs);
	test_eq_u32("handler acknowledges RX and TX sources", 0, dsp_hw_shared_memory[0x0553] & transfer_irqs);
}

static void validate_fifo(void) {
	uint32_t after_fill = dsp_hw_shared_memory[0x0560];
	uint32_t full = dsp_hw_shared_memory[0x0561];

	test_category("SSC / FIFO depth, thresholds, and ordering");
	test_check("initial burst queues multiple TX words", ((after_fill & SSC_TX_LEVEL_MASK) >> SSC_TX_LEVEL_SHIFT) > 1);
	test_eq_u32("32-word burst drains TX FIFO", 0, full & SSC_TX_LEVEL_MASK);
	test_eq_u32("32-word burst fills RX FIFO to documented depth", 32, full & SSC_RX_LEVEL_MASK);
	test_eq_u32("FIFO thresholds raise RX and TX sources", SSC_RX_IRQ | SSC_TX_IRQ,
		dsp_hw_shared_memory[0x0562] & (SSC_RX_IRQ | SSC_TX_IRQ));
	test_eq_u32("reading first burst drains RX FIFO", 0, dsp_hw_shared_memory[0x0563] & SSC_RX_LEVEL_MASK);
	test_eq_u32("9-word burst below RX threshold raises only TX source", SSC_TX_IRQ,
		dsp_hw_shared_memory[0x0564] & (SSC_RX_IRQ | SSC_TX_IRQ));
	test_eq_u32("reading second burst drains RX FIFO", 0, dsp_hw_shared_memory[0x0565] & SSC_RX_LEVEL_MASK);
	for (size_t i = 0; i < ARRAY_SIZE(FIFO_WORDS); i++) {
		char name[56];

		tfp_sprintf(name, "FIFO word %u preserves order", (uint32_t) i);
		test_eq_u32(name, FIFO_WORDS[i], dsp_hw_shared_memory[FIFO_RESULT_BASE + i]);
	}
}

static void validate_flush(void) {
	uint32_t tx_before = dsp_hw_shared_memory[0x0566];
	uint32_t rx_before = dsp_hw_shared_memory[0x0569];

	test_category("SSC / FIFO flush and recovery");
	test_check("TX FIFO contains queued data before flush", (tx_before & SSC_TX_LEVEL_MASK) != 0);
	test_eq_u32("TX flush command self-clears", 0, dsp_hw_shared_memory[0x0567] & SSC_FIFO_FLUSH);
	test_eq_u32("TX FIFO remains enabled after flush", SSC_FIFO_ENABLE,
		dsp_hw_shared_memory[0x0567] & SSC_FIFO_ENABLE);
	test_eq_u32("TX flush empties queued words", 0, dsp_hw_shared_memory[0x0568] & SSC_TX_LEVEL_MASK);
	test_check("RX FIFO contains received data before flush", (rx_before & SSC_RX_LEVEL_MASK) != 0);
	test_eq_u32("RX flush command self-clears", 0, dsp_hw_shared_memory[0x056A] & SSC_FIFO_FLUSH);
	test_eq_u32("RX FIFO remains enabled after flush", SSC_FIFO_ENABLE,
		dsp_hw_shared_memory[0x056A] & SSC_FIFO_ENABLE);
	test_eq_u32("RX flush empties received words", 0, dsp_hw_shared_memory[0x056B] & SSC_RX_LEVEL_MASK);
	test_eq_u32("loopback recovers after both flushes", 0x5A3C, dsp_hw_shared_memory[0x056C]);
}

static void validate_error_recovery(void) {
	test_category("SSC / RX underflow error and recovery");
	test_check("empty RX FIFO read sets receive-error status",
		(dsp_hw_shared_memory[0x0570] & SSC_RECEIVE_ERROR) != 0);
	test_eq_u32("receive error raises SSC1ERR source", SSC_ERROR_IRQ,
		dsp_hw_shared_memory[0x0571] & SSC_ERROR_IRQ);
	test_eq_u32("pending error enters INT0 handler", 1, dsp_hw_shared_memory[0x0572]);
	test_eq_u32("INT0 handler observes SSC1ERR", SSC_ERROR_IRQ,
		dsp_hw_shared_memory[0x0573] & SSC_ERROR_IRQ);
	test_eq_u32("WHBCON.CLRRE clears receive-error status", 0,
		dsp_hw_shared_memory[0x0576] & SSC_RECEIVE_ERROR);
	test_eq_u32("WHBCON.CLRRE deasserts acknowledged SSC1ERR", 0,
		dsp_hw_shared_memory[0x0577] & SSC_ERROR_IRQ);
	test_eq_u32("programming-mode recovery clears SSC1ERR", 0,
		dsp_hw_shared_memory[0x0574] & SSC_ERROR_IRQ);
	test_eq_u32("loopback works after receive-error recovery", 0xC35A, dsp_hw_shared_memory[0x0575]);
}

static bool run_pass(size_t pass) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	for (size_t address = RESULT_BASE; address <= RESULT_LAST; address++)
		dsp_hw_shared_memory[address] = 0xDEAD;
	bool loaded = dsp_hw_load_image(DSP_SSC_IMAGE, sizeof(DSP_SSC_IMAGE));
	if (!test_check("boot commands load SSC test", loaded))
		return false;
	if (!test_check("BRANCH starts SSC test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("SSC test becomes ready", dsp_hw_wait_shared(0x0500, READY_MARKER, 100)))
		return false;
	if (!test_check("SSC scenarios complete", dsp_hw_wait_shared(0x0501, COMPLETE_MARKER, 2000)))
		return false;

	validate_direct_loopback();
	validate_progress();
	validate_interrupts();
	validate_fifo();
	validate_flush();
	validate_error_recovery();
	test_eq_u32("test reaches final functional phase", 6, dsp_hw_shared_memory[0x0502]);
	print_record(pass);
	if (pass == 1) {
		for (size_t i = 0; i < RESULT_WORDS; i++)
			first_record[i] = dsp_hw_shared_memory[RESULT_BASE + i];
	} else {
		test_category("SSC / Determinism");
		test_eq_memory("complete result record repeats after DSP reset", first_record,
			dsp_hw_shared_memory + RESULT_BASE, sizeof(first_record));
	}

	return true;
}

int main(void) {
	test_start("DSP SSC functional test");
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
