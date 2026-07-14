#include <pmb887x.h>
#include <string.h>

#include "test.h"

#define USART_IRQ_MASK 0xFF
#define USART_TIMEOUT_MS 100
#define BLOCK_SIZE 1024
#define DMA_RX_CHANNEL 0
#define DMA_TX_CHANNEL 1

enum fifo_mode {
	FIFO_OFF,
	FIFO_ON,
	FIFO_TRANSPARENT,
};

static uint8_t block_tx[BLOCK_SIZE] __attribute__((aligned(16)));
static uint8_t block_rx[BLOCK_SIZE] __attribute__((aligned(16)));

static volatile uint8_t irq_rx_data;
static volatile uint32_t tx_irqs;
static volatile uint32_t tbuf_irqs;
static volatile uint32_t rx_irqs;
static volatile uint32_t error_irqs;
static volatile uint32_t timeout_irqs;
static volatile uint32_t dma_tx_irqs;
static volatile uint32_t dma_rx_irqs;

struct dma_lli {
	uint32_t source;
	uint32_t destination;
	uint32_t next;
	uint32_t control;
};

struct dma_options {
	uint32_t size;
	uint32_t source_burst;
	uint32_t destination_burst;
	uint32_t lli_split;
	bool irq;
};

static struct dma_lli tx_lli __attribute__((aligned(16)));
static struct dma_lli rx_lli __attribute__((aligned(16)));
static const struct dma_options dma_defaults = {
	.source_burst = DMAC_CH_CONTROL_SB_SIZE_SZ_1,
	.destination_burst = DMAC_CH_CONTROL_DB_SIZE_SZ_1,
};

static bool wait_for_status(uint32_t mask) {
	stopwatch_t start = stopwatch_get();

	while ((USART_RIS(USART1) & mask) != mask && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();

	return (USART_RIS(USART1) & mask) == mask;
}

static void configure_usart(uint32_t mode) {
	uint32_t control = mode | USART_CON_FDE | USART_CON_LB;

	USART_CON(USART1) = control;
	USART_BG(USART1) = 0x0C;
	USART_FDV(USART1) = 0x1D8;
	USART_TMO(USART1) = 0;
	USART_DMAE(USART1) = 0;
	USART_IMSC(USART1) = 0;
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_RXFCON(USART1) = 0;
	USART_TXFCON(USART1) = 0;
	USART_WHBCON(USART1) = USART_WHBCON_CLRPE | USART_WHBCON_CLRFE | USART_WHBCON_CLROE;
	USART_CON(USART1) = control | USART_CON_CON_R;
	USART_WHBCON(USART1) = USART_WHBCON_SETREN;
}

static void configure_fifo(enum fifo_mode mode) {
	if (mode == FIFO_OFF) {
		USART_RXFCON(USART1) = 0;
		USART_TXFCON(USART1) = 0;
		return;
	}

	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(4 << USART_RXFCON_RXFITL_SHIFT) | (mode == FIFO_TRANSPARENT ? USART_RXFCON_RXTMEN : 0)
	);
	USART_TXFCON(USART1) = (
		USART_TXFCON_TXFEN | USART_TXFCON_TXFFLU |
		(4 << USART_TXFCON_TXFITL_SHIFT) | (mode == FIFO_TRANSPARENT ? USART_TXFCON_TXTMEN : 0)
	);
}

static void fill_block(void) {
	for (uint32_t i = 0; i < sizeof(block_tx); i++)
		block_tx[i] = (uint8_t) ((i * 29 + 0x53) ^ (i >> 3));
	memset(block_rx, 0, sizeof(block_rx));
}

static bool transfer_block(enum fifo_mode mode) {
	configure_usart(USART_CON_M_ASYNC_8BIT);
	configure_fifo(mode);
	fill_block();

	uint32_t tx = 0;
	uint32_t rx = 0;
	stopwatch_t start = stopwatch_get();
	while (rx < sizeof(block_rx) && stopwatch_elapsed_ms(start) < 500) {
		if (mode == FIFO_OFF) {
			if (tx == rx && tx < sizeof(block_tx)) {
				USART_TXB(USART1) = block_tx[tx++];
				USART_ICR(USART1) = USART_ICR_TX | USART_ICR_TB;
			}
			if ((USART_RIS(USART1) & USART_RIS_RX) != 0) {
				USART_ICR(USART1) = USART_ICR_RX;
				block_rx[rx++] = USART_RXB(USART1);
			}
		} else {
			while (tx < sizeof(block_tx) &&
				((USART_FSTAT(USART1) & USART_FSTAT_TXFFL) >> USART_FSTAT_TXFFL_SHIFT) < 8)
				USART_TXB(USART1) = block_tx[tx++];
			while (rx < sizeof(block_rx) && (USART_FSTAT(USART1) & USART_FSTAT_RXFFL) != 0)
				block_rx[rx++] = USART_RXB(USART1);
		}
		test_watchdog_serve();
	}

	return rx == sizeof(block_rx);
}

static bool transfer_block_mem2per_per2mem(const struct dma_options *options) {
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);
	uint32_t first_count = options->lli_split == 0 ? options->size : options->lli_split;
	uint32_t first_interrupt = options->lli_split == 0 ? DMAC_CH_CONTROL_I : 0;

	configure_usart(USART_CON_M_ASYNC_8BIT);
	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(1 << USART_RXFCON_RXFITL_SHIFT)
	);
	USART_TXFCON(USART1) = (
		USART_TXFCON_TXFEN | USART_TXFCON_TXFFLU |
		(4 << USART_TXFCON_TXFITL_SHIFT)
	);
	fill_block();

	USART_DMAE(USART1) = 0;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DMAC_TC_CLEAR = channels;
	DMAC_ERR_CLEAR = channels;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;
	SCU_DMARS &= ~(BIT(6) | BIT(7));

	DMAC_CH_SRC_ADDR(DMA_RX_CHANNEL) = (uint32_t) &USART_RXB(USART1);
	DMAC_CH_DST_ADDR(DMA_RX_CHANNEL) = (uint32_t) block_rx;
	DMAC_CH_CONTROL(DMA_RX_CHANNEL) = (
		first_count | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_DI | first_interrupt
	);
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = (
		(7 << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM | DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);

	DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL) = (uint32_t) block_tx;
	DMAC_CH_DST_ADDR(DMA_TX_CHANNEL) = (uint32_t) &USART_TXB(USART1);
	DMAC_CH_CONTROL(DMA_TX_CHANNEL) = (
		first_count | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_SI | first_interrupt
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(6 << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC
	);
	DMAC_CH_LLI(DMA_RX_CHANNEL) = 0;
	DMAC_CH_LLI(DMA_TX_CHANNEL) = 0;
	if (options->lli_split != 0) {
		uint32_t remaining = options->size - options->lli_split;

		rx_lli = (struct dma_lli) {
			.source = (uint32_t) &USART_RXB(USART1),
			.destination = (uint32_t) block_rx + options->lli_split,
			.control = remaining | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | options->destination_burst |
				DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
				DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I,
		};
		tx_lli = (struct dma_lli) {
			.source = (uint32_t) block_tx + options->lli_split,
			.destination = (uint32_t) &USART_TXB(USART1),
			.control = remaining | options->source_burst | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
				DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
				DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I,
		};
		DMAC_CH_LLI(DMA_RX_CHANNEL) = (uint32_t) &rx_lli;
		DMAC_CH_LLI(DMA_TX_CHANNEL) = (uint32_t) &tx_lli;
	}

	dma_tx_irqs = 0;
	dma_rx_irqs = 0;
	USART_DMAE(USART1) = USART_DMAE_RX | USART_DMAE_TX;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	stopwatch_t start = stopwatch_get();
	while ((DMAC_RAW_ERR_STATUS & channels) == 0 && stopwatch_elapsed_ms(start) < 500) {
		bool complete = options->irq ?
			dma_tx_irqs != 0 && dma_rx_irqs != 0 :
			(DMAC_RAW_TC_STATUS & channels) == channels;

		if (complete)
			break;
		test_watchdog_serve();
	}

	USART_DMAE(USART1) = 0;
	return options->irq ?
		dma_tx_irqs != 0 && dma_rx_irqs != 0 :
		(DMAC_RAW_TC_STATUS & channels) == channels;
}

static void check_dma_postconditions(void) {
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);

	test_eq_u32("DMA has no bus errors", 0, DMAC_RAW_ERR_STATUS & channels);
	test_eq_u32(
		"DMA RX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(DMA_RX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32(
		"DMA TX transfer size reaches zero",
		0,
		DMAC_CH_CONTROL(DMA_TX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE
	);
	test_eq_u32("DMA channels automatically disable", 0, DMAC_EN_CHAN & channels);
	test_eq_u32("DMA RX FIFO drains", 0, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
	test_eq_u32("DMA TX FIFO drains", 0, USART_FSTAT(USART1) & USART_FSTAT_TXFFL);
	test_eq_u32(
		"DMA loopback has no USART errors",
		0,
		USART_CON(USART1) & (USART_CON_PE | USART_CON_FE | USART_CON_OE)
	);
}

static bool loopback_word(uint32_t tx, uint32_t rx) {
	USART_ICR(USART1) = USART_IRQ_MASK;
	USART_TXB(USART1) = tx;
	if (!wait_for_status(USART_RIS_TX | USART_RIS_RX))
		return false;

	USART_ICR(USART1) = USART_ICR_TX | USART_ICR_TB | USART_ICR_RX;
	return USART_RXB(USART1) == rx;
}

static bool send_without_reading(uint8_t value) {
	USART_ICR(USART1) = USART_ICR_TX | USART_ICR_TB;
	USART_TXB(USART1) = value;
	if (!wait_for_status(USART_RIS_TX))
		return false;
	USART_ICR(USART1) = USART_ICR_TX | USART_ICR_TB;

	return true;
}

static bool send_with_tx_event_ordering(uint8_t value) {
	if (!wait_for_status(USART_RIS_TB))
		return false;
	USART_ICR(USART1) = USART_ICR(USART1) | USART_ICR_TB;
	USART_TXB(USART1) = (USART_TXB(USART1) & ~0xFF) | value;
	if (!wait_for_status(USART_RIS_TB))
		return false;
	USART_ICR(USART1) = USART_ICR(USART1) | USART_ICR_TX;

	return wait_for_status(USART_RIS_TX);
}

static bool send_txb_then_wait_tbuf(uint8_t value) {
	USART_TXB(USART1) = (USART_TXB(USART1) & ~0xFF) | value;
	if (!wait_for_status(USART_RIS_TB))
		return false;
	USART_ICR(USART1) = USART_ICR(USART1) | USART_ICR_TB;

	return true;
}

static bool transfer_block_with_sender(enum fifo_mode mode, bool (*send)(uint8_t)) {
	fill_block();

	for (uint32_t i = 0; i < sizeof(block_tx); i++) {
		if (!send(block_tx[i]))
			return false;

		stopwatch_t start = stopwatch_get();
		if (mode == FIFO_OFF) {
			while ((USART_RIS(USART1) & USART_RIS_RX) == 0 &&
				stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
				test_watchdog_serve();
			if ((USART_RIS(USART1) & USART_RIS_RX) == 0)
				return false;
		} else {
			while ((USART_FSTAT(USART1) & USART_FSTAT_RXFFL) == 0 &&
				stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
				test_watchdog_serve();
			if ((USART_FSTAT(USART1) & USART_FSTAT_RXFFL) == 0)
				return false;
		}

		USART_ICR(USART1) = USART_ICR(USART1) | USART_ICR_RX;
		block_rx[i] = USART_RXB(USART1);
	}

	return true;
}

static void test_registers(void) {
	test_module_id("module ID", 0x00004400, USART_ID(USART1));
	test_module_clock("module clock", USART_CLC(USART1));
	test_eq_u32("baud rate reload readback", 0x0C, USART_BG(USART1));
	test_eq_u32("fractional divider readback", 0x1D8, USART_FDV(USART1));
	test_eq_u32(
		"USART1 loopback configuration",
		USART_CON_M_ASYNC_8BIT | USART_CON_FDE | USART_CON_LB | USART_CON_REN | USART_CON_CON_R,
		USART_CON(USART1)
	);
	test_eq_u32("DMA starts disabled", 0, USART_DMAE(USART1));
	test_eq_u32("timeout starts disabled", 0, USART_TMO(USART1));
}

static void test_loopback(void) {
	static const uint8_t data[] = {0x00, 0x01, 0x55, 0xAA, 0xFE, 0xFF};

	configure_usart(USART_CON_M_ASYNC_8BIT);
	for (uint32_t i = 0; i < ARRAY_SIZE(data); i++)
		test_check("8-bit loopback data", loopback_word(data[i], data[i]));
	test_eq_u32("8-bit loopback has no errors", 0, USART_CON(USART1) & (USART_CON_PE | USART_CON_FE | USART_CON_OE));
}

static void test_modes(void) {
	configure_usart(USART_CON_M_SYNC_8BIT);
	test_check("synchronous 8-bit loopback", loopback_word(0xA5, 0xA5));

	configure_usart(USART_CON_M_ASYNC_PARITY_7BIT | USART_CON_PEN);
	test_check("asynchronous 7-bit even parity loopback", loopback_word(0x55, 0x55));

	configure_usart(USART_CON_M_ASYNC_PARITY_8BIT | USART_CON_PEN | USART_CON_ODD);
	test_check("asynchronous 8-bit odd parity loopback", loopback_word(0xA5, 0x1A5));
	test_eq_u32(
		"loopback modes have no parity or framing errors",
		0,
		USART_CON(USART1) & (USART_CON_PE | USART_CON_FE)
	);

	configure_usart(USART_CON_M_ASYNC_9BIT);
	test_check("asynchronous 9-bit loopback with bit 8 clear", loopback_word(0x055, 0x055));
	test_check("asynchronous 9-bit loopback with bit 8 set", loopback_word(0x155, 0x155));

	configure_usart(USART_CON_M_ASYNC_WAKE_UP_8BIT);
	USART_TXB(USART1) = 0x05A;
	test_check("wake-up frame with bit 8 clear transmits", wait_for_status(USART_RIS_TX));
	stopwatch_usleep_wd(1000);
	test_eq_u32("wake-up mode ignores frame with bit 8 clear", 0, USART_RIS(USART1) & USART_RIS_RX);
	test_check("wake-up loopback with bit 8 set", loopback_word(0x15A, 0x15A));
}

static void test_fifo(void) {
	static const uint8_t data[] = {0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87};
	uint8_t actual[ARRAY_SIZE(data)];

	configure_usart(USART_CON_M_ASYNC_8BIT);
	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(4 << USART_RXFCON_RXFITL_SHIFT)
	);
	USART_TXFCON(USART1) = (
		USART_TXFCON_TXFEN | USART_TXFCON_TXFFLU |
		(4 << USART_TXFCON_TXFITL_SHIFT)
	);
	for (uint32_t i = 0; i < ARRAY_SIZE(data); i++)
		USART_TXB(USART1) = data[i];

	stopwatch_t start = stopwatch_get();
	while ((USART_FSTAT(USART1) & USART_FSTAT_RXFFL) != ARRAY_SIZE(data) &&
		stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	test_eq_u32("RX FIFO receives all bytes", ARRAY_SIZE(data), USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
	test_check("FIFO receive threshold raises RX status", (USART_RIS(USART1) & USART_RIS_RX) != 0);

	for (uint32_t i = 0; i < ARRAY_SIZE(actual); i++)
		actual[i] = USART_RXB(USART1);
	test_eq_memory("FIFO loopback data", data, actual, sizeof(data));
	test_eq_u32("RX FIFO drains completely", 0, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
	test_eq_u32("FIFO loopback has no overrun", 0, USART_CON(USART1) & USART_CON_OE);
}

static void test_fifo_thresholds(void) {
	static const uint8_t levels[] = {1, 4, 8};

	for (uint32_t i = 0; i < ARRAY_SIZE(levels); i++) {
		configure_usart(USART_CON_M_ASYNC_8BIT);
		USART_RXFCON(USART1) = (
			USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
			(levels[i] << USART_RXFCON_RXFITL_SHIFT)
		);
		for (uint32_t byte = 1; byte < levels[i]; byte++)
			send_without_reading((uint8_t) byte);
		test_eq_u32("RX stays inactive below trigger", 0, USART_RIS(USART1) & USART_RIS_RX);
		test_check("RX trigger byte transmits", send_without_reading(levels[i]));
		test_check("RX activates at trigger", (USART_RIS(USART1) & USART_RIS_RX) != 0);
		test_eq_u32("RX FIFO reaches trigger level", levels[i], USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
		USART_RXFCON(USART1) |= USART_RXFCON_RXFFLU;
		test_eq_u32("RX flush empties FIFO", 0, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
		test_eq_u32("RX flush bit clears itself", 0, USART_RXFCON(USART1) & USART_RXFCON_RXFFLU);
	}

	configure_usart(USART_CON_M_ASYNC_8BIT);
	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU | USART_RXFCON_RXTMEN |
		(8 << USART_RXFCON_RXFITL_SHIFT)
	);
	test_check("transparent FIFO byte transmits", send_without_reading(0xA5));
	test_check("transparent FIFO ignores trigger level", (USART_RIS(USART1) & USART_RIS_RX) != 0);
	test_eq_u32("transparent FIFO contains one byte", 1, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);
}

static void test_fifo_errors(void) {
	configure_usart(USART_CON_M_ASYNC_8BIT | USART_CON_OEN);
	USART_RXFCON(USART1) = (
		USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU |
		(8 << USART_RXFCON_RXFITL_SHIFT)
	);
	error_irqs = 0;
	VIC_CON(VIC_USART1_ERR_IRQ) = 1;
	USART_IMSC(USART1) = USART_IMSC_ERR;
	cpu_enable_irq(true);
	bool transmitted = true;
	for (uint32_t i = 0; i < 9; i++)
		transmitted &= send_without_reading((uint8_t) i);
	test_check("RX overflow data transmits", transmitted);

	stopwatch_t start = stopwatch_get();
	while (error_irqs == 0 && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_check("RX overflow raises ERR IRQ", error_irqs != 0);
	test_check("RX overflow sets OE", (USART_CON(USART1) & USART_CON_OE) != 0);
	test_eq_u32("overflow keeps RX FIFO full", 8, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);

	USART_WHBCON(USART1) = USART_WHBCON_CLROE;
	USART_RXFCON(USART1) |= USART_RXFCON_RXFFLU;
	test_eq_u32("WHBCON clears OE", 0, USART_CON(USART1) & USART_CON_OE);
	test_eq_u32("overflow recovery flushes RX FIFO", 0, USART_FSTAT(USART1) & USART_FSTAT_RXFFL);

	error_irqs = 0;
	USART_ICR(USART1) = USART_IRQ_MASK;
	cpu_enable_irq(true);
	(void) USART_RXB(USART1);
	start = stopwatch_get();
	while (error_irqs == 0 && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_check("empty RX read raises ERR IRQ", error_irqs != 0);

	USART_IMSC(USART1) = 0;
	VIC_CON(VIC_USART1_ERR_IRQ) = 0;
	configure_usart(USART_CON_M_ASYNC_8BIT);
	test_check("loopback recovers after FIFO errors", loopback_word(0x5A, 0x5A));
}

static void test_large_blocks(void) {
	test_check("FIFO OFF block completes", transfer_block(FIFO_OFF));
	test_eq_memory("FIFO OFF block data", block_tx, block_rx, sizeof(block_tx));
	test_eq_u32(
		"FIFO OFF block has no errors",
		0,
		USART_CON(USART1) & (USART_CON_PE | USART_CON_FE | USART_CON_OE)
	);

	test_check("FIFO ON block completes", transfer_block(FIFO_ON));
	test_eq_memory("FIFO ON block data", block_tx, block_rx, sizeof(block_tx));
	test_eq_u32(
		"FIFO ON block has no errors",
		0,
		USART_CON(USART1) & (USART_CON_PE | USART_CON_FE | USART_CON_OE)
	);

	test_check("transparent FIFO block completes", transfer_block(FIFO_TRANSPARENT));
	test_eq_memory("transparent FIFO block data", block_tx, block_rx, sizeof(block_tx));
	test_eq_u32(
		"transparent FIFO block has no errors",
		0,
		USART_CON(USART1) & (USART_CON_PE | USART_CON_FE | USART_CON_OE)
	);
}

static void test_tx_event_ordering(void) {
	configure_usart(USART_CON_M_ASYNC_8BIT);
	configure_fifo(FIFO_TRANSPARENT);
	test_check(
		"TX/TBUF event sequence completes",
		transfer_block_with_sender(FIFO_TRANSPARENT, send_with_tx_event_ordering)
	);
	test_eq_memory("TX/TBUF event sequence data", block_tx, block_rx, sizeof(block_tx));
}

static void test_txb_then_tbuf(void) {
	configure_usart(USART_CON_M_ASYNC_8BIT);
	configure_fifo(FIFO_OFF);
	test_check("TXB then TBUF polling completes", transfer_block_with_sender(FIFO_OFF, send_txb_then_wait_tbuf));
	test_eq_memory("TXB then TBUF polling data", block_tx, block_rx, sizeof(block_tx));
}

static void test_dma_full_duplex(void) {
	static const uint16_t sizes[] = {1, 7, 8, 9, 31, 32, 33, 255, BLOCK_SIZE};
	struct dma_options options = dma_defaults;

	test_category("Full-duplex MEM2PER / PER2MEM");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		options.size = sizes[i];
		test_check("DMA transfer completes", transfer_block_mem2per_per2mem(&options));
		test_eq_memory("DMA loopback data", block_tx, block_rx, options.size);
		check_dma_postconditions();
	}
}

static void test_dma_bursts(void) {
	static const struct {
		uint32_t source;
		uint32_t destination;
	} bursts[] = {
		{ DMAC_CH_CONTROL_SB_SIZE_SZ_1, DMAC_CH_CONTROL_DB_SIZE_SZ_1 },
		{ DMAC_CH_CONTROL_SB_SIZE_SZ_4, DMAC_CH_CONTROL_DB_SIZE_SZ_4 },
		{ DMAC_CH_CONTROL_SB_SIZE_SZ_8, DMAC_CH_CONTROL_DB_SIZE_SZ_8 },
	};
	struct dma_options options = dma_defaults;

	test_category("Memory bursts");
	options.size = 33;
	for (uint32_t i = 0; i < ARRAY_SIZE(bursts); i++) {
		options.source_burst = bursts[i].source;
		options.destination_burst = bursts[i].destination;
		test_check("burst transfer completes", transfer_block_mem2per_per2mem(&options));
		test_eq_memory("burst loopback data", block_tx, block_rx, options.size);
		check_dma_postconditions();
	}
}

static void test_dma_status(void) {
	struct dma_options options = dma_defaults;
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);

	test_category("DMA status");
	options.size = 23;
	test_check("status transfer completes", transfer_block_mem2per_per2mem(&options));
	test_eq_u32("raw terminal count", channels, DMAC_RAW_TC_STATUS & channels);
	test_eq_u32("masked terminal count", channels, DMAC_TC_STATUS & channels);
	test_eq_u32("combined interrupt status", channels, DMAC_INT_STATUS & channels);
	test_eq_u32("TX source reaches last byte", (uint32_t) (block_tx + options.size - 1), DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL));
	test_eq_u32("TX destination remains fixed", (uint32_t) &USART_TXB(USART1), DMAC_CH_DST_ADDR(DMA_TX_CHANNEL));
	test_eq_u32("RX source remains fixed", (uint32_t) &USART_RXB(USART1), DMAC_CH_SRC_ADDR(DMA_RX_CHANNEL));
	test_eq_u32(
		"RX destination reaches last byte",
		(uint32_t) (block_rx + options.size - 1),
		DMAC_CH_DST_ADDR(DMA_RX_CHANNEL)
	);
	check_dma_postconditions();
}

static void test_dma_lli(void) {
	struct dma_options options = dma_defaults;

	test_category("Linked lists");
	options.size = 37;
	options.lli_split = 17;
	test_check("LLI transfer completes", transfer_block_mem2per_per2mem(&options));
	test_eq_memory("LLI loopback data", block_tx, block_rx, options.size);
	test_eq_u32("TX reaches end of LLI chain", 0, DMAC_CH_LLI(DMA_TX_CHANNEL));
	test_eq_u32("RX reaches end of LLI chain", 0, DMAC_CH_LLI(DMA_RX_CHANNEL));
	check_dma_postconditions();
}

static void test_dma_irqs(void) {
	struct dma_options options = dma_defaults;
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);

	test_category("DMA interrupts");
	options.size = 17;
	options.irq = true;
	VIC_CON(VIC_DMAC_CH0_IRQ + DMA_RX_CHANNEL) = 2;
	VIC_CON(VIC_DMAC_CH0_IRQ + DMA_TX_CHANNEL) = 1;
	cpu_enable_irq(true);
	test_check("IRQ transfer completes", transfer_block_mem2per_per2mem(&options));
	cpu_enable_irq(false);
	test_eq_u32("TX terminal-count IRQ", 1, dma_tx_irqs);
	test_eq_u32("RX terminal-count IRQ", 1, dma_rx_irqs);
	test_eq_u32("IRQ handler clears terminal count", 0, DMAC_RAW_TC_STATUS & channels);
	test_eq_memory("IRQ loopback data", block_tx, block_rx, options.size);
	check_dma_postconditions();
	VIC_CON(VIC_DMAC_CH0_IRQ + DMA_RX_CHANNEL) = 0;
	VIC_CON(VIC_DMAC_CH0_IRQ + DMA_TX_CHANNEL) = 0;
}

static void test_interrupts(void) {
	configure_usart(USART_CON_M_ASYNC_8BIT);
	tx_irqs = 0;
	tbuf_irqs = 0;
	rx_irqs = 0;
	error_irqs = 0;
	irq_rx_data = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 1;
	VIC_CON(VIC_USART1_TBUF_IRQ) = 1;
	VIC_CON(VIC_USART1_RX_IRQ) = 1;
	VIC_CON(VIC_USART1_ERR_IRQ) = 1;
	USART_IMSC(USART1) = USART_IMSC_TX | USART_IMSC_TB | USART_IMSC_RX | USART_IMSC_ERR;
	cpu_enable_irq(true);
	USART_TXB(USART1) = 0x5A;

	stopwatch_t start = stopwatch_get();
	while ((tx_irqs == 0 || tbuf_irqs == 0 || rx_irqs == 0) && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_check("loopback raises TX IRQ", tx_irqs != 0);
	test_check("loopback raises transmit buffer IRQ", tbuf_irqs != 0);
	test_check("loopback raises RX IRQ", rx_irqs != 0);
	test_eq_u32("RX IRQ receives loopback data", 0x5A, irq_rx_data);
	test_eq_u32("loopback raises no error IRQ", 0, error_irqs);

	USART_IMSC(USART1) = 0;
	VIC_CON(VIC_USART1_TX_IRQ) = 0;
	VIC_CON(VIC_USART1_TBUF_IRQ) = 0;
	VIC_CON(VIC_USART1_RX_IRQ) = 0;
	VIC_CON(VIC_USART1_ERR_IRQ) = 0;
}

static void test_timeout(void) {
	configure_usart(USART_CON_M_ASYNC_8BIT);
	timeout_irqs = 0;
	rx_irqs = 0;
	VIC_CON(VIC_USART1_RX_IRQ) = 1;
	VIC_CON(VIC_USART1_TMO_IRQ) = 1;
	USART_IMSC(USART1) = USART_IMSC_RX | USART_IMSC_TMO;
	cpu_enable_irq(true);
	USART_TXB(USART1) = 0x3C;
	stopwatch_t start = stopwatch_get();
	while (rx_irqs == 0 && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	stopwatch_usleep_wd(1000);
	test_check("disabled timeout still receives loopback byte", rx_irqs != 0);
	test_eq_u32("zero timeout reload disables IRQ", 0, timeout_irqs);

	USART_TMO(USART1) = 32;
	test_eq_u32("timeout reload readback", 32, USART_TMO(USART1));
	USART_TXB(USART1) = 0xC3;
	start = stopwatch_get();
	while (timeout_irqs == 0 && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	test_check("inter-character timeout raises IRQ", timeout_irqs != 0);

	USART_TXB(USART1) = 0x5A;
	start = stopwatch_get();
	while (timeout_irqs < 2 && stopwatch_elapsed_ms(start) < USART_TIMEOUT_MS)
		test_watchdog_serve();
	cpu_enable_irq(false);
	test_check("timeout raises again after ICR", timeout_irqs >= 2);

	USART_TMO(USART1) = 0;
	USART_IMSC(USART1) = 0;
	VIC_CON(VIC_USART1_RX_IRQ) = 0;
	VIC_CON(VIC_USART1_TMO_IRQ) = 0;
}

int usart_test(void) {
	test_start("USART1 peripheral test");

	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	configure_usart(USART_CON_M_ASYNC_8BIT);

	test_category("Registers");
	test_registers();
	test_category("Loopback");
	test_loopback();
	test_category("Frame modes");
	test_modes();
	test_category("FIFO");
	test_fifo();
	test_category("FIFO thresholds");
	test_fifo_thresholds();
	test_category("FIFO errors");
	test_fifo_errors();
	test_category("Large blocks");
	test_large_blocks();
	test_category("Transparent FIFO TX/TBUF event ordering");
	test_tx_event_ordering();
	test_category("FIFO OFF TXB then TBUF polling");
	test_txb_then_tbuf();
	test_category("Interrupts");
	test_interrupts();
	test_category("Receive timeout");
	test_timeout();

	return test_finish();
}

int usart_dma_test(void) {
	test_start("USART1 DMA test");

	USART_CLC(USART1) = 1 << MOD_CLC_RMC_SHIFT;
	test_dma_full_duplex();
	test_dma_bursts();
	test_dma_status();
	test_dma_lli();
	test_dma_irqs();

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_USART1_TX_IRQ) {
		tx_irqs++;
		USART_ICR(USART1) = USART_ICR_TX;
	} else if (irq == VIC_USART1_TBUF_IRQ) {
		tbuf_irqs++;
		USART_ICR(USART1) = USART_ICR_TB;
	} else if (irq == VIC_USART1_RX_IRQ) {
		rx_irqs++;
		irq_rx_data = USART_RXB(USART1);
		USART_ICR(USART1) = USART_ICR_RX;
	} else if (irq == VIC_USART1_ERR_IRQ) {
		error_irqs++;
		USART_ICR(USART1) = USART_ICR_ERR;
	} else if (irq == VIC_USART1_TMO_IRQ) {
		timeout_irqs++;
		USART_ICR(USART1) = USART_ICR_TMO;
	} else if (irq == VIC_DMAC_CH0_IRQ + DMA_TX_CHANNEL) {
		dma_tx_irqs++;
		DMAC_TC_CLEAR = BIT(DMA_TX_CHANNEL);
	} else if (irq == VIC_DMAC_CH0_IRQ + DMA_RX_CHANNEL) {
		dma_rx_irqs++;
		DMAC_TC_CLEAR = BIT(DMA_RX_CHANNEL);
	}

	VIC_IRQ_ACK = 1;
}
