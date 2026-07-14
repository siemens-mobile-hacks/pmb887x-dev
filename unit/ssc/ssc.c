#include <pmb887x.h>
#include <string.h>

#include "test.h"

#define SSC_WAIT_ITERATIONS 300000
#define SSC_BLOCK_WORDS 256
#define SSC_RX_FIFO_WORDS 32
#define SSC_IRQ_MASK (SSC_IMSC_TX | SSC_IMSC_RX | SSC_IMSC_ERR)
#define SSC_DMA_TX_REQUEST 2
#define SSC_DMA_RX_REQUEST 3
#define SSC_DMA_TX_CHANNEL 4
#define SSC_DMA_RX_CHANNEL 5
#define SSC_DMA_CHANNELS (BIT(SSC_DMA_TX_CHANNEL) | BIT(SSC_DMA_RX_CHANNEL))

enum fifo_mode {
	FIFO_OFF,
	FIFO_ON,
	FIFO_TRANSPARENT,
};

static uint16_t source[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static uint16_t destination[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static uint32_t dma_source[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static uint32_t dma_destination[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static uint8_t dma_source8[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static uint8_t dma_destination8[SSC_BLOCK_WORDS] __attribute__((aligned(16)));
static volatile uint32_t tx_irqs;
static volatile uint32_t rx_irqs;
static volatile uint32_t error_irqs;
static volatile uint32_t dma_tx_irqs;
static volatile uint32_t dma_rx_irqs;
static volatile uint32_t irq_tx_index;
static volatile uint32_t irq_rx_index;
static volatile uint32_t irq_words;
static volatile bool irq_loopback_active;

struct dma_lli {
	uint32_t source;
	uint32_t destination;
	uint32_t next;
	uint32_t control;
};

struct dma_options {
	uint32_t words;
	uint32_t width;
	uint32_t source_burst;
	uint32_t destination_burst;
	uint32_t rx_trigger;
	uint32_t tx_trigger;
	uint32_t lli_split;
	bool irq;
};

static struct dma_lli tx_lli __attribute__((aligned(16)));
static struct dma_lli rx_lli __attribute__((aligned(16)));
static const struct dma_options dma_defaults = {
	.width = 16,
	.source_burst = DMAC_CH_CONTROL_SB_SIZE_SZ_1,
	.destination_burst = DMAC_CH_CONTROL_DB_SIZE_SZ_1,
	.rx_trigger = 1,
	.tx_trigger = 4,
};

static void configure_ssc(uint32_t width, enum fifo_mode mode, uint32_t format) {
	SSC_CON = 0;
	SSC_BR = 3;
	SSC_IMSC = 0;
	SSC_ICR = SSC_IRQ_MASK;
	SSC_RXFCON = 0;
	SSC_TXFCON = 0;
	if (mode != FIFO_OFF) {
		uint32_t transparent = mode == FIFO_TRANSPARENT ? SSC_RXFCON_RXTMEN : 0;

		SSC_RXFCON = SSC_RXFCON_RXFEN | SSC_RXFCON_RXFLU | transparent |
			(4 << SSC_RXFCON_RXFITL_SHIFT);
		SSC_TXFCON = SSC_TXFCON_TXFEN | SSC_TXFCON_TXFLU | transparent |
			(4 << SSC_TXFCON_TXFITL_SHIFT);
	}
	SSC_CON = format | SSC_CON_LB | SSC_CON_MS_MASTER | ((width - 1) << SSC_CON_BM_SHIFT);
	SSC_CON |= SSC_CON_EN;
}

static void fill_data(uint32_t width, uint32_t words) {
	uint16_t mask = width == 16 ? UINT16_MAX : (1 << width) - 1;

	for (uint32_t i = 0; i < words; i++)
		source[i] = ((i * 0x2D + 0x53) ^ (i << 7) ^ (i >> 2)) & mask;
	memset(destination, 0, sizeof(destination));
}

static bool transfer_loopback(uint32_t width, enum fifo_mode mode, uint32_t format, uint32_t words) {
	uint32_t tx = 0;
	uint32_t rx = 0;

	configure_ssc(width, mode, format);
	fill_data(width, words);
	for (uint32_t wait = 0; rx < words && wait < SSC_WAIT_ITERATIONS; wait++) {
		if (mode == FIFO_OFF) {
			if (tx == rx && tx < words)
				SSC_TB = source[tx++];
			if ((SSC_RIS & SSC_RIS_RX) != 0) {
				destination[rx++] = SSC_RB;
				SSC_ICR = SSC_ICR_RX;
			}
		} else {
			uint32_t rx_level = (SSC_FSTAT & SSC_FSTAT_RXFFL) >> SSC_FSTAT_RXFFL_SHIFT;

			while (tx < words && tx - rx < 31) {
				SSC_TB = source[tx++];
			}
			rx_level = (SSC_FSTAT & SSC_FSTAT_RXFFL) >> SSC_FSTAT_RXFFL_SHIFT;
			while (rx < words && rx_level-- != 0)
				destination[rx++] = SSC_RB;
			SSC_ICR = SSC_ICR_TX | SSC_ICR_RX;
		}
		test_watchdog_serve();
	}
	return rx == words;
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_SSC_TX_IRQ) {
		tx_irqs++;
		SSC_ICR = SSC_ICR_TX;
		if (irq_loopback_active && irq_tx_index < irq_words)
			SSC_TB = source[irq_tx_index++];
	} else if (irq == VIC_SSC_RX_IRQ) {
		rx_irqs++;
		if (irq_loopback_active) {
			uint32_t rx_level = (SSC_FSTAT & SSC_FSTAT_RXFFL) >> SSC_FSTAT_RXFFL_SHIFT;

			while (irq_rx_index < irq_words && rx_level-- != 0)
				destination[irq_rx_index++] = SSC_RB;
		}
		SSC_ICR = SSC_ICR_RX;
	} else if (irq == VIC_SSC_ERR_IRQ) {
		error_irqs++;
		SSC_ICR = SSC_ICR_ERR;
	} else if (irq == VIC_DMAC_CH0_IRQ + SSC_DMA_TX_CHANNEL) {
		dma_tx_irqs++;
		DMAC_TC_CLEAR = BIT(SSC_DMA_TX_CHANNEL);
	} else if (irq == VIC_DMAC_CH0_IRQ + SSC_DMA_RX_CHANNEL) {
		dma_rx_irqs++;
		DMAC_TC_CLEAR = BIT(SSC_DMA_RX_CHANNEL);
	}
	VIC_IRQ_ACK = 1;
}

static void test_irq_loopback(void) {
	static const uint16_t sizes[] = {1, 3, 4, 5, 31, 32, 33, SSC_BLOCK_WORDS};

	test_category("IRQ loopback");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		configure_ssc(16, FIFO_ON, 0);
		SSC_RXFCON = SSC_RXFCON_RXFEN | SSC_RXFCON_RXFLU | (1 << SSC_RXFCON_RXFITL_SHIFT);
		SSC_TXFCON = SSC_TXFCON_TXFEN | SSC_TXFCON_TXFLU | (1 << SSC_TXFCON_TXFITL_SHIFT);
		fill_data(16, sizes[i]);
		tx_irqs = 0;
		rx_irqs = 0;
		error_irqs = 0;
		irq_tx_index = 0;
		irq_rx_index = 0;
		irq_words = sizes[i];
		irq_loopback_active = true;
		SSC_IMSC = SSC_IRQ_MASK;
		SSC_ISR = SSC_ISR_TX;
		for (uint32_t wait = 0; irq_rx_index < irq_words && wait < SSC_WAIT_ITERATIONS; wait++)
			test_watchdog_serve();
		SSC_IMSC = 0;
		irq_loopback_active = false;
		test_eq_u32("IRQ loopback output size", sizes[i], irq_rx_index);
		test_check("IRQ loopback raises TX IRQ", tx_irqs != 0);
		test_check("IRQ loopback raises RX IRQ", rx_irqs != 0);
		test_eq_u32("IRQ loopback has no error IRQ", 0, error_irqs);
		test_eq_memory("IRQ loopback data", source, destination, sizes[i] * sizeof(source[0]));
	}
}

static void test_registers(void) {
	test_category("Registers");
	test_module_id("module ID", 0x00004500, SSC_ID);
	test_module_clock("module clock", SSC_CLC);
	SSC_CON = 0;
	SSC_BR = 0xA55A;
	test_eq_u32("baud-rate reload", 0xA55A, SSC_BR);
}

static void test_bit_count(void) {
	test_category("Bit count");
	for (uint32_t width = 2; width <= 16; width++) {
		uint32_t mask = width == 16 ? UINT16_MAX : BIT(width) - 1;

		configure_ssc(width, FIFO_OFF, 0);
		SSC_TB = 0xA55A;
		while ((SSC_RIS & SSC_RIS_RX) == 0)
			test_watchdog_serve();
		test_eq_u32("BM limits transferred word width", 0xA55A & mask, SSC_RB);
		SSC_ICR = SSC_ICR_TX | SSC_ICR_RX;
	}
}

static void test_irq_registers(void) {
	test_category("IRQ status and masks");
	tx_irqs = 0;
	rx_irqs = 0;
	error_irqs = 0;
	SSC_IMSC = 0;
	SSC_ICR = SSC_IRQ_MASK;
	SSC_ISR = SSC_ISR_TX | SSC_ISR_RX | SSC_ISR_ERR;
	test_spin(1000);
	test_eq_u32("masked sources do not reach TX IRQ", 0, tx_irqs);
	test_eq_u32("masked sources do not reach RX IRQ", 0, rx_irqs);
	test_eq_u32("masked sources do not reach error IRQ", 0, error_irqs);
	test_eq_u32("ISR sets raw status", SSC_IRQ_MASK, SSC_RIS & SSC_IRQ_MASK);
	test_eq_u32("masked status stays hidden", 0, SSC_MIS & SSC_IRQ_MASK);
	SSC_ICR = SSC_ICR_RX;
	test_eq_u32("ICR clears only selected source", SSC_RIS_TX | SSC_RIS_ERR, SSC_RIS & SSC_IRQ_MASK);
	SSC_IMSC = SSC_IMSC_TX | SSC_IMSC_ERR;
	test_spin(1000);
	test_eq_u32("TX IRQ routed", 1, tx_irqs);
	test_eq_u32("masked RX IRQ remains hidden", 0, rx_irqs);
	test_eq_u32("error IRQ routed", 1, error_irqs);
	test_eq_u32("IRQ handlers clear raw status", 0, SSC_RIS & SSC_IRQ_MASK);
	test_eq_u32("IRQ handlers clear masked status", 0, SSC_MIS & SSC_IRQ_MASK);
	SSC_IMSC = 0;
}

static void test_widths(void) {
	test_category("Data widths");
	for (uint32_t width = 2; width <= 16; width++) {
		test_check("loopback transfer completes", transfer_loopback(width, FIFO_ON, 0, 17));
		test_eq_memory("loopback data", source, destination, 17 * sizeof(source[0]));
	}
}

static void test_formats(void) {
	static const uint32_t formats[] = {
		0,
		SSC_CON_HB_MSB,
		SSC_CON_PH_1,
		SSC_CON_PO_1,
		SSC_CON_HB_MSB | SSC_CON_PH_1 | SSC_CON_PO_1,
	};

	test_category("Serial formats");
	for (uint32_t i = 0; i < ARRAY_SIZE(formats); i++) {
		test_check("format loopback completes", transfer_loopback(16, FIFO_ON, formats[i], 19));
		test_eq_memory("format loopback data", source, destination, 19 * sizeof(source[0]));
	}
}

static void test_fifo_modes(void) {
	static const enum fifo_mode modes[] = {FIFO_OFF, FIFO_ON, FIFO_TRANSPARENT};

	test_category("FIFO modes");
	for (uint32_t i = 0; i < ARRAY_SIZE(modes); i++) {
		test_check("FIFO mode loopback completes", transfer_loopback(16, modes[i], 0, SSC_BLOCK_WORDS));
		test_eq_memory("FIFO mode loopback data", source, destination, sizeof(source));
	}
}

static void test_fifo_capacity(void) {
	uint32_t max_rx_level = 0;

	test_category("FIFO capacity");
	SSC_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_ssc(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < SSC_RX_FIFO_WORDS * 2; i++)
		SSC_TB = i;
	for (uint32_t wait = 0; wait < SSC_WAIT_ITERATIONS; wait++) {
		uint32_t status = SSC_FSTAT;
		uint32_t rx_level = (status & SSC_FSTAT_RXFFL) >> SSC_FSTAT_RXFFL_SHIFT;

		if (rx_level > max_rx_level)
			max_rx_level = rx_level;
		if ((status & SSC_FSTAT_TXFFL) == 0 && (SSC_CON & SSC_CON_BSY) == 0)
			break;
		test_watchdog_serve();
	}
	test_eq_u32("maximum RX FIFO level", SSC_RX_FIFO_WORDS, max_rx_level);
	SSC_RXFCON = SSC_RXFCON_RXFEN | SSC_RXFCON_RXFLU;
	SSC_TXFCON = SSC_TXFCON_TXFEN | SSC_TXFCON_TXFLU;
	SSC_CLC = 1 << MOD_CLC_RMC_SHIFT;
}

static void test_fifo_flush(void) {
	test_category("FIFO flush");
	SSC_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_ssc(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < SSC_RX_FIFO_WORDS; i++)
		SSC_TB = i + 1;
	for (uint32_t wait = 0; wait < SSC_WAIT_ITERATIONS; wait++) {
		if ((SSC_FSTAT & SSC_FSTAT_RXFFL) != 0)
			break;
		test_watchdog_serve();
	}
	test_check("RX FIFO receives data before flush", (SSC_FSTAT & SSC_FSTAT_RXFFL) != 0);
	SSC_RXFCON |= SSC_RXFCON_RXFLU;
	test_eq_u32("RX flush bit self-clears", 0, SSC_RXFCON & SSC_RXFCON_RXFLU);
	test_eq_u32("RX flush empties FIFO", 0, SSC_FSTAT & SSC_FSTAT_RXFFL);

	configure_ssc(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < SSC_RX_FIFO_WORDS * 2; i++)
		SSC_TB = i + 1;
	test_check("TX FIFO queues data before flush", (SSC_FSTAT & SSC_FSTAT_TXFFL) != 0);
	SSC_TXFCON |= SSC_TXFCON_TXFLU;
	test_eq_u32("TX flush bit self-clears", 0, SSC_TXFCON & SSC_TXFCON_TXFLU);
	test_eq_u32("TX flush empties FIFO", 0, SSC_FSTAT & SSC_FSTAT_TXFFL);
	SSC_RXFCON |= SSC_RXFCON_RXFLU;
	SSC_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_check("loopback recovers after FIFO flush", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("FIFO flush recovery data", source, destination, 17 * sizeof(source[0]));
}

static void test_abort_restart(void) {
	test_category("Abort and restart");
	SSC_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_ssc(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < SSC_RX_FIFO_WORDS * 2; i++)
		SSC_TB = i + 1;
	test_check("transfer is active before abort", (SSC_CON & SSC_CON_BSY) != 0 || (SSC_FSTAT & SSC_FSTAT_TXFFL) != 0);
	SSC_CON = 0;
	test_eq_u32("CON.EN stops serial engine", 0, SSC_CON & SSC_CON_EN);
	SSC_RXFCON = SSC_RXFCON_RXFEN | SSC_RXFCON_RXFLU;
	SSC_TXFCON = SSC_TXFCON_TXFEN | SSC_TXFCON_TXFLU;
	SSC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_check("loopback recovers after abort", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("abort recovery data", source, destination, 17 * sizeof(source[0]));
}

static void test_error_irq(void) {
	test_category("Errors");
	configure_ssc(16, FIFO_ON, SSC_CON_REN);
	error_irqs = 0;
	SSC_IMSC = SSC_IMSC_ERR;
	(void) SSC_RB;
	test_spin(1000);
	test_eq_u32("RX underflow raises error IRQ", 1, error_irqs);
	test_check("receive error flag is set", (SSC_CON & SSC_CON_RE) != 0);
	SSC_IMSC = 0;
	test_check("loopback recovers after RX underflow", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("RX underflow recovery data", source, destination, 17 * sizeof(source[0]));
}

static void configure_irqs(void) {
	VIC_CON(VIC_SSC_TX_IRQ) = 2;
	VIC_CON(VIC_SSC_RX_IRQ) = 3;
	VIC_CON(VIC_SSC_ERR_IRQ) = 1;
	cpu_enable_irq(true);
}

int ssc_test(void) {
	test_start("SSC");
	SSC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	configure_irqs();
	test_registers();
	test_bit_count();
	test_irq_registers();
	test_irq_loopback();
	test_widths();
	test_formats();
	test_fifo_modes();
	test_fifo_capacity();
	test_fifo_flush();
	test_abort_restart();
	test_error_irq();
	return test_finish();
}

static bool transfer_dma(const struct dma_options *options) {
	uint32_t first_count = options->lli_split == 0 ? options->words : options->lli_split;
	uint32_t first_interrupt = options->lli_split == 0 ? DMAC_CH_CONTROL_I : 0;
	uint32_t source_width = options->width == 8 ? DMAC_CH_CONTROL_S_WIDTH_BYTE : DMAC_CH_CONTROL_S_WIDTH_DWORD;
	uint32_t destination_width = options->width == 8 ? DMAC_CH_CONTROL_D_WIDTH_BYTE : DMAC_CH_CONTROL_D_WIDTH_DWORD;
	uint32_t item_size = options->width == 8 ? sizeof(dma_source8[0]) : sizeof(dma_source[0]);
	uint32_t source_address = options->width == 8 ? (uint32_t) dma_source8 : (uint32_t) dma_source;
	uint32_t destination_address = options->width == 8 ? (uint32_t) dma_destination8 : (uint32_t) dma_destination;

	for (uint32_t i = 0; i < options->words; i++) {
		uint32_t value = (i * 0x2D + 0x53) ^ (i << 7) ^ (i >> 2);

		if (options->width == 8) {
			dma_source8[i] = value;
			dma_destination8[i] = 0;
		} else {
			dma_source[i] = value & UINT16_MAX;
			dma_destination[i] = 0;
		}
	}
	configure_ssc(options->width, FIFO_ON, 0);
	SSC_RXFCON = SSC_RXFCON_RXFEN | SSC_RXFCON_RXFLU | (options->rx_trigger << SSC_RXFCON_RXFITL_SHIFT);
	SSC_TXFCON = SSC_TXFCON_TXFEN | SSC_TXFCON_TXFLU | (options->tx_trigger << SSC_TXFCON_TXFITL_SHIFT);
	SSC_DMAE = 0;
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(SSC_DMA_TX_CHANNEL) = 0;
	DMAC_CH_CONFIG(SSC_DMA_RX_CHANNEL) = 0;
	DMAC_TC_CLEAR = SSC_DMA_CHANNELS;
	DMAC_ERR_CLEAR = SSC_DMA_CHANNELS;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;

	DMAC_CH_SRC_ADDR(SSC_DMA_RX_CHANNEL) = (uint32_t) &SSC_RB;
	DMAC_CH_DST_ADDR(SSC_DMA_RX_CHANNEL) = destination_address;
	DMAC_CH_CONTROL(SSC_DMA_RX_CHANNEL) = (
		first_count | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
		source_width | destination_width |
		DMAC_CH_CONTROL_DI | first_interrupt
	);
	DMAC_CH_CONFIG(SSC_DMA_RX_CHANNEL) = (
		(SSC_DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);

	DMAC_CH_SRC_ADDR(SSC_DMA_TX_CHANNEL) = source_address;
	DMAC_CH_DST_ADDR(SSC_DMA_TX_CHANNEL) = (uint32_t) &SSC_TB;
	DMAC_CH_CONTROL(SSC_DMA_TX_CHANNEL) = (
		first_count | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		source_width | destination_width |
		DMAC_CH_CONTROL_SI | first_interrupt
	);
	DMAC_CH_CONFIG(SSC_DMA_TX_CHANNEL) = (
		(SSC_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);
	DMAC_CH_LLI(SSC_DMA_RX_CHANNEL) = 0;
	DMAC_CH_LLI(SSC_DMA_TX_CHANNEL) = 0;
	if (options->lli_split != 0) {
		uint32_t remaining = options->words - options->lli_split;

		rx_lli = (struct dma_lli) {
			.source = (uint32_t) &SSC_RB,
			.destination = destination_address + options->lli_split * item_size,
			.control = remaining | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
				source_width | destination_width |
				DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I,
		};
		tx_lli = (struct dma_lli) {
			.source = source_address + options->lli_split * item_size,
			.destination = (uint32_t) &SSC_TB,
			.control = remaining | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
				source_width | destination_width |
				DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I,
		};
		DMAC_CH_LLI(SSC_DMA_RX_CHANNEL) = (uint32_t) &rx_lli;
		DMAC_CH_LLI(SSC_DMA_TX_CHANNEL) = (uint32_t) &tx_lli;
	}

	dma_tx_irqs = 0;
	dma_rx_irqs = 0;
	SSC_DMAE = SSC_DMAE_RX | SSC_DMAE_TX;
	DMAC_CH_CONFIG(SSC_DMA_RX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	DMAC_CH_CONFIG(SSC_DMA_TX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	for (uint32_t wait = 0; wait < SSC_WAIT_ITERATIONS; wait++) {
		if ((DMAC_RAW_ERR_STATUS & SSC_DMA_CHANNELS) != 0)
			break;
		bool complete = options->irq ?
			dma_tx_irqs != 0 && dma_rx_irqs != 0 : (DMAC_RAW_TC_STATUS & SSC_DMA_CHANNELS) == SSC_DMA_CHANNELS;

		if (complete)
			break;
		test_watchdog_serve();
	}
	for (uint32_t wait = 0; wait < SSC_WAIT_ITERATIONS; wait++) {
		if ((SSC_FSTAT & (SSC_FSTAT_RXFFL | SSC_FSTAT_TXFFL)) == 0 && (SSC_CON & SSC_CON_BSY) == 0)
			break;
		test_watchdog_serve();
	}
	SSC_DMAE = 0;
	return options->irq ?
		dma_tx_irqs != 0 && dma_rx_irqs != 0 : (DMAC_RAW_TC_STATUS & SSC_DMA_CHANNELS) == SSC_DMA_CHANNELS;
}

static void check_dma_postconditions(void) {
	test_eq_u32("DMA has no bus errors", 0, DMAC_RAW_ERR_STATUS & SSC_DMA_CHANNELS);
	test_eq_u32(
		"DMA RX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(SSC_DMA_RX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32(
		"DMA TX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(SSC_DMA_TX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32("DMA channels automatically disable", 0, DMAC_EN_CHAN & SSC_DMA_CHANNELS);
	test_eq_u32("DMA RX FIFO drains", 0, SSC_FSTAT & SSC_FSTAT_RXFFL);
	test_eq_u32("DMA TX FIFO drains", 0, SSC_FSTAT & SSC_FSTAT_TXFFL);
	test_eq_u32("DMA loopback has no SSC errors", 0, SSC_CON & (SSC_CON_TE | SSC_CON_RE | SSC_CON_PE | SSC_CON_BE));
}

static void test_dma_full_duplex(void) {
	static const uint16_t sizes[] = {1, 3, 4, 5, 31, 32, 33, 255, 256};
	struct dma_options options = dma_defaults;

	test_category("Full-duplex MEM2PER / PER2MEM");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		options.words = sizes[i];
		test_check("DMA transfer completes", transfer_dma(&options));
		test_eq_memory("DMA loopback data", dma_source, dma_destination, sizes[i] * sizeof(dma_source[0]));
		check_dma_postconditions();
	}
}

static void test_dma_bursts(void) {
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
	options.words = 33;
	for (uint32_t i = 0; i < ARRAY_SIZE(bursts); i++) {
		options.source_burst = bursts[i].source;
		options.destination_burst = bursts[i].destination;
		test_check("burst transfer completes", transfer_dma(&options));
		test_eq_memory("burst loopback data", dma_source, dma_destination, options.words * sizeof(dma_source[0]));
		check_dma_postconditions();
	}
}

static void test_dma_fifo_thresholds(void) {
	static const uint8_t thresholds[] = {1, 4, 16};
	struct dma_options options = dma_defaults;

	test_category("TX FIFO thresholds");
	options.words = 63;
	for (uint32_t i = 0; i < ARRAY_SIZE(thresholds); i++) {
		options.tx_trigger = thresholds[i];
		test_check("threshold transfer completes", transfer_dma(&options));
		test_eq_memory("threshold loopback data", dma_source, dma_destination, options.words * sizeof(dma_source[0]));
		check_dma_postconditions();
	}
}

static void test_dma_status(void) {
	struct dma_options options = dma_defaults;

	test_category("DMA status");
	options.words = 23;
	test_check("status transfer completes", transfer_dma(&options));
	test_eq_u32("raw terminal count", SSC_DMA_CHANNELS, DMAC_RAW_TC_STATUS & SSC_DMA_CHANNELS);
	test_eq_u32("masked terminal count", SSC_DMA_CHANNELS, DMAC_TC_STATUS & SSC_DMA_CHANNELS);
	test_eq_u32("combined interrupt status", SSC_DMA_CHANNELS, DMAC_INT_STATUS & SSC_DMA_CHANNELS);
	test_eq_u32(
		"TX source reaches last item",
		(uint32_t) (dma_source + options.words - 1),
		DMAC_CH_SRC_ADDR(SSC_DMA_TX_CHANNEL)
	);
	test_eq_u32("TX destination remains fixed", (uint32_t) &SSC_TB, DMAC_CH_DST_ADDR(SSC_DMA_TX_CHANNEL));
	test_eq_u32("RX source remains fixed", (uint32_t) &SSC_RB, DMAC_CH_SRC_ADDR(SSC_DMA_RX_CHANNEL));
	test_eq_u32(
		"RX destination reaches last item",
		(uint32_t) (dma_destination + options.words - 1),
		DMAC_CH_DST_ADDR(SSC_DMA_RX_CHANNEL)
	);
	check_dma_postconditions();
}

static void test_dma_lli(void) {
	struct dma_options options = dma_defaults;

	test_category("Linked lists");
	options.words = 37;
	options.lli_split = 17;
	test_check("LLI transfer completes", transfer_dma(&options));
	test_eq_memory("LLI loopback data", dma_source, dma_destination, options.words * sizeof(dma_source[0]));
	test_eq_u32("TX reaches end of LLI chain", 0, DMAC_CH_LLI(SSC_DMA_TX_CHANNEL));
	test_eq_u32("RX reaches end of LLI chain", 0, DMAC_CH_LLI(SSC_DMA_RX_CHANNEL));
	check_dma_postconditions();
}

static void test_dma_widths(void) {
	struct dma_options options = dma_defaults;

	test_category("Data widths");
	options.words = 65;
	options.width = 8;
	test_check("8-bit DMA transfer completes", transfer_dma(&options));
	test_eq_memory("8-bit DMA loopback data", dma_source8, dma_destination8, options.words);
	check_dma_postconditions();
}

static void test_dma_repeated_transfers(void) {
	struct dma_options options = dma_defaults;
	bool complete = true;

	test_category("Repeated transfers");
	options.words = SSC_BLOCK_WORDS;
	for (uint32_t block = 0; block < 4; block++) {
		complete &= transfer_dma(&options);
		complete &= memcmp(dma_source, dma_destination, sizeof(dma_source)) == 0;
	}
	test_check("repeated DMA transfer of at least 1 KiB completes", complete);
	check_dma_postconditions();
}

static void test_dma_irqs(void) {
	struct dma_options options = dma_defaults;

	test_category("DMA interrupts");
	options.words = 17;
	options.irq = true;
	VIC_CON(VIC_DMAC_CH0_IRQ + SSC_DMA_TX_CHANNEL) = 2;
	VIC_CON(VIC_DMAC_CH0_IRQ + SSC_DMA_RX_CHANNEL) = 1;
	cpu_enable_irq(true);
	test_check("IRQ transfer completes", transfer_dma(&options));
	test_eq_u32("TX terminal-count IRQ", 1, dma_tx_irqs);
	test_eq_u32("RX terminal-count IRQ", 1, dma_rx_irqs);
	test_eq_u32("IRQ handler clears terminal count", 0, DMAC_RAW_TC_STATUS & SSC_DMA_CHANNELS);
	test_eq_memory("IRQ loopback data", dma_source, dma_destination, options.words * sizeof(dma_source[0]));
	check_dma_postconditions();
}

int ssc_dma_test(void) {
	test_start("SSC DMA");
	SSC_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_dma_full_duplex();
	test_dma_bursts();
	test_dma_fifo_thresholds();
	test_dma_status();
	test_dma_lli();
	test_dma_widths();
	test_dma_repeated_transfers();
	test_dma_irqs();
	return test_finish();
}
