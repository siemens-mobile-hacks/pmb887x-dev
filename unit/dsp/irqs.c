#include <pmb887x.h>

#include "test.h"

#define DSP_BOOT_PLOAD 0
#define DSP_BOOT_DLOAD 1
#define DSP_BOOT_BRANCH 2
#define DSP_BOOT_DATA_OFFSET 2
#define DSP_BOOT_MAX_WORDS 507
#define DSP_WAIT_ITERATIONS 1000000
#define DSP_STARTUP_ADDRESS 0x0100
#define DSP_IRQ_REQUEST_OFFSET 0x0300
#define DSP_IRQ_READY_OFFSET 0x0301
#define DSP1_HEADER_SIZE 0x300
#define DSP1_FILE_SIZE_OFFSET 0x104
#define DSP1_SEGMENT_COUNT_OFFSET 0x10E
#define DSP1_SEGMENT_TABLE_OFFSET 0x120
#define DSP1_SEGMENT_ENTRY_SIZE 0x30
#define DSP1_MAX_SEGMENTS 10

static volatile uint16_t *const DSP_SHARED_MEMORY = (volatile uint16_t *) DSP_RAM_BASE;

static const uint32_t DSP_IRQS[] = {
	VIC_SCU_DSP0_IRQ,
	VIC_SCU_DSP1_IRQ,
	VIC_SCU_DSP2_IRQ,
	VIC_SCU_DSP3_IRQ,
};

static volatile uint32_t irq_count;
static volatile uint32_t irq_number;

#include "irqs-0602.inc"
#include "irqs-0604.inc"
#include "irqs-0801.inc"

struct dsp_irq_mask_config {
	uint16_t mask_id;
	const uint8_t *image;
	size_t image_size;
};

static const struct dsp_irq_mask_config DSP_IRQ_MASK_CONFIGS[] = {
	{ 0x0602, DSP_IRQ_IMAGE_0602, sizeof(DSP_IRQ_IMAGE_0602) },
	{ 0x0604, DSP_IRQ_IMAGE_0604, sizeof(DSP_IRQ_IMAGE_0604) },
	{ 0x0801, DSP_IRQ_IMAGE_0801, sizeof(DSP_IRQ_IMAGE_0801) },
};

static const struct dsp_irq_mask_config *find_dsp_irq_mask_config(uint16_t mask_id) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IRQ_MASK_CONFIGS); i++) {
		if (DSP_IRQ_MASK_CONFIGS[i].mask_id == mask_id)
			return &DSP_IRQ_MASK_CONFIGS[i];
	}

	return NULL;
}

static bool wait_for_boot_ready(void) {
	for (size_t i = 0; i < DSP_WAIT_ITERATIONS; i++) {
		if ((DSP_COM_STATUS & BIT(0)) == 0)
			return true;
		if ((i & 0x3FFF) == 0)
			test_watchdog_serve();
	}

	return false;
}

static bool reset_dsp(void) {
	DSP_COM_CLEAR = UINT16_MAX;
	SCU_DSP_INT = 0;
	SCU_RST_REQ = SCU_RST_REQ_DSP;
	uint32_t reset_readback = SCU_RST_REQ;
	SCU_RST_REQ = 0;
	(void) reset_readback;

	return wait_for_boot_ready();
}

static bool submit_boot_command(void) {
	DSP_COM_SET = BIT(0);
	SCU_DSP_INT = BIT(0);
	SCU_DSP_INT = 0;

	return wait_for_boot_ready();
}

static bool load_words(uint16_t command, uint16_t destination, const uint16_t *values, size_t words) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = command;
	boot_data[1] = destination;
	boot_data[2] = (uint16_t) words;
	for (size_t i = 0; i < words; i++)
		boot_data[3 + i] = values[i];

	return submit_boot_command();
}

static uint32_t read_le32(const uint8_t *data) {
	return data[0] | (uint32_t) data[1] << 8 | (uint32_t) data[2] << 16 | (uint32_t) data[3] << 24;
}

static bool load_dsp1_image(const uint8_t *image, size_t image_size) {
	if (image_size < DSP1_HEADER_SIZE || image[0x100] != 'D' || image[0x101] != 'S' || image[0x102] != 'P' ||
		image[0x103] != '1' || read_le32(image + DSP1_FILE_SIZE_OFFSET) != image_size ||
		image[DSP1_SEGMENT_COUNT_OFFSET] > DSP1_MAX_SEGMENTS)
		return false;

	uint16_t payload[DSP_BOOT_MAX_WORDS];
	size_t segments = image[DSP1_SEGMENT_COUNT_OFFSET];
	for (size_t i = 0; i < segments; i++) {
		const uint8_t *entry = image + DSP1_SEGMENT_TABLE_OFFSET + i * DSP1_SEGMENT_ENTRY_SIZE;
		uint32_t offset = read_le32(entry);
		uint32_t address = read_le32(entry + 4);
		uint32_t size = read_le32(entry + 8);
		uint8_t memory_type = entry[0x0F];
		size_t words = size / sizeof(uint16_t);

		if (size == 0 || (size & 1) != 0 || words > DSP_BOOT_MAX_WORDS || address > UINT16_MAX ||
			offset > image_size || size > image_size - offset || memory_type > 2)
			return false;
		for (size_t j = 0; j < words; j++)
			payload[j] = image[offset + j * 2] | (uint16_t) image[offset + j * 2 + 1] << 8;
		if (!load_words(memory_type == 2 ? DSP_BOOT_DLOAD : DSP_BOOT_PLOAD, (uint16_t) address, payload, words))
			return false;
	}

	return true;
}

static bool branch_to(uint16_t destination) {
	volatile uint16_t *boot_data = DSP_SHARED_MEMORY + DSP_BOOT_DATA_OFFSET;

	boot_data[0] = DSP_BOOT_BRANCH;
	boot_data[1] = destination;

	return submit_boot_command();
}

static bool wait_for_irq(void) {
	stopwatch_t start = stopwatch_get();

	while (irq_count == 0 && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return irq_count != 0;
}

static bool wait_for_shared_value(size_t offset, uint16_t expected) {
	stopwatch_t start = stopwatch_get();

	while (DSP_SHARED_MEMORY[offset] != expected && stopwatch_elapsed_ms(start) < 100)
		test_watchdog_serve();

	return DSP_SHARED_MEMORY[offset] == expected;
}

static void clear_irq_sources(void) {
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IRQS); i++) {
		SCU_DSP_SRC(i) = MOD_SRC_CLRR;
		VIC_CON(DSP_IRQS[i]) = 0;
	}
	VIC_IRQ_ACK = 1;
}

static void prepare_irq(size_t source) {
	clear_irq_sources();
	irq_count = 0;
	irq_number = 0;
	SCU_DSP_SRC(source) = MOD_SRC_CLRR | MOD_SRC_SRE;
	VIC_CON(DSP_IRQS[source]) = 1;
	cpu_enable_irq(true);
}

static void test_dsp_irq(size_t bit) {
	size_t source = bit;
	char request_name[64];
	char route_name[64];
	char consumed_name[64];

	prepare_irq(source);
	DSP_SHARED_MEMORY[DSP_IRQ_REQUEST_OFFSET] = BIT(bit);
	tfp_sprintf(request_name, "DSP INT_TOMCU bit %u raises DSP_SRC%u", (uint32_t) bit, (uint32_t) source);
	test_check(request_name, wait_for_irq());
	cpu_enable_irq(false);
	tfp_sprintf(consumed_name, "DSP consumes interrupt request bit %u", (uint32_t) bit);
	test_check(consumed_name, wait_for_shared_value(DSP_IRQ_REQUEST_OFFSET, 0));
	tfp_sprintf(route_name, "DSP_SRC%u routes to VIC IRQ %u", (uint32_t) source, DSP_IRQS[source]);
	test_eq_u32(route_name, DSP_IRQS[source], irq_number);
}

static void test_reserved_bit(void) {
	clear_irq_sources();
	irq_count = 0;
	irq_number = 0;
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IRQS); i++) {
		SCU_DSP_SRC(i) = MOD_SRC_CLRR | MOD_SRC_SRE;
		VIC_CON(DSP_IRQS[i]) = 1;
	}
	cpu_enable_irq(true);
	DSP_SHARED_MEMORY[DSP_IRQ_REQUEST_OFFSET] = BIT(4);
	test_check("DSP consumes INT_TOMCU reserved bit 4 request", wait_for_shared_value(DSP_IRQ_REQUEST_OFFSET, 0));
	stopwatch_usleep_wd(1000);
	cpu_enable_irq(false);
	test_eq_u32("INT_TOMCU bit 4 raises no MCU IRQ", 0, irq_count);
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IRQS); i++) {
		char source_name[64];

		tfp_sprintf(source_name, "INT_TOMCU bit 4 leaves DSP_SRC%u clear", (uint32_t) i);
		test_eq_u32(source_name, 0, SCU_DSP_SRC(i) & MOD_SRC_SRR);
	}
}

int main(void) {
	test_start("DSP interrupt test");
	DSP_CLC = 1 << MOD_CLC_RMC_SHIFT;
	if (!test_check("Mask ROM boot dispatcher becomes ready", reset_dsp()))
		return test_finish();
	uint16_t mask_id = DSP_SHARED_MEMORY[0];
	printf("# DSP mask ID: %04X\n", (uint32_t) mask_id);
	const struct dsp_irq_mask_config *mask_config = find_dsp_irq_mask_config(mask_id);
	if (mask_config == NULL) {
		test_skip("DSP-generated interrupts", "Mask ID parameters are not known");
		return test_finish();
	}
	bool loaded = load_dsp1_image(mask_config->image, mask_config->image_size);
	if (!test_check("boot commands load DSP interrupt generator", loaded))
		return test_finish();
	DSP_SHARED_MEMORY[DSP_IRQ_REQUEST_OFFSET] = 0;
	DSP_SHARED_MEMORY[DSP_IRQ_READY_OFFSET] = 0;
	if (!test_check("BRANCH starts DSP interrupt generator", branch_to(DSP_STARTUP_ADDRESS)))
		return test_finish();
	if (!test_check("DSP interrupt generator enters request loop",
		wait_for_shared_value(DSP_IRQ_READY_OFFSET, 0xA55A)))
		return test_finish();

	test_category("DSP-generated interrupt lines");
	for (size_t bit = 0; bit < 4; bit++)
		test_dsp_irq(bit);
	test_reserved_bit();
	clear_irq_sources();

	return test_finish();
}

__IRQ void irq_handler(void) {
	irq_number = VIC_IRQ_CURRENT;
	irq_count++;
	for (size_t i = 0; i < ARRAY_SIZE(DSP_IRQS); i++) {
		if (irq_number == DSP_IRQS[i])
			SCU_DSP_SRC(i) |= MOD_SRC_CLRR;
	}
	VIC_IRQ_ACK = 1;
}
