#include <pmb887x.h>
#include <string.h>

#include "dif-v1.h"
#include "test.h"

#ifdef PMB8875

#define DIF_DMA_WAIT_ITERATIONS 300000
#define DIF_DMA_ITEMS 256
#define DIF_DMA_TX_REQUEST 4
#define DIF_DMA_RX_REQUEST 5
#define DIF_DMA_TX_CHANNEL 4
#define DIF_DMA_RX_CHANNEL 5
#define DIF_DMA_CHANNELS (BIT(DIF_DMA_TX_CHANNEL) | BIT(DIF_DMA_RX_CHANNEL))

struct dma_lli {
	uint32_t source;
	uint32_t destination;
	uint32_t next;
	uint32_t control;
};

enum conversion_mode {
	CONVERSION_IDENTITY,
	CONVERSION_SWAP_BITS_0_1,
	CONVERSION_CLAMP_BIT_0,
};

struct dma_options {
	uint32_t items;
	uint32_t width;
	uint32_t source_burst;
	uint32_t destination_burst;
	uint32_t tx_trigger;
	uint32_t lli_split;
	bool irq;
	enum conversion_mode conversion;
};

static uint16_t source16[DIF_DMA_ITEMS] __attribute__((aligned(16)));
static uint16_t destination16[DIF_DMA_ITEMS] __attribute__((aligned(16)));
static uint8_t source8[DIF_DMA_ITEMS] __attribute__((aligned(16)));
static uint8_t destination8[DIF_DMA_ITEMS] __attribute__((aligned(16)));
static uint16_t expected16[DIF_DMA_ITEMS];
static struct dma_lli tx_lli __attribute__((aligned(16)));
static struct dma_lli rx_lli __attribute__((aligned(16)));
static volatile uint32_t tx_irqs;
static volatile uint32_t rx_irqs;

static const struct dma_options dma_defaults = {
	.width = 16,
	.source_burst = DMAC_CH_CONTROL_SB_SIZE_SZ_1,
	.destination_burst = DMAC_CH_CONTROL_DB_SIZE_SZ_1,
	.tx_trigger = 1,
};

static void fill_buffers(const struct dma_options *options) {
	for (uint32_t i = 0; i < options->items; i++) {
		uint32_t value = (i * 0x2D + 0x53) ^ (i << 7) ^ (i >> 2);

		if (options->width == 8) {
			source8[i] = value;
			destination8[i] = 0;
		} else {
			source16[i] = value;
			destination16[i] = 0;
		}
	}
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_DMAC_CH0_IRQ + DIF_DMA_TX_CHANNEL) {
		tx_irqs++;
		DMAC_TC_CLEAR = BIT(DIF_DMA_TX_CHANNEL);
	} else if (irq == VIC_DMAC_CH0_IRQ + DIF_DMA_RX_CHANNEL) {
		rx_irqs++;
		DMAC_TC_CLEAR = BIT(DIF_DMA_RX_CHANNEL);
	}
	VIC_IRQ_ACK = 1;
}

static bool transfer_dma(const struct dma_options *options) {
	uint32_t first_count = options->lli_split == 0 ? options->items : options->lli_split;
	uint32_t first_interrupt = options->lli_split == 0 ? DMAC_CH_CONTROL_I : 0;
	uint32_t source_width = options->width == 8 ? DMAC_CH_CONTROL_S_WIDTH_BYTE : DMAC_CH_CONTROL_S_WIDTH_WORD;
	uint32_t destination_width = options->width == 8 ? DMAC_CH_CONTROL_D_WIDTH_BYTE : DMAC_CH_CONTROL_D_WIDTH_WORD;
	uint32_t item_size = options->width == 8 ? sizeof(source8[0]) : sizeof(source16[0]);
	uint32_t source_address = options->width == 8 ? (uint32_t) source8 : (uint32_t) source16;
	uint32_t destination_address = options->width == 8 ? (uint32_t) destination8 : (uint32_t) destination16;

	fill_buffers(options);
	dif_v1_configure(
		options->width,
		DIF_RXFCON_RXFEN | DIF_RXFCON_RXFLU | (1 << DIF_RXFCON_RXFITL_SHIFT),
		DIF_TXFCON_TXFEN | DIF_TXFCON_TXFLU | (options->tx_trigger << DIF_TXFCON_TXFITL_SHIFT),
		0
	);
	if (options->conversion == CONVERSION_SWAP_BITS_0_1) {
		DIF_BMREG0 = 0x14830801;
	} else if (options->conversion == CONVERSION_CLAMP_BIT_0) {
		DIF_BCSEL0 = 1;
		DIF_BCREG = 1;
	}
	DIF_DMAE = 0;
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(DIF_DMA_TX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DIF_DMA_RX_CHANNEL) = 0;
	DMAC_TC_CLEAR = DIF_DMA_CHANNELS;
	DMAC_ERR_CLEAR = DIF_DMA_CHANNELS;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;

	DMAC_CH_SRC_ADDR(DIF_DMA_RX_CHANNEL) = (uint32_t) &DIF_RB;
	DMAC_CH_DST_ADDR(DIF_DMA_RX_CHANNEL) = destination_address;
	DMAC_CH_CONTROL(DIF_DMA_RX_CHANNEL) = (
		first_count | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
		source_width | destination_width | DMAC_CH_CONTROL_DI | first_interrupt
	);
	DMAC_CH_CONFIG(DIF_DMA_RX_CHANNEL) = (
		(DIF_DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);

	DMAC_CH_SRC_ADDR(DIF_DMA_TX_CHANNEL) = source_address;
	DMAC_CH_DST_ADDR(DIF_DMA_TX_CHANNEL) = (uint32_t) &DIF_TB;
	DMAC_CH_CONTROL(DIF_DMA_TX_CHANNEL) = (
		first_count | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		source_width | destination_width | DMAC_CH_CONTROL_SI | first_interrupt
	);
	DMAC_CH_CONFIG(DIF_DMA_TX_CHANNEL) = (
		(DIF_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);
	DMAC_CH_LLI(DIF_DMA_RX_CHANNEL) = 0;
	DMAC_CH_LLI(DIF_DMA_TX_CHANNEL) = 0;
	if (options->lli_split != 0) {
		uint32_t remaining = options->items - options->lli_split;

		rx_lli = (struct dma_lli) {
			.source = (uint32_t) &DIF_RB,
			.destination = destination_address + options->lli_split * item_size,
			.control = remaining | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
				source_width | destination_width | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I,
		};
		tx_lli = (struct dma_lli) {
			.source = source_address + options->lli_split * item_size,
			.destination = (uint32_t) &DIF_TB,
			.control = remaining | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
				source_width | destination_width | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I,
		};
		DMAC_CH_LLI(DIF_DMA_RX_CHANNEL) = (uint32_t) &rx_lli;
		DMAC_CH_LLI(DIF_DMA_TX_CHANNEL) = (uint32_t) &tx_lli;
	}

	tx_irqs = 0;
	rx_irqs = 0;
	DIF_DMAE = DIF_DMAE_RX | DIF_DMAE_TX;
	DMAC_CH_CONFIG(DIF_DMA_RX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DIF_DMA_TX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	for (uint32_t wait = 0; wait < DIF_DMA_WAIT_ITERATIONS; wait++) {
		if ((DMAC_RAW_ERR_STATUS & DIF_DMA_CHANNELS) != 0)
			break;
		bool complete = options->irq ?
			tx_irqs != 0 && rx_irqs != 0 : (DMAC_RAW_TC_STATUS & DIF_DMA_CHANNELS) == DIF_DMA_CHANNELS;

		if (complete)
			break;
		test_watchdog_serve();
	}
	for (uint32_t wait = 0; wait < DIF_DMA_WAIT_ITERATIONS; wait++) {
		if ((DIF_FSTAT & (DIF_FSTAT_RXFFL | DIF_FSTAT_TXFFL)) == 0 && (DIF_CON & DIF_CON_BSY) == 0)
			break;
		test_watchdog_serve();
	}
	DIF_DMAE = 0;
	return options->irq ?
		tx_irqs != 0 && rx_irqs != 0 : (DMAC_RAW_TC_STATUS & DIF_DMA_CHANNELS) == DIF_DMA_CHANNELS;
}

static void check_dma_postconditions(void) {
	test_eq_u32("DMA has no bus errors", 0, DMAC_RAW_ERR_STATUS & DIF_DMA_CHANNELS);
	test_eq_u32(
		"DMA RX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(DIF_DMA_RX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32(
		"DMA TX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(DIF_DMA_TX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32("DMA channels automatically disable", 0, DMAC_EN_CHAN & DIF_DMA_CHANNELS);
	test_eq_u32("DMA RX FIFO drains", 0, DIF_FSTAT & DIF_FSTAT_RXFFL);
	test_eq_u32("DMA TX FIFO drains", 0, DIF_FSTAT & DIF_FSTAT_TXFFL);
	test_eq_u32("DMA loopback has no DIF errors", 0, DIF_CON & (DIF_CON_TE | DIF_CON_RE | DIF_CON_PE | DIF_CON_BE));
}

static void check_data(const struct dma_options *options, const char *name) {
	if (options->width == 8) {
		test_eq_memory(name, source8, destination8, options->items);
	} else {
		for (uint32_t i = 0; i < options->items; i++) {
			if (options->conversion == CONVERSION_SWAP_BITS_0_1) {
				expected16[i] = (source16[i] & ~3) | ((source16[i] & 1) << 1) | ((source16[i] & 2) >> 1);
			} else if (options->conversion == CONVERSION_CLAMP_BIT_0) {
				expected16[i] = source16[i] | 1;
			} else {
				expected16[i] = source16[i];
			}
		}
		test_eq_memory(name, expected16, destination16, options->items * sizeof(expected16[0]));
	}
}

static void test_full_duplex(void) {
	static const uint16_t sizes[] = {1, 3, 4, 5, 31, 32, 33, 255, 256};
	struct dma_options options = dma_defaults;

	test_category("Full-duplex MEM2PER / PER2MEM");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		options.items = sizes[i];
		test_check("DMA transfer completes", transfer_dma(&options));
		check_data(&options, "DMA loopback data");
		check_dma_postconditions();
	}
}

static void test_bursts(void) {
	static const struct {
		uint32_t source;
		uint32_t destination;
	} bursts[] = {
		{DMAC_CH_CONTROL_SB_SIZE_SZ_1, DMAC_CH_CONTROL_DB_SIZE_SZ_1},
		{DMAC_CH_CONTROL_SB_SIZE_SZ_4, DMAC_CH_CONTROL_DB_SIZE_SZ_4},
		{DMAC_CH_CONTROL_SB_SIZE_SZ_8, DMAC_CH_CONTROL_DB_SIZE_SZ_8},
	};
	struct dma_options options = dma_defaults;

	test_category("Memory bursts");
	options.items = 33;
	for (uint32_t i = 0; i < ARRAY_SIZE(bursts); i++) {
		options.source_burst = bursts[i].source;
		options.destination_burst = bursts[i].destination;
		test_check("burst transfer completes", transfer_dma(&options));
		check_data(&options, "burst loopback data");
		check_dma_postconditions();
	}
}

static void test_fifo_thresholds(void) {
	static const uint8_t THRESHOLDS[] = {1, 4, 16};
	struct dma_options options = dma_defaults;

	test_category("TX FIFO thresholds");
	options.items = 63;
	for (uint32_t i = 0; i < ARRAY_SIZE(THRESHOLDS); i++) {
		options.tx_trigger = THRESHOLDS[i];
		test_check("threshold transfer completes", transfer_dma(&options));
		check_data(&options, "threshold loopback data");
		check_dma_postconditions();
	}
}

static void test_status(void) {
	struct dma_options options = dma_defaults;

	test_category("DMA status");
	options.items = 23;
	test_check("status transfer completes", transfer_dma(&options));
	test_eq_u32("raw terminal count", DIF_DMA_CHANNELS, DMAC_RAW_TC_STATUS & DIF_DMA_CHANNELS);
	test_eq_u32("masked terminal count", DIF_DMA_CHANNELS, DMAC_TC_STATUS & DIF_DMA_CHANNELS);
	test_eq_u32("combined interrupt status", DIF_DMA_CHANNELS, DMAC_INT_STATUS & DIF_DMA_CHANNELS);
	test_eq_u32(
		"TX source reaches last item",
		(uint32_t) (source16 + options.items - 1),
		DMAC_CH_SRC_ADDR(DIF_DMA_TX_CHANNEL)
	);
	test_eq_u32("TX destination remains fixed", (uint32_t) &DIF_TB, DMAC_CH_DST_ADDR(DIF_DMA_TX_CHANNEL));
	test_eq_u32("RX source remains fixed", (uint32_t) &DIF_RB, DMAC_CH_SRC_ADDR(DIF_DMA_RX_CHANNEL));
	test_eq_u32(
		"RX destination reaches last item",
		(uint32_t) (destination16 + options.items - 1),
		DMAC_CH_DST_ADDR(DIF_DMA_RX_CHANNEL)
	);
	check_dma_postconditions();
}

static void test_lli(void) {
	struct dma_options options = dma_defaults;

	test_category("Linked lists");
	options.items = 37;
	options.lli_split = 17;
	test_check("LLI transfer completes", transfer_dma(&options));
	check_data(&options, "LLI loopback data");
	test_eq_u32("TX reaches end of LLI chain", 0, DMAC_CH_LLI(DIF_DMA_TX_CHANNEL));
	test_eq_u32("RX reaches end of LLI chain", 0, DMAC_CH_LLI(DIF_DMA_RX_CHANNEL));
	check_dma_postconditions();
}

static void test_widths(void) {
	struct dma_options options = dma_defaults;

	test_category("Data widths");
	options.items = 65;
	options.width = 8;
	test_check("8-bit DMA transfer completes", transfer_dma(&options));
	check_data(&options, "8-bit DMA loopback data");
	check_dma_postconditions();
}

static void test_conversion(void) {
	struct dma_options options = dma_defaults;

	test_category("LCD bit conversion");
	options.items = 33;
	options.conversion = CONVERSION_SWAP_BITS_0_1;
	test_check("bit mapping DMA completes", transfer_dma(&options));
	check_data(&options, "BMREG maps DMA data");
	check_dma_postconditions();
	options.conversion = CONVERSION_CLAMP_BIT_0;
	test_check("bit clamp DMA completes", transfer_dma(&options));
	check_data(&options, "BCREG clamps DMA data");
	check_dma_postconditions();
}

static void test_repeated_transfers(void) {
	struct dma_options options = dma_defaults;
	bool complete = true;

	test_category("Repeated transfers");
	options.items = DIF_DMA_ITEMS;
	for (uint32_t block = 0; block < 4; block++) {
		complete &= transfer_dma(&options);
		complete &= memcmp(source16, destination16, sizeof(source16)) == 0;
	}
	test_check("repeated DMA transfer of at least 1 KiB completes", complete);
	check_dma_postconditions();
}

static void test_irqs(void) {
	struct dma_options options = dma_defaults;

	test_category("DMA interrupts");
	options.items = 17;
	options.irq = true;
	VIC_CON(VIC_DMAC_CH0_IRQ + DIF_DMA_TX_CHANNEL) = 2;
	VIC_CON(VIC_DMAC_CH0_IRQ + DIF_DMA_RX_CHANNEL) = 1;
	cpu_enable_irq(true);
	test_check("IRQ transfer completes", transfer_dma(&options));
	test_eq_u32("TX terminal-count IRQ", 1, tx_irqs);
	test_eq_u32("RX terminal-count IRQ", 1, rx_irqs);
	test_eq_u32("IRQ handler clears terminal count", 0, DMAC_RAW_TC_STATUS & DIF_DMA_CHANNELS);
	check_data(&options, "IRQ loopback data");
	check_dma_postconditions();
}

int dif_v1_dma_test(void) {
	test_start("DIFv1 DMA");
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_full_duplex();
	test_bursts();
	test_fifo_thresholds();
	test_status();
	test_lli();
	test_widths();
	test_conversion();
	test_repeated_transfers();
	test_irqs();
	return test_finish();
}

#else

int dif_v1_dma_test(void) {
	test_start("DIFv1 DMA");
	test_skip("DIFv1 DMA", "unsupported");
	return test_finish();
}

#endif
