#include <pmb887x.h>

#include "test.h"

#ifdef PMB8876

#include <string.h>

#define DIF_REQUEST_IRQS 0xFF
#define DIF_IRQ_MASK 0x7FF
#define DIF_ERROR_MASK 0x383F
#define DIF_FATAL_ERRORS (DIF_ERROR_MASK & ~DIF_ERRIRQSS_PHASE)
#define DIF_TIMEOUT_MS 100
#define DMA_RX_CHANNEL 0
#define DMA_TX_CHANNEL 1
#define DMA_RX_REQUEST 5
#define DMA_TX_REQUEST 4
#define DIF_BCSEL0_LOW_BYTE_FROM_TX 0x55550000
#define DIF_BCSEL1_ALL_FROM_BCREG 0x55555555
#define DIF_9BIT_WORD_MASK GENMASK(8, 0)

static const uint8_t tx_data[] = {
	0x03, 0x14, 0x25, 0x36, 0x47, 0x58, 0x69, 0x7A, 0x8B, 0x9C,
	0xAD, 0xBE, 0xCF, 0xD0, 0xE1, 0xF2, 0x12, 0x34, 0x56, 0x78,
	0x9A, 0xBC, 0xDE, 0xF0, 0x0F, 0x1E, 0x2D, 0x3C, 0x4B, 0x5A,
	0x69, 0x78, 0x87, 0x96, 0xA5, 0xB4, 0xC3,
};

static uint8_t rx_data[sizeof(tx_data)];
static uint32_t dma_tx[sizeof(tx_data)] __attribute__((aligned(16)));
static uint32_t dma_rx[sizeof(tx_data)] __attribute__((aligned(16)));
static struct transfer_config {
	uint32_t tx_size;
	uint32_t rx_size;
	uint32_t tx_alignment;
	uint32_t rx_alignment;
	uint32_t tx_bytes_per_stage;
	uint32_t tx_burst_stages;
	uint32_t rx_burst_stages;
} irq_transfer;

static volatile struct transfer_state {
	uint32_t tx_offset;
	uint32_t rx_offset;
	uint32_t tx_status;
	uint32_t rx_status;
	uint32_t error_status;
	uint32_t tx_irqs;
	uint32_t rx_single_irqs;
	uint32_t rx_burst_irqs;
} transfer;

enum dma_flow {
	DMA_CONTROLLED,
	PERIPHERAL_CONTROLLED,
};

struct dma_transfer {
	uint32_t size;
	uint32_t bsconf_bytes;
	enum dma_flow flow;
	uint32_t con;
	uint32_t rx_fifo;
	uint32_t tx_fifo;
	uint32_t channels;
	bool preload_tx;
};

static const uint32_t bsconf_8bit[] = {
	DIF_CSREG_BSCONF_1x8BIT,
	DIF_CSREG_BSCONF_2x8BIT,
	DIF_CSREG_BSCONF_3x8BIT,
	DIF_CSREG_BSCONF_4x8BIT,
};

static void reset_data_conversion(void) {
	DIF_PBCCON = 0;
	DIF_BMREG0 = 0x14830820;
	DIF_BMREG1 = 0x2D4920E6;
	DIF_BMREG2 = 0x460F39AC;
	DIF_BMREG3 = 0x5ED55272;
	DIF_BMREG4 = 0x779B6B38;
	DIF_BMREG5 = 0x000003FE;
	DIF_BCSEL0 = 0;
	DIF_BCSEL1 = 0;
	DIF_BCREG = 0;
	DIF_INVERT_BIT = 0;
	DIF_COEFF_REG1 = 0;
	DIF_COEFF_REG2 = 0;
	DIF_COEFF_REG3 = 0;
	DIF_OFFSET = 0;
}

static void configure_dif(uint32_t con, uint32_t rx_fifo, uint32_t tx_fifo) {
	DIF_RUNCTRL = 0;
	reset_data_conversion();
	DIF_CON = con | DIF_CON_LB;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_SERIAL;
	DIF_CSREG = DIF_CSREG_BSCONF_OFF;
	DIF_BR = 0;
	DIF_FDIV = 0;
	DIF_RXFIFO_CFG = rx_fifo;
	DIF_TXFIFO_CFG = tx_fifo;
	DIF_DMAE = 0;
	DIF_IMSC = 0;
	DIF_ICR = DIF_IRQ_MASK;
	DIF_ERRIRQSM = DIF_ERROR_MASK;
	DIF_ERRIRQSC = DIF_ERROR_MASK;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static void configure_8bit_split(uint32_t bytes, const uint8_t *value) {
	uint32_t fixed_bytes = (
		(uint32_t) value[1] << 8 |
		(uint32_t) value[2] << 16 |
		(uint32_t) value[3] << 24
	);

	DIF_RUNCTRL = 0;
	DIF_BCSEL0 = DIF_BCSEL0_LOW_BYTE_FROM_TX;
	DIF_BCSEL1 = DIF_BCSEL1_ALL_FROM_BCREG;
	DIF_BCREG = fixed_bytes;
	DIF_CSREG = bsconf_8bit[bytes - 1];
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

static void write_tx_fifo(uint32_t stages) {
	while (stages-- != 0 && transfer.tx_offset < irq_transfer.tx_size) {
		uint32_t stage_end = transfer.tx_offset + irq_transfer.tx_bytes_per_stage;
		uint32_t value = 0;

		if (stage_end > irq_transfer.tx_size)
			stage_end = irq_transfer.tx_size;
		for (uint32_t byte = 0; byte < 4 / irq_transfer.tx_alignment; byte++) {
			uint32_t offset = transfer.tx_offset + byte;

			if (offset >= stage_end)
				break;
			value |= (uint32_t) tx_data[offset] << (byte * irq_transfer.tx_alignment * 8);
		}
		transfer.tx_offset = stage_end;
		DIF_TXD = value;
	}
}

static void read_rx_fifo(uint32_t stages) {
	uint32_t available = DIF_RXFFS_STAT;

	if (stages > available)
		stages = available;
	while (stages-- != 0 && transfer.rx_offset < irq_transfer.rx_size) {
		uint32_t value = DIF_RXD;

		for (uint32_t byte = 0;
			byte < 4 / irq_transfer.rx_alignment && transfer.rx_offset < irq_transfer.rx_size; byte++)
			rx_data[transfer.rx_offset++] = value >> (byte * irq_transfer.rx_alignment * 8);
	}
}

static bool wait_for_transfer(void) {
	stopwatch_t start = stopwatch_get();

	while ((transfer.tx_offset < irq_transfer.tx_size || transfer.rx_offset < irq_transfer.rx_size ||
		(DIF_STAT & DIF_STAT_BSY) != 0) && (transfer.error_status & DIF_FATAL_ERRORS) == 0 &&
		stopwatch_elapsed_ms(start) < DIF_TIMEOUT_MS) {
		transfer.error_status |= DIF_ERRIRQSS;
		test_watchdog_serve();
	}
	transfer.error_status |= DIF_ERRIRQSS;

	return (
		transfer.tx_offset == irq_transfer.tx_size &&
		transfer.rx_offset == irq_transfer.rx_size &&
		(DIF_STAT & DIF_STAT_BSY) == 0 &&
		(transfer.error_status & DIF_FATAL_ERRORS) == 0
	);
}

static void test_registers(void) {
	test_module_id("module ID", 0xF043C000, DIF_ID);
	test_module_clock("module clock", DIF_CLC);
	test_eq_u32(
		"serial loopback configuration",
		DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1 | DIF_CON_LB | DIF_CON_BM_8,
		DIF_CON
	);
	test_eq_u32("serial peripheral mode", DIF_PERREG_DIFPERMODE_SERIAL, DIF_PERREG);
	test_check("DIF interface is running", (DIF_RUNCTRL & DIF_RUNCTRL_RUN) != 0);
	test_eq_u32("DMA starts disabled", 0, DIF_DMAE);
}

static void test_conversion_defaults(void) {
	test_eq_u32("PBCCON reset value", 0, DIF_PBCCON);
	test_eq_u32("BMREG0 identity mapping", 0x14830820, DIF_BMREG0);
	test_eq_u32("BMREG1 identity mapping", 0x2D4920E6, DIF_BMREG1);
	test_eq_u32("BMREG2 identity mapping", 0x460F39AC, DIF_BMREG2);
	test_eq_u32("BMREG3 identity mapping", 0x5ED55272, DIF_BMREG3);
	test_eq_u32("BMREG4 identity mapping", 0x779B6B38, DIF_BMREG4);
	test_eq_u32("BMREG5 identity mapping", 0x000003FE, DIF_BMREG5);
	test_eq_u32("BCSEL0 reset value", 0, DIF_BCSEL0);
	test_eq_u32("BCSEL1 reset value", 0, DIF_BCSEL1);
	test_eq_u32("BCREG reset value", 0, DIF_BCREG);
	test_eq_u32("INVERT_BIT reset value", 0, DIF_INVERT_BIT);
	test_eq_u32("COEFF_REG1 reset value", 0, DIF_COEFF_REG1);
	test_eq_u32("COEFF_REG2 reset value", 0, DIF_COEFF_REG2);
	test_eq_u32("COEFF_REG3 reset value", 0, DIF_COEFF_REG3);
	test_eq_u32("OFFSET reset value", 0, DIF_OFFSET);
}

static void test_irq_status(void) {
	cpu_enable_irq(false);
	DIF_IMSC = 0;
	DIF_ICR = DIF_IRQ_MASK;
	DIF_ISR = DIF_ISR_TXSREQ | DIF_ISR_RXSREQ;
	test_eq_u32(
		"software IRQ sets raw status",
		DIF_RIS_TXSREQ | DIF_RIS_RXSREQ,
		DIF_RIS & (DIF_RIS_TXSREQ | DIF_RIS_RXSREQ)
	);
	test_eq_u32("masked IRQ stays hidden", 0, DIF_MIS & (DIF_MIS_TXSREQ | DIF_MIS_RXSREQ));
	DIF_IMSC = DIF_IMSC_TXSREQ | DIF_IMSC_RXSREQ;
	test_eq_u32(
		"unmasked IRQ appears in MIS",
		DIF_MIS_TXSREQ | DIF_MIS_RXSREQ,
		DIF_MIS & (DIF_MIS_TXSREQ | DIF_MIS_RXSREQ)
	);
	DIF_ICR = DIF_ICR_TXSREQ | DIF_ICR_RXSREQ;
	test_eq_u32("IRQ clear resets status", 0, DIF_RIS & (DIF_RIS_TXSREQ | DIF_RIS_RXSREQ));
	DIF_IMSC = 0;
}

static void execute_irq_loopback_transfer(
	uint32_t tx_count,
	uint32_t tx_size,
	uint32_t tx_bytes_per_stage,
	const uint8_t *expected,
	uint32_t rx_size
) {
	memset(rx_data, 0, sizeof(rx_data));
	transfer = (struct transfer_state) {0};
	irq_transfer.tx_size = tx_size;
	irq_transfer.rx_size = rx_size;
	irq_transfer.tx_alignment = (
		1 << ((DIF_TXFIFO_CFG & DIF_TXFIFO_CFG_TXFA) >> DIF_TXFIFO_CFG_TXFA_SHIFT)
	);
	irq_transfer.rx_alignment = (
		1 << ((DIF_RXFIFO_CFG & DIF_RXFIFO_CFG_RXFA) >> DIF_RXFIFO_CFG_RXFA_SHIFT)
	);
	irq_transfer.tx_bytes_per_stage = tx_bytes_per_stage;
	irq_transfer.tx_burst_stages = (DIF_TXFIFO_CFG & DIF_TXFIFO_CFG_TXFC) != 0 ?
		1 << ((DIF_TXFIFO_CFG & DIF_TXFIFO_CFG_TXBS) >> DIF_TXFIFO_CFG_TXBS_SHIFT) : 1;
	irq_transfer.rx_burst_stages = (DIF_RXFIFO_CFG & DIF_RXFIFO_CFG_RXFC) != 0 ?
		1 << ((DIF_RXFIFO_CFG & DIF_RXFIFO_CFG_RXBS) >> DIF_RXFIFO_CFG_RXBS_SHIFT) : 1;
	VIC_CON(VIC_DIF_RX_SINGLE_IRQ) = 1;
	VIC_CON(VIC_DIF_RX_BURST_IRQ) = 1;
	VIC_CON(VIC_DIF_TX_IRQ) = 1;
	VIC_CON(VIC_DIF_ERR_IRQ) = 1;
	DIF_IMSC = DIF_REQUEST_IRQS;
	cpu_enable_irq(true);
	DIF_TPS_CTRL = tx_count;

	test_check("IRQ loopback transfer completes", wait_for_transfer());
	cpu_enable_irq(false);
	test_eq_memory("IRQ loopback data", expected, rx_data, rx_size);
	test_check("IRQ loopback uses TX requests", transfer.tx_irqs != 0);
	test_check("IRQ loopback uses RX requests", transfer.rx_single_irqs + transfer.rx_burst_irqs != 0);
	test_check("IRQ loopback raises phase event", (transfer.error_status & DIF_ERRIRQSS_PHASE) != 0);
	test_eq_u32("IRQ loopback has no fatal errors", 0, transfer.error_status & DIF_FATAL_ERRORS);
	test_eq_u32("received packet size", rx_size, DIF_RPS_STAT & DIF_RPS_STAT_RPS);
}

static void execute_irq_loopback(uint32_t size, const uint8_t *expected, uint32_t rx_size) {
	uint32_t tx_alignment = 1 << ((DIF_TXFIFO_CFG & DIF_TXFIFO_CFG_TXFA) >> DIF_TXFIFO_CFG_TXFA_SHIFT);

	execute_irq_loopback_transfer(size, size, 4 / tx_alignment, expected, rx_size);
}

static void run_irq_loopback(uint32_t size, uint32_t con, uint32_t rx_fifo, uint32_t tx_fifo) {
	configure_dif(con, rx_fifo, tx_fifo);
	execute_irq_loopback(size, tx_data, size);
}

static void test_bit_conversion(void) {
	uint8_t expected[16];
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;

	configure_dif(DIF_CON_BM_8, fifo, fifo);

	test_category("INVERT_BIT conversion");
	DIF_RUNCTRL = 0;
	for (uint32_t byte = 0; byte < sizeof(expected); byte++)
		expected[byte] = tx_data[byte] ^ 1;
	DIF_INVERT_BIT = 1;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	execute_irq_loopback(sizeof(expected), expected, sizeof(expected));

	DIF_RUNCTRL = 0;
	DIF_INVERT_BIT = 0;
	DIF_BMREG0 = 0x14830801;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	test_category("BMREG bit mapping");
	for (uint32_t byte = 0; byte < sizeof(expected); byte++)
		expected[byte] = (tx_data[byte] & ~3) | ((tx_data[byte] & 1) << 1) | ((tx_data[byte] & 2) >> 1);
	execute_irq_loopback(sizeof(expected), expected, sizeof(expected));

	DIF_RUNCTRL = 0;
	DIF_BMREG0 = 0x14830820;
	DIF_BCSEL0 = 1;
	DIF_BCREG = 1;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	test_category("BCREG bit clamp");
	for (uint32_t byte = 0; byte < sizeof(expected); byte++)
		expected[byte] = tx_data[byte] | 1;
	execute_irq_loopback(sizeof(expected), expected, sizeof(expected));
	DIF_RUNCTRL = 0;
	DIF_BCREG = 0;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	for (uint32_t byte = 0; byte < sizeof(expected); byte++)
		expected[byte] = tx_data[byte] & ~1;
	execute_irq_loopback(sizeof(expected), expected, sizeof(expected));
}

static void test_pixel_conversion(void) {
	uint8_t expected[6];
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;

	for (uint32_t byte = 0; byte < sizeof(expected); byte++)
		expected[byte] = tx_data[byte * 2];
	configure_dif(DIF_CON_BM_8, fifo, fifo);
	DIF_RUNCTRL = 0;
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	test_category("PBCCON pixel packing");
	execute_irq_loopback(12, expected, sizeof(expected));
	DIF_RUNCTRL = 0;
	DIF_COEFF_REG1 = 0x09540800;
	DIF_COEFF_REG2 = 0x095F3B98;
	DIF_COEFF_REG3 = 0x095000CC;
	DIF_OFFSET = 0x01020080;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	test_category("YUV coefficients loopback position");
	execute_irq_loopback(12, expected, sizeof(expected));
}

static void test_irq_burst_boundaries(void) {
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;

	run_irq_loopback(15, DIF_CON_BM_8, fifo, fifo);
	test_check("short packet uses last burst requests", (
		(transfer.tx_status & DIF_RIS_TXLBREQ) != 0 &&
		(transfer.rx_status & DIF_RIS_RXLBREQ) != 0
	));
	run_irq_loopback(16, DIF_CON_BM_8, fifo, fifo);
	test_check("exact burst uses last burst requests", (
		(transfer.tx_status & DIF_RIS_TXLBREQ) != 0 &&
		(transfer.rx_status & DIF_RIS_RXLBREQ) != 0
	));
	run_irq_loopback(17, DIF_CON_BM_8, fifo, fifo);
	test_check("long packet uses burst requests", (
		(transfer.tx_status & DIF_RIS_TXBREQ) != 0 &&
		(transfer.rx_status & DIF_RIS_RXBREQ) != 0
	));
	test_check("long packet ends with last single requests", (
		(transfer.tx_status & DIF_RIS_TXLSREQ) != 0 &&
		(transfer.rx_status & DIF_RIS_RXLSREQ) != 0
	));
}

static void test_irq_fifo_alignment(void) {
	uint32_t fifo2 = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_2 | DIF_RXFIFO_CFG_RXFC;
	uint32_t fifo4 = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;

	run_irq_loopback(17, DIF_CON_BM_8, fifo2, fifo2);
	run_irq_loopback(17, DIF_CON_BM_8, fifo4, fifo4);

	test_category("Asymmetric FIFO alignment");
	run_irq_loopback(
		17,
		DIF_CON_BM_8,
		DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_2 | DIF_RXFIFO_CFG_RXFC,
		DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_1 | DIF_TXFIFO_CFG_TXFC
	);
	run_irq_loopback(
		17,
		DIF_CON_BM_8,
		DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC,
		DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC
	);
}

static void test_bsconf_loopback(void) {
	uint32_t rx_fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	uint32_t tx_fifo = DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC;

	for (uint32_t bytes = 1; bytes <= 4; bytes++) {
		configure_dif(DIF_CON_BM_8, rx_fifo, tx_fifo);
		configure_8bit_split(bytes, tx_data);
		execute_irq_loopback_transfer(1, bytes, bytes, tx_data, bytes);
	}
}

static void test_bsconf_9bit_loopback(void) {
	static const uint32_t configurations[] = {
		DIF_CSREG_BSCONF_1x9BIT,
		DIF_CSREG_BSCONF_2x9BIT,
		DIF_CSREG_BSCONF_3x9BIT,
	};
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4;
	uint32_t value = (
		0x103 |
		0x114 << 9 |
		0x125 << 18
	);

	for (uint32_t words = 1; words <= 3; words++) {
		configure_dif(DIF_CON_BM_9, fifo, fifo);
		DIF_RUNCTRL = 0;
		DIF_CSREG = configurations[words - 1];
		DIF_RUNCTRL = DIF_RUNCTRL_RUN;
		DIF_TXD = value;
		DIF_TPS_CTRL = 1;
		stopwatch_t start = stopwatch_get();
		while ((DIF_STAT & DIF_STAT_BSY) != 0 && stopwatch_elapsed_ms(start) < DIF_TIMEOUT_MS)
			test_watchdog_serve();

		test_eq_u32("9-bit split transfer completes", 0, DIF_STAT & DIF_STAT_BSY);
		test_eq_u32("9-bit split produces one RX stage", 1, DIF_RXFFS_STAT);
		test_eq_u32(
			"9-bit split preserves first loopback word",
			value & DIF_9BIT_WORD_MASK,
			DIF_RXD & DIF_9BIT_WORD_MASK
		);
	}
}

static void test_start_lcd_read(void) {
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;
	uint32_t read_size = 5;

	configure_dif(DIF_CON_BM_8, fifo, fifo);
	DIF_STARTLCDRD = DIF_STARTLCDRD_STARTREAD;
	test_eq_u32("zero-length LCD read stays idle", 0, DIF_STAT & DIF_STAT_BSY);

	DIF_RUNCTRL = 0;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_PARALLEL;
	DIF_CSREG = DIF_CSREG_CS1;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
	DIF_STARTLCDRD = DIF_STARTLCDRD_STARTREAD | (read_size << DIF_STARTLCDRD_READBYTES_SHIFT);
	uint32_t start_status = DIF_STAT;
	test_eq_u32(
		"LCD read size readback",
		read_size,
		(DIF_STARTLCDRD & DIF_STARTLCDRD_READBYTES) >> DIF_STARTLCDRD_READBYTES_SHIFT
	);
	test_check("STARTLCDRD starts parallel receive transaction", (start_status & DIF_STAT_BSY) != 0);
	DIF_RUNCTRL = 0;
	test_eq_u32("LCD read abort clears busy", 0, DIF_STAT & DIF_STAT_BSY);
	test_eq_u32("LCD read abort clears RX FIFO", 0, DIF_RXFFS_STAT);
	DIF_STARTLCDRD = 0;
	configure_dif(DIF_CON_BM_8, fifo, fifo);
	execute_irq_loopback(8, tx_data, 8);
}

static void test_serial_modes(void) {
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;

	run_irq_loopback(17, DIF_CON_BM_8, fifo, fifo);
	run_irq_loopback(17, DIF_CON_PH_1 | DIF_CON_BM_8, fifo, fifo);
	run_irq_loopback(17, DIF_CON_PO_1 | DIF_CON_BM_8, fifo, fifo);
	run_irq_loopback(17, DIF_CON_PH_1 | DIF_CON_PO_1 | DIF_CON_BM_8, fifo, fifo);
	run_irq_loopback(17, DIF_CON_HB_MSB | DIF_CON_BM_8, fifo, fifo);
}

static void test_fifo_errors(void) {
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;
	configure_dif(DIF_CON_BM_8, fifo, fifo);
	DIF_ERRIRQSM = DIF_ERRIRQSM_RXFUFL;
	(void) DIF_RXD;
	test_eq_u32("RX underflow status", DIF_ERRIRQSS_RXFUFL, DIF_ERRIRQSS & DIF_ERRIRQSS_RXFUFL);
	test_check("RX underflow raises error IRQ", (DIF_RIS & DIF_RIS_ERR) != 0);
	DIF_ERRIRQSC = DIF_ERRIRQSC_RXFUFL;
	DIF_ICR = DIF_ICR_ERR;
	test_eq_u32("RX underflow clears", 0, DIF_ERRIRQSS & DIF_ERRIRQSS_RXFUFL);

	DIF_ERRIRQSM = DIF_ERRIRQSM_TXFOFL;
	for (uint32_t stage = 0; stage < 256 && (DIF_ERRIRQSS & DIF_ERRIRQSS_TXFOFL) == 0; stage++)
		DIF_TXD = stage;
	test_eq_u32("TX overflow status", DIF_ERRIRQSS_TXFOFL, DIF_ERRIRQSS & DIF_ERRIRQSS_TXFOFL);
	test_check("TX overflow raises error IRQ", (DIF_RIS & DIF_RIS_ERR) != 0);
	DIF_ERRIRQSC = DIF_ERRIRQSC_TXFOFL;
	DIF_ICR = DIF_ICR_ERR;
	test_eq_u32("TX overflow clears", 0, DIF_ERRIRQSS & DIF_ERRIRQSS_TXFOFL);
}

static void test_abort(void) {
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;
	configure_dif(DIF_CON_BM_8, fifo, fifo);
	DIF_TPS_CTRL = sizeof(tx_data);
	DIF_TXD = 0x12345678;
	test_check("transfer becomes busy", (DIF_STAT & DIF_STAT_BSY) != 0);
	DIF_RUNCTRL = 0;
	test_eq_u32("RUNCTRL abort clears busy", 0, DIF_STAT & DIF_STAT_BSY);
	test_eq_u32("RUNCTRL abort clears TX FIFO", 0, DIF_TXFFS_STAT);
	test_eq_u32("RUNCTRL abort clears RX FIFO", 0, DIF_RXFFS_STAT);
}

static bool run_dma_loopback(const struct dma_transfer *dma) {
	uint32_t all_channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);
	uint32_t rx_alignment = 1 << ((dma->rx_fifo & DIF_RXFIFO_CFG_RXFA) >> DIF_RXFIFO_CFG_RXFA_SHIFT);
	uint32_t tx_alignment = 1 << ((dma->tx_fifo & DIF_TXFIFO_CFG_TXFA) >> DIF_TXFIFO_CFG_TXFA_SHIFT);
	uint32_t rx_bytes_per_stage = 4 / rx_alignment;
	uint32_t tx_bytes_per_stage = 4 / tx_alignment;
	uint32_t rx_transfer_size = 0;
	uint32_t tx_transfer_size = 0;
	uint32_t tx_count = dma->size;

	if (dma->flow == DMA_CONTROLLED) {
		rx_transfer_size = (dma->size + rx_bytes_per_stage - 1) / rx_bytes_per_stage;
		tx_transfer_size = (dma->size + tx_bytes_per_stage - 1) / tx_bytes_per_stage;
	}
	if (dma->bsconf_bytes != 0) {
		tx_count = dma->size / dma->bsconf_bytes;
		if (dma->flow == DMA_CONTROLLED)
			tx_transfer_size = tx_count;
	}

	configure_dif(dma->con, dma->rx_fifo, dma->tx_fifo);
	if (dma->bsconf_bytes != 0)
		configure_8bit_split(dma->bsconf_bytes, tx_data);
	memset(dma_tx, 0, sizeof(dma_tx));
	memset(dma_rx, 0, sizeof(dma_rx));
	memset(rx_data, 0, sizeof(rx_data));
	if (dma->bsconf_bytes != 0) {
		for (uint32_t group = 0; group < tx_count; group++)
			dma_tx[group] = tx_data[group * dma->bsconf_bytes];
	} else if (tx_alignment == 1) {
		memcpy(dma_tx, tx_data, dma->size);
	} else {
		for (uint32_t byte = 0; byte < dma->size; byte++)
			dma_tx[byte / tx_bytes_per_stage] |= (uint32_t) tx_data[byte] <<
				(byte % tx_bytes_per_stage * tx_alignment * 8);
	}
	cpu_enable_irq(false);

	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DMAC_TC_CLEAR = all_channels;
	DMAC_ERR_CLEAR = all_channels;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	SCU_DMARS &= ~(BIT(DMA_RX_REQUEST) | BIT(DMA_TX_REQUEST));

	DMAC_CH_SRC_ADDR(DMA_RX_CHANNEL) = (uint32_t) &DIF_RXD;
	DMAC_CH_DST_ADDR(DMA_RX_CHANNEL) = (uint32_t) dma_rx;
	DMAC_CH_CONTROL(DMA_RX_CHANNEL) = (
		rx_transfer_size | DMAC_CH_CONTROL_SB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = (
		(DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		(dma->flow == PERIPHERAL_CONTROLLED ?
			DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER : DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM) |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC |
		((dma->channels & BIT(DMA_RX_CHANNEL)) != 0 ? DMAC_CH_CONFIG_ENABLE : 0)
	);

	DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL) = (uint32_t) dma_tx;
	DMAC_CH_DST_ADDR(DMA_TX_CHANNEL) = (uint32_t) &DIF_TXD;
	DMAC_CH_CONTROL(DMA_TX_CHANNEL) = (
		tx_transfer_size | DMAC_CH_CONTROL_SB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB2 |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		(dma->flow == PERIPHERAL_CONTROLLED ?
			DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER : DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER) |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC |
		((dma->channels & BIT(DMA_TX_CHANNEL)) != 0 ? DMAC_CH_CONFIG_ENABLE : 0)
	);

	DIF_IMSC = DIF_REQUEST_IRQS;
	DIF_DMAE = DIF_REQUEST_IRQS;
	if (dma->preload_tx)
		DIF_TXD = dma_tx[0];
	DIF_TPS_CTRL = tx_count;
	stopwatch_t start = stopwatch_get();
	while ((DMAC_RAW_TC_STATUS & dma->channels) != dma->channels &&
		(DMAC_RAW_ERR_STATUS & dma->channels) == 0 &&
		stopwatch_elapsed_ms(start) < DIF_TIMEOUT_MS)
		test_watchdog_serve();

	DIF_DMAE = 0;
	DIF_IMSC = 0;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	for (uint32_t byte = 0; byte < dma->size && byte < sizeof(rx_data); byte++)
		rx_data[byte] = dma_rx[byte / rx_bytes_per_stage] >> (byte % rx_bytes_per_stage * rx_alignment * 8);
	return (
		(DMAC_RAW_TC_STATUS & dma->channels) == dma->channels &&
		(DMAC_RAW_ERR_STATUS & dma->channels) == 0
	);
}

static void test_dma(void) {
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1;
	struct dma_transfer dma = {
		.size = sizeof(tx_data),
		.flow = DMA_CONTROLLED,
		.con = DIF_CON_BM_8,
		.rx_fifo = fifo,
		.tx_fifo = fifo,
		.channels = channels,
	};

	test_category("DMA controlled flow");
	test_check("MEM2PER/PER2MEM partial burst completes", run_dma_loopback(&dma));
	test_eq_memory("MEM2PER/PER2MEM partial burst data", tx_data, rx_data, sizeof(tx_data));
	dma.size = 16;
	test_check("MEM2PER/PER2MEM exact burst completes", run_dma_loopback(&dma));
	test_eq_memory("MEM2PER/PER2MEM exact burst data", tx_data, rx_data, 16);

	test_category("Peripheral controlled DMA flow");
	dma.size = sizeof(tx_data);
	dma.flow = PERIPHERAL_CONTROLLED;
	dma.rx_fifo |= DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo |= DIF_TXFIFO_CFG_TXFC;
	test_check("MEM2PER_PER/PER2MEM_PER partial burst completes", run_dma_loopback(&dma));
	test_eq_memory("MEM2PER_PER/PER2MEM_PER partial burst data", tx_data, rx_data, sizeof(tx_data));
	dma.size = 16;
	test_check("MEM2PER_PER/PER2MEM_PER exact burst completes", run_dma_loopback(&dma));
	test_eq_memory("MEM2PER_PER/PER2MEM_PER exact burst data", tx_data, rx_data, 16);

	test_category("DMA FIFO configurations");
	dma.size = 17;
	dma.rx_fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_2 | DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_2 | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA alignment 2 completes", run_dma_loopback(&dma));
	test_eq_memory("DMA alignment 2 data", tx_data, rx_data, 17);
	dma.size = 9;
	dma.rx_fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_4 | DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA alignment 4 completes", run_dma_loopback(&dma));
	test_eq_memory("DMA alignment 4 data", tx_data, rx_data, 9);
	dma.size = 16;
	dma.con = DIF_CON_BM_16;
	dma.rx_fifo = fifo | DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo = fifo | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA 16-bit words complete", run_dma_loopback(&dma));
	test_eq_memory("DMA 16-bit word data", tx_data, rx_data, 16);
	dma.size = sizeof(tx_data);
	dma.con = DIF_CON_BM_8;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_1 | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA asymmetric FIFO bursts complete", run_dma_loopback(&dma));
	test_eq_memory("DMA asymmetric FIFO burst data", tx_data, rx_data, sizeof(tx_data));

	test_category("DMA asymmetric FIFO alignment");
	dma.flow = PERIPHERAL_CONTROLLED;
	dma.size = 17;
	dma.rx_fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_2 | DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_1 | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA TX alignment 1 to RX alignment 2 completes", run_dma_loopback(&dma));
	test_eq_memory("DMA TX alignment 1 to RX alignment 2 data", tx_data, rx_data, dma.size);
	dma.rx_fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXBS_4_WORD | DIF_TXFIFO_CFG_TXFA_4 | DIF_TXFIFO_CFG_TXFC;
	test_check("DMA TX alignment 4 to RX alignment 1 completes", run_dma_loopback(&dma));
	test_eq_memory("DMA TX alignment 4 to RX alignment 1 data", tx_data, rx_data, dma.size);

	test_category("DMA BSCONF loopback");
	dma.flow = DMA_CONTROLLED;
	dma.rx_fifo = DIF_RXFIFO_CFG_RXFA_4;
	dma.tx_fifo = DIF_TXFIFO_CFG_TXFA_4;
	for (uint32_t bytes = 1; bytes <= 4; bytes++) {
		dma.size = bytes;
		dma.bsconf_bytes = bytes;
		test_check("BSCONF MEM2PER/PER2MEM completes", run_dma_loopback(&dma));
		test_eq_memory("BSCONF MEM2PER/PER2MEM data", tx_data, rx_data, bytes);
	}

	test_category("DMA repeated transfers");
	dma.size = sizeof(tx_data);
	dma.bsconf_bytes = 0;
	dma.flow = DMA_CONTROLLED;
	dma.rx_fifo = fifo;
	dma.tx_fifo = fifo;
	bool large_block = true;
	for (uint32_t packet = 0; packet < 28; packet++) {
		large_block &= run_dma_loopback(&dma);
		large_block &= memcmp(tx_data, rx_data, sizeof(tx_data)) == 0;
	}
	test_check("DMA repeated 1 KiB transfer completes", large_block);

	test_category("DMA channel isolation");
	dma.size = 4;
	dma.channels = BIT(DMA_TX_CHANNEL);
	test_check("TX request works without RX DMA", run_dma_loopback(&dma));
	test_eq_u32(
		"TX request does not complete RX channel",
		BIT(DMA_TX_CHANNEL),
		DMAC_RAW_TC_STATUS & channels
	);
	dma.channels = BIT(DMA_RX_CHANNEL);
	dma.preload_tx = true;
	test_check("RX request works without TX DMA", run_dma_loopback(&dma));
	test_eq_memory("RX-only DMA data", tx_data, rx_data, 4);
	test_eq_u32(
		"RX request does not complete TX channel",
		BIT(DMA_RX_CHANNEL),
		DMAC_RAW_TC_STATUS & channels
	);

	test_category("DMA abort and recovery");
	configure_dif(DIF_CON_BM_8, fifo, fifo);
	DMAC_TC_CLEAR = channels;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = (
		(DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM | DMAC_CH_CONFIG_HALT | DMAC_CH_CONFIG_ENABLE
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_HALT | DMAC_CH_CONFIG_ENABLE
	);
	DIF_IMSC = DIF_REQUEST_IRQS;
	DIF_DMAE = DIF_REQUEST_IRQS;
	DIF_TXD = tx_data[0] | (tx_data[1] << 8) | (tx_data[2] << 16) | (tx_data[3] << 24);
	DIF_TPS_CTRL = sizeof(tx_data);
	test_check("halted DMA transfer becomes busy", (DIF_STAT & DIF_STAT_BSY) != 0);
	DIF_RUNCTRL = 0;
	test_eq_u32("DMA abort clears busy", 0, DIF_STAT & DIF_STAT_BSY);
	DIF_DMAE = 0;
	DIF_IMSC = 0;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	dma.size = 16;
	dma.channels = channels;
	dma.preload_tx = false;
	test_check("DMA recovers after abort", run_dma_loopback(&dma));
	test_eq_memory("DMA recovery data", tx_data, rx_data, 16);
	test_eq_u32("DMA channels reach terminal count", channels, DMAC_RAW_TC_STATUS & channels);
	test_eq_u32("DMA channels have no bus errors", 0, DMAC_RAW_ERR_STATUS & channels);
	test_eq_u32("DMA channels stop after last requests", 0, DMAC_EN_CHAN & channels);
}

int dif_v2_test(void) {
	test_start("DIFv2 peripheral test");

	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_category("Conversion reset values");
	test_conversion_defaults();
	uint32_t fifo = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1 | DIF_RXFIFO_CFG_RXFC;
	configure_dif(DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1 | DIF_CON_BM_8, fifo, fifo);

	test_category("Registers");
	test_registers();
	test_category("IRQ status and masks");
	test_irq_status();
	test_category("IRQ loopback");
	run_irq_loopback(
		sizeof(tx_data),
		DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1 | DIF_CON_BM_8,
		fifo,
		fifo
	);
	test_category("Burst boundaries");
	test_irq_burst_boundaries();
	test_category("FIFO alignment");
	test_irq_fifo_alignment();
	test_category("BSCONF loopback");
	test_bsconf_loopback();
	test_category("BSCONF 9-bit loopback");
	test_bsconf_9bit_loopback();
	test_category("STARTLCDRD");
	test_start_lcd_read();
	test_category("Serial modes");
	test_serial_modes();
	test_category("16-bit words");
	run_irq_loopback(16, DIF_CON_BM_16, fifo, fifo);
	test_bit_conversion();
	test_pixel_conversion();
	test_category("FIFO errors");
	test_fifo_errors();
	test_category("Abort and restart");
	test_abort();
	run_irq_loopback(17, DIF_CON_BM_8, fifo, fifo);
	return test_finish();
}

int dif_v2_dma_test(void) {
	test_start("DIFv2 DMA test");

	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_dma();

	return test_finish();
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;
	uint32_t status = DIF_RIS;

	if (irq == VIC_DIF_TX_IRQ) {
		transfer.tx_irqs++;
		transfer.tx_status |= status;
		write_tx_fifo(
			(status & (DIF_RIS_TXBREQ | DIF_RIS_TXLBREQ)) != 0 ? irq_transfer.tx_burst_stages : 1
		);
		DIF_ICR = status & (DIF_ICR_TXLSREQ | DIF_ICR_TXSREQ | DIF_ICR_TXLBREQ | DIF_ICR_TXBREQ);
	} else if (irq == VIC_DIF_RX_SINGLE_IRQ || irq == VIC_DIF_RX_BURST_IRQ) {
		if (irq == VIC_DIF_RX_SINGLE_IRQ)
			transfer.rx_single_irqs++;
		else
			transfer.rx_burst_irqs++;
		transfer.rx_status |= status;
		read_rx_fifo(
			(status & (DIF_RIS_RXBREQ | DIF_RIS_RXLBREQ)) != 0 ? irq_transfer.rx_burst_stages : 1
		);
		DIF_ICR = status & (DIF_ICR_RXLSREQ | DIF_ICR_RXSREQ | DIF_ICR_RXLBREQ | DIF_ICR_RXBREQ);
	} else if (irq == VIC_DIF_ERR_IRQ) {
		transfer.error_status |= DIF_ERRIRQSS;
		DIF_ERRIRQSC = DIF_ERRIRQSS;
		DIF_ICR = DIF_ICR_ERR;
	}

	VIC_IRQ_ACK = 1;
}

#else

int dif_v2_test(void) {
	test_start("DIFv2 peripheral test");
	test_skip("DIFv2 is available", "PMB8875 has the previous DIF controller");

	return test_finish();
}

int dif_v2_dma_test(void) {
	test_start("DIFv2 DMA test");
	test_skip("DIFv2 is available", "PMB8875 has the previous DIF controller");

	return test_finish();
}

#endif
