#include <pmb887x.h>

#include "dsp-hw.h"
#include "test.h"

#define READY_MARKER 0xA55A
#define COMPLETE_MARKER 0xA55A
#define CIPHER_IRQ BIT(0)
#define A53_COMMAND 0x0904
#define A53_PARAMETER_BASE 0x0A00
#define A53_RESULT_BASE 0x0A20

#ifdef PMB8875
#include "cipher-a512-functional-8875.inc"
#include "cipher-a53-functional-8875.inc"
#define DSP_CIPHER_A512_IMAGE DSP_CIPHER_A512_IMAGE_8875
#define DSP_CIPHER_A53_IMAGE DSP_CIPHER_A53_IMAGE_8875
#else
#include "cipher-a512-functional-8876.inc"
#include "cipher-a53-functional-8876.inc"
#define DSP_CIPHER_A512_IMAGE DSP_CIPHER_A512_IMAGE_8876
#define DSP_CIPHER_A53_IMAGE DSP_CIPHER_A53_IMAGE_8876
#endif

static const uint16_t A51_GSM_EXPECTED[] = {
	0x45D3, 0x6EAA, 0xBAE8, 0x38DC, 0xF4DE, 0x1594, 0x6D83, 0x0000,
	0xC09B, 0xF07A, 0x3754, 0x59C8, 0x2BA0, 0xDE15, 0x268D, 0x0001,
};

static const uint16_t A52_GSM_EXPECTED[] = {
	0x39A2, 0xC311, 0xED41, 0xCDFF, 0x4B19, 0x769F, 0x28F0, 0x0001,
	0x5C0F, 0x7B35, 0xDAC7, 0xA67A, 0x5D01, 0x03D5, 0x649A, 0x0002,
};

static const uint16_t A51_EDGE_EXPECTED[] = {
	0x45D3, 0x6EAA, 0xBAE8, 0x38DC, 0xF4DE, 0x1594, 0x6D83, 0x026C,
	0xC1EB, 0xDD53, 0x6720, 0xAE81, 0x7854, 0x9A37, 0xDAF4, 0xFC82,
	0xB4EF, 0x5825, 0x7E90, 0xA0D0, 0xBE5A, 0x0DE1,
	0xB40A, 0x97C9, 0x4C1E, 0xE597, 0x8C33, 0x9A86, 0x7269, 0x5547,
	0x288E, 0x691F, 0xCAE3, 0x8FA2, 0x0EC4, 0xD675, 0x51AF, 0x7726,
	0xC69E, 0xA468, 0xB0F4, 0x8F45, 0x1B49, 0x0C26,
};

// CIPH_KEY command words for the first 3GPP A5/3 vector.
static const uint16_t A53_COMMAND_KEY[] = {
	0xBC00, 0x82C5, 0x459F, 0x2BD6, 0xBC00, 0x82C5, 0x459F, 0x2BD6,
};

static const uint16_t A53_COUNTERS[] = { 0x000F, 0x0010, 0x049E };

static const uint16_t A53_HARDWARE_EXPECTED[] = {
	0xCB91, 0x0D88, 0xEF61, 0xE86A, 0x7B46, 0xAABE, 0x227B, 0x0002,
	0x8B7D, 0xEAB6, 0x3C11, 0x3DA7, 0x8913, 0x01AA, 0x728D, 0x0001,
};

static bool streams_match(size_t base, const uint16_t *expected, size_t words_per_stream, uint16_t tail_mask) {
	for (size_t block = 0; block < 2; block++) {
		for (size_t word = 0; word < words_per_stream; word++) {
			size_t index = block * words_per_stream + word;
			uint16_t mask = word + 1 == words_per_stream ? tail_mask : UINT16_MAX;
			if ((dsp_hw_shared_memory[base + index] & mask) != expected[index])
				return false;
		}
	}
	return true;
}

static bool shared_words_equal(size_t base, const uint16_t *expected, size_t words) {
	for (size_t i = 0; i < words; i++) {
		if (dsp_hw_shared_memory[base + i] != expected[i])
			return false;
	}
	return true;
}

static bool shared_regions_equal(size_t first, size_t second, size_t words) {
	for (size_t i = 0; i < words; i++) {
		if (dsp_hw_shared_memory[first + i] != dsp_hw_shared_memory[second + i])
			return false;
	}
	return true;
}

static bool run_image(const uint8_t *image, size_t image_size, size_t ready, size_t complete) {
	if (!test_check("Mask ROM boot dispatcher becomes ready", dsp_hw_reset()))
		return false;
	DSP_COM_CLEAR = UINT16_MAX;
	if (!test_check("boot commands load cipher test", dsp_hw_load_image(image, image_size)))
		return false;
	if (!test_check("BRANCH starts cipher test", dsp_hw_branch(DSP_HW_STARTUP_ADDRESS)))
		return false;
	if (!test_check("cipher test becomes ready", dsp_hw_wait_shared(ready, READY_MARKER, 100)))
		return false;
	return test_check("cipher scenarios become available", dsp_hw_wait_shared(complete, COMPLETE_MARKER, 3000));
}

static bool run_a53_request(const uint16_t *key, const uint16_t *counters, uint16_t request) {
	for (size_t i = 0; i < ARRAY_SIZE(A53_COMMAND_KEY); i++)
		dsp_hw_shared_memory[A53_PARAMETER_BASE + i] = key[i];
	for (size_t i = 0; i < ARRAY_SIZE(A53_COUNTERS); i++)
		dsp_hw_shared_memory[A53_PARAMETER_BASE + ARRAY_SIZE(A53_COMMAND_KEY) + i] = counters[i];
	dsp_hw_shared_memory[A53_COMMAND] = request;
	return dsp_hw_wait_shared(A53_COMMAND, 0, 100);
}

static void run_a512(void) {
	test_category("Cipher / A5/1 and A5/2");
	if (!run_image(DSP_CIPHER_A512_IMAGE, sizeof(DSP_CIPHER_A512_IMAGE), 0x0700, 0x0701))
		return;

	test_check("A5/1 exposes active state immediately after start", dsp_hw_shared_memory[0x0710] & 1);
	test_eq_u32("A5/1 returns to idle after generation", 0, dsp_hw_shared_memory[0x0711] & 1);
	test_check("masked CIPH completion remains pending", dsp_hw_shared_memory[0x0712] & CIPHER_IRQ);
	test_eq_u32("masked CIPH completion does not enter INT1 handler", 0, dsp_hw_shared_memory[0x0713]);
	test_eq_u32("enabling pending CIPH delivers one INT1", 1, dsp_hw_shared_memory[0x0716]);
	test_eq_u32("RINT1 acknowledgement clears CIPH request", 0, dsp_hw_shared_memory[0x0717] & CIPHER_IRQ);
	test_eq_u32("next cipher operation delivers a new INT1", 2, dsp_hw_shared_memory[0x0718]);
	test_eq_u32("all A5/1 and A5/2 operations deliver completion interrupts", 5, dsp_hw_shared_memory[0x0702]);
	test_eq_u32("INT1 handler observes CIPH source", CIPHER_IRQ, dsp_hw_shared_memory[0x0703] & CIPHER_IRQ);
	test_check("A5/1 matches published GSM streams", streams_match(0x0740, A51_GSM_EXPECTED, 8, 0x0003));
	test_check("identical A5/1 input repeats exactly", shared_regions_equal(0x0740, 0x0760, 16));
	test_check("A5/2 matches published GSM streams", streams_match(0x0780, A52_GSM_EXPECTED, 8, 0x0003));
	test_check("key and frame changes affect A5/1 output", !shared_regions_equal(0x0740, 0x07A0, 16));
	test_check("A5/1 EDGE exposes active state", dsp_hw_shared_memory[0x0714] & 1);
	test_eq_u32("A5/1 EDGE returns to idle", 0, dsp_hw_shared_memory[0x0715] & 1);
	test_check("A5/1 EDGE generates both 348-bit streams", streams_match(0x0800, A51_EDGE_EXPECTED, 22, 0x0FFF));
}

static void run_a53(void) {
	uint16_t baseline[ARRAY_SIZE(A53_HARDWARE_EXPECTED)];
	uint16_t changed_key[ARRAY_SIZE(A53_COMMAND_KEY)];
	uint16_t changed_counters[ARRAY_SIZE(A53_COUNTERS)];

	test_category("Cipher / A5/3");
	if (!run_image(DSP_CIPHER_A53_IMAGE, sizeof(DSP_CIPHER_A53_IMAGE), 0x0900, 0x0901))
		return;

	if (!test_check("A5/3 MASK ROM register sequence completes",
		run_a53_request(A53_COMMAND_KEY, A53_COUNTERS, 1)))
		return;
	test_check("A5/3 hardware vector matches both GSM streams",
		streams_match(A53_RESULT_BASE, A53_HARDWARE_EXPECTED, 8, 0x0003));
	for (size_t i = 0; i < ARRAY_SIZE(baseline); i++)
		baseline[i] = dsp_hw_shared_memory[A53_RESULT_BASE + i];

	test_check("identical A5/3 input completes without reset", run_a53_request(A53_COMMAND_KEY, A53_COUNTERS, 2));
	test_check("identical A5/3 input repeats exactly", shared_words_equal(A53_RESULT_BASE, baseline, ARRAY_SIZE(baseline)));

	for (size_t i = 0; i < ARRAY_SIZE(changed_key); i++)
		changed_key[i] = A53_COMMAND_KEY[i];
	for (size_t i = 0; i < ARRAY_SIZE(changed_counters); i++)
		changed_counters[i] = A53_COUNTERS[i];
	changed_key[0] ^= 1;
	changed_counters[0] ^= 1;
	test_check("changed A5/3 key and frame complete", run_a53_request(changed_key, changed_counters, 3));
	test_check("key and frame changes affect A5/3 output", !shared_words_equal(A53_RESULT_BASE, baseline, ARRAY_SIZE(baseline)));

	test_eq_u32("A5/3 start exposes CACT and INIT", 0x0009, dsp_hw_shared_memory[0x0910] & 0x0009);
	test_eq_u32("A5/3 completion clears CACT and INIT", 0, dsp_hw_shared_memory[0x0911] & 0x0009);
	test_eq_u32("all A5/3 operations deliver completion interrupts", 3, dsp_hw_shared_memory[0x0902]);
	test_eq_u32("INT1 handler observes A5/3 CIPH source", CIPHER_IRQ, dsp_hw_shared_memory[0x0903] & CIPHER_IRQ);
}

int main(void) {
	test_start("DSP cipher functional test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;

	run_a512();
	run_a53();

	DSP_COM_CLEAR = UINT16_MAX;
	(void) dsp_hw_reset();
	return test_finish();
}
