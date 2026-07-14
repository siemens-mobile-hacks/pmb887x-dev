#include <pmb887x.h>
#include <string.h>

#include "dif-v1.h"
#include "test.h"

#ifdef PMB8875

#define DIF_WAIT_ITERATIONS 300000
#define DIF_BLOCK_WORDS 256
#define DIF_RX_FIFO_WORDS 4
#define DIF_IRQ_MASK (DIF_IMSC_TX | DIF_IMSC_RX | DIF_IMSC_ERR)

enum fifo_mode {
	FIFO_OFF,
	FIFO_ON,
	FIFO_TRANSPARENT,
};

static uint16_t source[DIF_BLOCK_WORDS];
static uint16_t destination[DIF_BLOCK_WORDS];
static volatile uint32_t tx_irqs;
static volatile uint32_t rx_irqs;
static volatile uint32_t error_irqs;
static volatile uint32_t irq_tx_index;
static volatile uint32_t irq_rx_index;
static volatile uint32_t irq_words;
static volatile bool irq_loopback_active;

static void configure_dif(uint32_t width, enum fifo_mode mode, uint32_t format) {
	uint32_t rxfcon = 0;
	uint32_t txfcon = 0;

	if (mode != FIFO_OFF) {
		uint32_t transparent = mode == FIFO_TRANSPARENT ? DIF_RXFCON_RXTMEN : 0;

		rxfcon = DIF_RXFCON_RXFEN | DIF_RXFCON_RXFLU | transparent | (4 << DIF_RXFCON_RXFITL_SHIFT);
		txfcon = DIF_TXFCON_TXFEN | DIF_TXFCON_TXFLU | transparent | (4 << DIF_TXFCON_TXFITL_SHIFT);
	}
	dif_v1_configure(width, rxfcon, txfcon, format);
}

static void fill_data(uint32_t width, uint32_t words) {
	uint16_t mask = width == 16 ? UINT16_MAX : BIT(width) - 1;

	for (uint32_t i = 0; i < words; i++)
		source[i] = ((i * 0x2D + 0x53) ^ (i << 7) ^ (i >> 2)) & mask;
	memset(destination, 0, sizeof(destination));
}

static bool transfer_active(enum fifo_mode mode, uint32_t words) {
	uint32_t tx = 0;
	uint32_t rx = 0;

	for (uint32_t wait = 0; rx < words && wait < DIF_WAIT_ITERATIONS; wait++) {
		if (mode == FIFO_OFF) {
			if (tx == rx && tx < words)
				DIF_TB = source[tx++];
			if ((DIF_RIS & DIF_RIS_RX) != 0) {
				destination[rx++] = DIF_RB;
				DIF_ICR = DIF_ICR_RX;
			}
		} else {
			uint32_t rx_level = (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT;

			while (tx < words && tx - rx < DIF_RX_FIFO_WORDS)
				DIF_TB = source[tx++];
			rx_level = (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT;
			while (rx < words && rx_level-- != 0)
				destination[rx++] = DIF_RB;
			DIF_ICR = DIF_ICR_TX | DIF_ICR_RX;
		}
		test_watchdog_serve();
	}
	return rx == words;
}

static bool transfer_loopback(uint32_t width, enum fifo_mode mode, uint32_t format, uint32_t words) {
	configure_dif(width, mode, format);
	fill_data(width, words);
	return transfer_active(mode, words);
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	if (irq == VIC_DIF_TX_IRQ) {
		tx_irqs++;
		DIF_ICR = DIF_ICR_TX;
		if (irq_loopback_active && irq_tx_index < irq_words)
			DIF_TB = source[irq_tx_index++];
	} else if (irq == VIC_DIF_RX_IRQ) {
		rx_irqs++;
		if (irq_loopback_active) {
			uint32_t rx_level = (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT;

			while (irq_rx_index < irq_words && rx_level-- != 0)
				destination[irq_rx_index++] = DIF_RB;
		}
		DIF_ICR = DIF_ICR_RX;
	} else if (irq == VIC_DIF_ERR_IRQ) {
		error_irqs++;
		DIF_ICR = DIF_ICR_ERR;
	}
	VIC_IRQ_ACK = 1;
}

static void test_irq_loopback(void) {
	static const uint16_t sizes[] = {1, 3, 4, 5, 31, 32, 33, DIF_BLOCK_WORDS};

	test_category("IRQ loopback");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		dif_v1_configure(
			16,
			DIF_RXFCON_RXFEN | DIF_RXFCON_RXFLU | (1 << DIF_RXFCON_RXFITL_SHIFT),
			DIF_TXFCON_TXFEN | DIF_TXFCON_TXFLU | (1 << DIF_TXFCON_TXFITL_SHIFT),
			0
		);
		fill_data(16, sizes[i]);
		tx_irqs = 0;
		rx_irqs = 0;
		error_irqs = 0;
		irq_tx_index = 0;
		irq_rx_index = 0;
		irq_words = sizes[i];
		irq_loopback_active = true;
		DIF_IMSC = DIF_IRQ_MASK;
		DIF_ISR = DIF_ISR_TX;
		for (uint32_t wait = 0; irq_rx_index < irq_words && wait < DIF_WAIT_ITERATIONS; wait++)
			test_watchdog_serve();
		DIF_IMSC = 0;
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
	test_module_id("module ID", 0xF043C000, DIF_ID);
	test_module_clock("module clock", DIF_CLC);
	DIF_CON = 0;
	DIF_BR = 0xA55A;
	test_eq_u32("baud-rate reload", 0xA55A, DIF_BR);
}

static void test_irq_registers(void) {
	test_category("IRQ status and masks");
	tx_irqs = 0;
	rx_irqs = 0;
	error_irqs = 0;
	DIF_IMSC = 0;
	DIF_ICR = DIF_IRQ_MASK;
	DIF_ISR = DIF_ISR_TX | DIF_ISR_RX | DIF_ISR_ERR;
	test_spin(1000);
	test_eq_u32("masked sources do not reach TX IRQ", 0, tx_irqs);
	test_eq_u32("masked sources do not reach RX IRQ", 0, rx_irqs);
	test_eq_u32("masked sources do not reach error IRQ", 0, error_irqs);
	test_eq_u32("ISR sets raw status", DIF_IRQ_MASK, DIF_RIS & DIF_IRQ_MASK);
	test_eq_u32("masked status stays hidden", 0, DIF_MIS & DIF_IRQ_MASK);
	DIF_ICR = DIF_ICR_RX;
	test_eq_u32("ICR clears only selected source", DIF_RIS_TX | DIF_RIS_ERR, DIF_RIS & DIF_IRQ_MASK);
	DIF_IMSC = DIF_IMSC_TX | DIF_IMSC_ERR;
	test_spin(1000);
	test_eq_u32("TX IRQ routed", 1, tx_irqs);
	test_eq_u32("masked RX IRQ remains hidden", 0, rx_irqs);
	test_eq_u32("error IRQ routed", 1, error_irqs);
	test_eq_u32("IRQ handlers clear raw status", 0, DIF_RIS & DIF_IRQ_MASK);
	test_eq_u32("IRQ handlers clear masked status", 0, DIF_MIS & DIF_IRQ_MASK);
	DIF_IMSC = 0;
}

static void test_bit_count(void) {
	test_category("Bit count");
	for (uint32_t width = 2; width <= 16; width++) {
		uint32_t mask = width == 16 ? UINT16_MAX : BIT(width) - 1;

		configure_dif(width, FIFO_OFF, 0);
		DIF_TB = 0xA55A;
		while ((DIF_RIS & DIF_RIS_RX) == 0)
			test_watchdog_serve();
		test_eq_u32("BM limits transferred word width", 0xA55A & mask, DIF_RB);
		DIF_ICR = DIF_ICR_TX | DIF_ICR_RX;
	}
}

static void test_data_widths(void) {
	test_category("Data widths");
	for (uint32_t width = 2; width <= 16; width++) {
		test_check("loopback transfer completes", transfer_loopback(width, FIFO_ON, 0, 33));
		test_eq_memory("loopback data", source, destination, 33 * sizeof(source[0]));
	}
}

static void test_formats(void) {
	static const uint32_t formats[] = {
		0,
		DIF_CON_HB_MSB,
		DIF_CON_PH_1,
		DIF_CON_PO_1,
		DIF_CON_HB_MSB | DIF_CON_PH_1 | DIF_CON_PO_1,
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
		test_check("FIFO mode loopback completes", transfer_loopback(16, modes[i], 0, DIF_BLOCK_WORDS));
		test_eq_memory("FIFO mode loopback data", source, destination, sizeof(source));
	}
}

static void test_fifo_capacity(void) {
	uint32_t max_rx_level = 0;

	test_category("FIFO capacity");
	DIF_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_dif(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < 64; i++)
		DIF_TB = i;
	for (uint32_t wait = 0; wait < DIF_WAIT_ITERATIONS; wait++) {
		uint32_t status = DIF_FSTAT;
		uint32_t rx_level = (status & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT;

		if (rx_level > max_rx_level)
			max_rx_level = rx_level;
		if ((status & DIF_FSTAT_TXFFL) == 0 && (DIF_CON & DIF_CON_BSY) == 0)
			break;
		test_watchdog_serve();
	}
	test_eq_u32("maximum RX FIFO level", DIF_RX_FIFO_WORDS, max_rx_level);
	DIF_RXFCON = DIF_RXFCON_RXFEN | DIF_RXFCON_RXFLU;
	DIF_TXFCON = DIF_TXFCON_TXFEN | DIF_TXFCON_TXFLU;
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
}

static void test_fifo_flush(void) {
	test_category("FIFO flush");
	DIF_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_dif(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < DIF_RX_FIFO_WORDS; i++)
		DIF_TB = i + 1;
	for (uint32_t wait = 0; wait < DIF_WAIT_ITERATIONS; wait++) {
		if ((DIF_FSTAT & DIF_FSTAT_RXFFL) != 0)
			break;
		test_watchdog_serve();
	}
	test_check("RX FIFO receives data before flush", (DIF_FSTAT & DIF_FSTAT_RXFFL) != 0);
	DIF_RXFCON |= DIF_RXFCON_RXFLU;
	test_eq_u32("RX flush bit self-clears", 0, DIF_RXFCON & DIF_RXFCON_RXFLU);
	test_eq_u32("RX flush empties FIFO", 0, DIF_FSTAT & DIF_FSTAT_RXFFL);

	configure_dif(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < DIF_RX_FIFO_WORDS * 2; i++)
		DIF_TB = i + 1;
	test_check("TX FIFO queues data before flush", (DIF_FSTAT & DIF_FSTAT_TXFFL) != 0);
	DIF_TXFCON |= DIF_TXFCON_TXFLU;
	test_eq_u32("TX flush bit self-clears", 0, DIF_TXFCON & DIF_TXFCON_TXFLU);
	test_eq_u32("TX flush empties FIFO", 0, DIF_FSTAT & DIF_FSTAT_TXFFL);
	DIF_RXFCON |= DIF_RXFCON_RXFLU;
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;

	test_check("loopback recovers after FIFO flush", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("FIFO flush recovery data", source, destination, 17 * sizeof(source[0]));
}

static void test_abort_restart(void) {
	test_category("Abort and restart");
	DIF_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	configure_dif(16, FIFO_ON, 0);
	for (uint32_t i = 0; i < DIF_RX_FIFO_WORDS * 2; i++)
		DIF_TB = i + 1;
	test_check("transfer is active before abort", (DIF_CON & DIF_CON_BSY) != 0 || (DIF_FSTAT & DIF_FSTAT_TXFFL) != 0);
	DIF_CON = 0;
	test_eq_u32("CON.EN stops serial engine", 0, DIF_CON & DIF_CON_EN);
	DIF_RXFCON = DIF_RXFCON_RXFEN | DIF_RXFCON_RXFLU;
	DIF_TXFCON = DIF_TXFCON_TXFEN | DIF_TXFCON_TXFLU;
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_check("loopback recovers after abort", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("abort recovery data", source, destination, 17 * sizeof(source[0]));
}

static void test_lcd_registers(void) {
	static const struct {
		volatile uint32_t *reg;
		uint32_t mask;
	} registers[] = {
		{&DIF_BMREG0, 0x7FFF7FFF},
		{&DIF_BMREG1, 0x7FFF7FFF},
		{&DIF_BMREG2, 0x7FFF7FFF},
		{&DIF_BMREG3, 0x7FFF7FFF},
		{&DIF_BMREG4, 0x7FFF7FFF},
		{&DIF_BMREG5, 0x000003FF},
		{&DIF_BCREG, 0x55555555},
		{&DIF_BCSEL0, UINT32_MAX},
		{&DIF_BCSEL1, UINT32_MAX},
	};

	test_category("LCD bit conversion registers");
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	test_eq_u32("pixel-bit conversion mode", DIF_PBCCON_PBBCONV_MODE, DIF_PBCCON);
	for (uint32_t i = 0; i < ARRAY_SIZE(registers); i++) {
		uint32_t value = (0xA55A1234 ^ (i * 0x11111111)) & registers[i].mask;

		*registers[i].reg = value;
		test_eq_u32("LCD conversion register readback", value, *registers[i].reg & registers[i].mask);
	}
}

static void test_lcd_loopback(void) {
	uint16_t expected[33];

	test_category("Bit mapping");
	configure_dif(16, FIFO_ON, 0);
	fill_data(16, ARRAY_SIZE(expected));
	DIF_BMREG0 = 0x14830801;
	for (uint32_t i = 0; i < ARRAY_SIZE(expected); i++)
		expected[i] = (source[i] & ~3) | ((source[i] & 1) << 1) | ((source[i] & 2) >> 1);
	test_check("bit mapping loopback completes", transfer_active(FIFO_ON, ARRAY_SIZE(expected)));
	test_eq_memory("BMREG swaps bit 0 and 1", expected, destination, sizeof(expected));

	test_category("Bit clamp");
	configure_dif(16, FIFO_ON, 0);
	fill_data(16, ARRAY_SIZE(expected));
	DIF_BCSEL0 = 1;
	DIF_BCREG = 1;
	for (uint32_t i = 0; i < ARRAY_SIZE(expected); i++)
		expected[i] = source[i] | 1;
	test_check("bit clamp loopback completes", transfer_active(FIFO_ON, ARRAY_SIZE(expected)));
	test_eq_memory("BCREG clamps bit 0 high", expected, destination, sizeof(expected));

	test_category("Bit mapping fan-out");
	configure_dif(16, FIFO_ON, 0);
	fill_data(16, ARRAY_SIZE(expected));
	DIF_BMREG0 = 0;
	DIF_BMREG1 = 0;
	DIF_BMREG2 = 0;
	for (uint32_t i = 0; i < ARRAY_SIZE(expected); i++)
		expected[i] = (source[i] & 1) != 0 ? UINT16_MAX : 0;
	test_check("bit fan-out loopback completes", transfer_active(FIFO_ON, ARRAY_SIZE(expected)));
	test_eq_memory("BMREG fans input bit 0 out to all bits", expected, destination, sizeof(expected));
}

static void test_pixel_packing(void) {
	uint8_t input[13];
	uint8_t expected[6];
	uint8_t actual[6] = {0};
	uint32_t tx = 0;
	uint32_t rx = 0;

	test_category("Pixel packing");
	for (uint32_t i = 0; i < ARRAY_SIZE(input); i++)
		input[i] = i * 17 + 3;
	for (uint32_t i = 0; i < ARRAY_SIZE(expected); i++)
		expected[i] = input[i * 2];
	configure_dif(8, FIFO_ON, 0);
	DIF_PBCCON = DIF_PBCCON_PBBCONV_MODE;
	for (uint32_t wait = 0; rx < ARRAY_SIZE(actual) && wait < DIF_WAIT_ITERATIONS; wait++) {
		if (tx < ARRAY_SIZE(input))
			DIF_TB = input[tx++];
		uint32_t rx_level = (DIF_FSTAT & DIF_FSTAT_RXFFL) >> DIF_FSTAT_RXFFL_SHIFT;

		while (rx < ARRAY_SIZE(actual) && rx_level-- != 0)
			actual[rx++] = DIF_RB;
		test_watchdog_serve();
	}
	test_eq_u32("pixel packing output size", ARRAY_SIZE(actual), rx);
	test_eq_memory("PBCCON packs even input bytes", expected, actual, sizeof(actual));
	test_spin(1000);
	test_eq_u32("incomplete PBCCON pair emits no word", 0, DIF_FSTAT & DIF_FSTAT_RXFFL);
}

static void test_errors(void) {
	test_category("Errors");
	configure_dif(16, FIFO_ON, DIF_CON_REN);
	error_irqs = 0;
	DIF_IMSC = DIF_IMSC_ERR;
	(void) DIF_RB;
	test_spin(1000);
	test_eq_u32("RX underflow raises error IRQ", 1, error_irqs);
	test_check("receive error flag is set", (DIF_CON & DIF_CON_RE) != 0);
	test_eq_u32("RX underflow does not set unrelated errors", 0, DIF_CON & (DIF_CON_TE | DIF_CON_PE | DIF_CON_BE));
	DIF_IMSC = 0;
	test_check("loopback recovers after RX underflow", transfer_loopback(16, FIFO_ON, 0, 17));
	test_eq_memory("RX underflow recovery data", source, destination, 17 * sizeof(source[0]));
}

static void configure_irqs(void) {
	VIC_CON(VIC_DIF_TX_IRQ) = 2;
	VIC_CON(VIC_DIF_RX_IRQ) = 3;
	VIC_CON(VIC_DIF_ERR_IRQ) = 1;
	cpu_enable_irq(true);
}

int dif_v1_test(void) {
	test_start("DIFv1");
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	configure_irqs();
	test_registers();
	test_irq_registers();
	test_irq_loopback();
	test_bit_count();
	test_data_widths();
	test_formats();
	test_fifo_modes();
	test_fifo_capacity();
	test_fifo_flush();
	test_abort_restart();
	test_lcd_registers();
	test_lcd_loopback();
	test_pixel_packing();
	test_errors();
	return test_finish();
}

#else

int dif_v1_test(void) {
	test_start("DIFv1");
	test_skip("DIFv1", "unsupported");
	return test_finish();
}

#endif
