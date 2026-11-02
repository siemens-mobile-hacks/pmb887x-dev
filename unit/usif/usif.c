#include <pmb887x.h>
#include <string.h>

#include "dmac.h"
#include "test.h"
#include "usif.h"

#define USIF_TIMEOUT_MS 100
#define USIF_FIFO_STAGES 8
#define USIF_MAX_REQUESTS 16
#define USIF_MAX_PACKET 64
#define USIF_RX_REQUESTS (USIF_RIS_RXLSREQ | USIF_RIS_RXSREQ | USIF_RIS_RXLBREQ | USIF_RIS_RXBREQ)
#define USIF_TX_REQUESTS (USIF_RIS_TXLSREQ | USIF_RIS_TXSREQ | USIF_RIS_TXLBREQ | USIF_RIS_TXBREQ)
#define USIF_TX_LAST_REQUESTS (USIF_RIS_TXLSREQ | USIF_RIS_TXLBREQ)
#define USIF_RX_LAST_REQUESTS (USIF_RIS_RXLSREQ | USIF_RIS_RXLBREQ)
#define USIF_GROUP_SUMMARIES (USIF_RIS_ERR | USIF_RIS_PIRQ)
#define USIF_BURST_REQUESTS (USIF_RIS_RXLBREQ | USIF_RIS_RXBREQ | USIF_RIS_TXLBREQ | USIF_RIS_TXBREQ)
#define DMA_RX_CHANNEL 0
#define DMA_TX_CHANNEL 1

struct request_sequence {
	uint32_t count;
	uint32_t requests[USIF_MAX_REQUESTS];
};

struct request_case {
	uint32_t size;
	struct request_sequence expected;
};

static uint8_t tx_bytes[USIF_MAX_PACKET] __attribute__((aligned(16)));
static uint32_t rx_words[USIF_MAX_PACKET / 4] __attribute__((aligned(16)));

static void fill_tx_bytes(void) {
	for (uint32_t index = 0; index < sizeof(tx_bytes); index++)
		tx_bytes[index] = (uint8_t) (0xA5 ^ (index * 0x1D));
}

static uint32_t tx_word(uint32_t word) {
	uint32_t value = 0;
	memcpy(&value, &tx_bytes[word * 4], sizeof(value));
	return value;
}

static void usif_configure(bool loopback, uint32_t bcrv, uint32_t txfifo_cfg, uint32_t rxfifo_cfg) {
	USIF_RUNCTRL = 0;
	USIF_IMSC = 0;
	USIF_DMAE = 0;
	USIF_ERRIRQSM = 0;
	USIF_PIRQSM = 0;
	USIF_TPS_CTRL = 0;
	USIF_MRPS_CTRL = 0;
	USIF_MODE_CFG = USIF_MODE_CFG_RXEN | (loopback ? USIF_MODE_CFG_LB : 0);
	USIF_PRTC_CFG = 8 << USIF_PRTC_CFG_CLEN_SHIFT;
	USIF_BC_CFG = bcrv << USIF_BC_CFG_BCRV_SHIFT;
	USIF_FDIV_CFG = 1 << USIF_FDIV_CFG_INC_SHIFT;
	USIF_ICTMO_CFG = 58 << USIF_ICTMO_CFG_TMO_SHIFT;
	USIF_TXFIFO_CFG = txfifo_cfg;
	USIF_RXFIFO_CFG = rxfifo_cfg;
	USIF_ICR = 0xFFFFFFFF;
	USIF_ERRIRQSC = 0xFFFFFFFF;
	USIF_PIRQSC = 0xFFFFFFFF;
}

static bool wait_for_pirq(uint32_t mask) {
	stopwatch_t start = stopwatch_get();
	while ((USIF_PIRQSS & mask) != mask && stopwatch_elapsed_ms(start) < USIF_TIMEOUT_MS)
		test_watchdog_serve();
	return (USIF_PIRQSS & mask) == mask;
}

static uint32_t wait_for_request(uint32_t mask) {
	stopwatch_t start = stopwatch_get();
	uint32_t requests = 0;
	while ((requests = USIF_RIS & mask) == 0 && stopwatch_elapsed_ms(start) < USIF_TIMEOUT_MS)
		test_watchdog_serve();
	return requests;
}

static const char *request_name(uint32_t request) {
	switch (request) {
		case USIF_RIS_RXLSREQ:
		case USIF_RIS_TXLSREQ:
			return "LSREQ";
		case USIF_RIS_RXSREQ:
		case USIF_RIS_TXSREQ:
			return "SREQ";
		case USIF_RIS_RXLBREQ:
		case USIF_RIS_TXLBREQ:
			return "LBREQ";
		case USIF_RIS_RXBREQ:
		case USIF_RIS_TXBREQ:
			return "BREQ";
	}
	return "mixed";
}

static void check_requests(
	const char *name,
	const struct request_sequence *expected,
	const struct request_sequence *actual
) {
	printf("# %s:", name);
	for (uint32_t index = 0; index < actual->count; index++)
		printf(" %s", request_name(actual->requests[index]));
	printf(" (raw");
	for (uint32_t index = 0; index < actual->count; index++)
		printf(" %03lX", actual->requests[index]);
	printf(")\n");
	bool same = actual->count == expected->count &&
		memcmp(actual->requests, expected->requests, actual->count * sizeof(actual->requests[0])) == 0;
	test_check(name, same);
}

static void test_registers(void) {
	static const struct {
		const char *name;
		volatile uint32_t *reg;
		uint32_t mask;
	} REGISTERS[] = {
		{ "MODE_CFG writable bits", &USIF_MODE_CFG,
			USIF_MODE_CFG_SYNC | USIF_MODE_CFG_MA | USIF_MODE_CFG_RXEN | USIF_MODE_CFG_LB | USIF_MODE_CFG_ECH |
			USIF_MODE_CFG_TXINV | USIF_MODE_CFG_RXINV | USIF_MODE_CFG_SCFRC | USIF_MODE_CFG_SCPOL |
			USIF_MODE_CFG_SCPH | USIF_MODE_CFG_TXIDLE | USIF_MODE_CFG_ABEN | USIF_MODE_CFG_EQU },
		{ "PRTC_CFG writable bits", &USIF_PRTC_CFG,
			USIF_PRTC_CFG_CLEN | USIF_PRTC_CFG_HD | USIF_PRTC_CFG_PAR | USIF_PRTC_CFG_ODD | USIF_PRTC_CFG_ERRFE |
			USIF_PRTC_CFG_STP2 | USIF_PRTC_CFG_RESWKUP | USIF_PRTC_CFG_FCIEN | USIF_PRTC_CFG_FCOEN |
			USIF_PRTC_CFG_FCIINV | USIF_PRTC_CFG_FCOTL | USIF_PRTC_CFG_SLIPEN },
		{ "BC_CFG writable bits", &USIF_BC_CFG, USIF_BC_CFG_BCRV },
		{ "FDIV_CFG writable bits", &USIF_FDIV_CFG, USIF_FDIV_CFG_INC | USIF_FDIV_CFG_DEC },
		{ "ICTMO_CFG writable bits", &USIF_ICTMO_CFG, USIF_ICTMO_CFG_TMO },
		{ "TXFIFO_CFG writable bits", &USIF_TXFIFO_CFG,
			USIF_TXFIFO_CFG_TXBS | USIF_TXFIFO_CFG_TXFA | USIF_TXFIFO_CFG_TXFC },
		{ "RXFIFO_CFG writable bits", &USIF_RXFIFO_CFG,
			USIF_RXFIFO_CFG_RXBS | USIF_RXFIFO_CFG_RXFA | USIF_RXFIFO_CFG_RXFC },
		{ "ERRIRQSM writable bits", &USIF_ERRIRQSM, GENMASK(9, 0) },
		{ "PIRQSM writable bits", &USIF_PIRQSM, GENMASK(7, 0) },
		{ "IMSC writable bits", &USIF_IMSC,
			USIF_RX_REQUESTS | USIF_TX_REQUESTS | USIF_RIS_AB | USIF_GROUP_SUMMARIES },
		{ "DMAE writable bits", &USIF_DMAE, USIF_RX_REQUESTS | USIF_TX_REQUESTS },
	};

	USIF_RUNCTRL = 0;
	for (uint32_t index = 0; index < ARRAY_SIZE(REGISTERS); index++) {
		*REGISTERS[index].reg = 0xFFFFFFFF;
		test_eq_u32(REGISTERS[index].name, REGISTERS[index].mask, *REGISTERS[index].reg);
		*REGISTERS[index].reg = 0;
	}
	test_eq_u32("ICR reads as zero", 0, USIF_ICR);
	test_eq_u32("ISR reads as zero", 0, USIF_ISR);
	test_eq_u32("ERRIRQSC reads as zero", 0, USIF_ERRIRQSC);
	test_eq_u32("PIRQSC reads as zero", 0, USIF_PIRQSC);
}

static void test_interrupt_set_clear(void) {
	usif_configure(false, 0, 0, 0);
	uint32_t settable = USIF_RX_REQUESTS | USIF_TX_REQUESTS | USIF_RIS_AB;
	USIF_ISR = settable;
	test_eq_u32("ISR sets request and autobaud RIS bits", settable, USIF_RIS);
	test_eq_u32("MIS is empty with IMSC clear", 0, USIF_MIS);
	USIF_IMSC = USIF_RIS_TXSREQ | USIF_RIS_AB;
	test_eq_u32("MIS follows IMSC", USIF_RIS_TXSREQ | USIF_RIS_AB, USIF_MIS);
	USIF_ICR = settable;
	test_eq_u32("ICR clears request and autobaud RIS bits", 0, USIF_RIS);

	USIF_ISR = USIF_GROUP_SUMMARIES;
	test_eq_u32("ISR sets group summaries", USIF_GROUP_SUMMARIES, USIF_RIS);
	test_eq_u32("ISR summaries leave ERRIRQSS clear", 0, USIF_ERRIRQSS);
	test_eq_u32("ISR summaries leave PIRQSS clear", 0, USIF_PIRQSS);
	USIF_ICR = USIF_GROUP_SUMMARIES;
	test_eq_u32("ICR keeps group summaries", USIF_GROUP_SUMMARIES, USIF_RIS);
	USIF_ERRIRQSC = 0xFFFFFFFF;
	test_eq_u32("ERRIRQSC clears the error summary", USIF_RIS_PIRQ, USIF_RIS);
	USIF_PIRQSC = 0xFFFFFFFF;
	test_eq_u32("PIRQSC clears the protocol summary", 0, USIF_RIS);
}

static void test_protocol_group(void) {
	usif_configure(true, 0, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 4;
	USIF_TXD = 0x44332211;
	test_check("loopback packet ends", wait_for_pirq(USIF_PIRQSS_TXEND | USIF_PIRQSS_RXEND));
	test_eq_u32("PIRQSS is raw status", USIF_PIRQSS_TXEND | USIF_PIRQSS_RXEND, USIF_PIRQSS);
	test_eq_u32("masked protocol events stay out of RIS", 0, USIF_RIS & USIF_RIS_PIRQ);
	USIF_PIRQSM = USIF_PIRQSM_TXEND | USIF_PIRQSM_RXEND;
	test_eq_u32("PIRQSM raises the protocol summary", USIF_RIS_PIRQ, USIF_RIS & USIF_RIS_PIRQ);
	USIF_ICR = 0xFFFFFFFF;
	test_eq_u32("ICR keeps the protocol summary", USIF_RIS_PIRQ, USIF_RIS & USIF_RIS_PIRQ);
	USIF_PIRQSC = USIF_PIRQSC_TXEND;
	test_eq_u32("PIRQSC clears one event", USIF_PIRQSS_RXEND, USIF_PIRQSS);
	test_eq_u32("summary stays while an enabled event remains", USIF_RIS_PIRQ, USIF_RIS & USIF_RIS_PIRQ);
	USIF_PIRQSM = 0;
	test_eq_u32("PIRQSM clear keeps the latched summary", USIF_RIS_PIRQ, USIF_RIS & USIF_RIS_PIRQ);
	USIF_PIRQSC = USIF_PIRQSC_RXEND;
	test_eq_u32("last PIRQSC clears the summary", 0, USIF_RIS & USIF_RIS_PIRQ);
	test_eq_u32("loopback data", 0x44332211, USIF_RXD);
}

static void test_vic_lines(void) {
	static const uint32_t LINES[] = {
		VIC_USIF_RX_SINGLE_IRQ, VIC_USIF_RX_SINGLE_IRQ, VIC_USIF_RX_SINGLE_IRQ, VIC_USIF_RX_BURST_IRQ,
		VIC_USIF_TX_IRQ, VIC_USIF_TX_IRQ, VIC_USIF_TX_IRQ, VIC_USIF_TX_IRQ,
		VIC_USIF_AB_IRQ, VIC_USIF_ERR_IRQ, VIC_USIF_PIRQ_IRQ,
	};

	usif_configure(false, 0, 0, 0);
	cpu_enable_irq(false);
	VIC_IRQ_ACK = 1;
	VIC_IRQ_CON = 0;
	for (uint32_t bit = 0; bit < ARRAY_SIZE(LINES); bit++) {
		uint32_t routed = 0;
		uint32_t masked = 0;
		USIF_ISR = BIT(bit);
		for (uint32_t line = VIC_USIF_RX_SINGLE_IRQ; line <= VIC_USIF_PIRQ_IRQ; line++) {
			VIC_CON(line) = 1;
			if ((VIC_IRQ_CON & VIC_IRQ_CON_NUM) == line)
				masked |= BIT(line - VIC_USIF_RX_SINGLE_IRQ);
			USIF_IMSC = BIT(bit);
			if ((VIC_IRQ_CON & VIC_IRQ_CON_NUM) == line)
				routed |= BIT(line - VIC_USIF_RX_SINGLE_IRQ);
			USIF_IMSC = 0;
			VIC_CON(line) = 0;
		}
		printf("# RIS bit %lu -> VIC lines mask %02lX (expected line %lu)\n", bit, routed, LINES[bit]);
		test_eq_u32("RIS bit reaches only its VIC line", BIT(LINES[bit] - VIC_USIF_RX_SINGLE_IRQ), routed);
		test_eq_u32("IMSC gates the VIC line", 0, masked);
		USIF_ICR = BIT(bit);
		USIF_ERRIRQSC = 0xFFFFFFFF;
		USIF_PIRQSC = 0xFFFFFFFF;
	}
}

static void collect_tx_requests(uint32_t size, struct request_sequence *sequence) {
	sequence->count = 0;
	USIF_TPS_CTRL = size;
	uint32_t written_words = 0;
	while (sequence->count < USIF_MAX_REQUESTS) {
		uint32_t request = wait_for_request(USIF_TX_REQUESTS);
		if (request == 0)
			break;
		sequence->requests[sequence->count++] = request;
		uint32_t words = (request & USIF_BURST_REQUESTS) != 0 ? 4 : 1;
		for (uint32_t word = 0; word < words; word++)
			USIF_TXD = tx_word(written_words++);
		USIF_ICR = request;
		if ((request & USIF_TX_LAST_REQUESTS) != 0)
			break;
	}
}

static void test_tx_requests(void) {
	static const struct request_case CASES[] = {
		{ 8, { 2, { USIF_RIS_TXSREQ, USIF_RIS_TXLSREQ } } },
		{ 16, { 1, { USIF_RIS_TXLBREQ } } },
		{ 40, { 4, { USIF_RIS_TXBREQ, USIF_RIS_TXBREQ, USIF_RIS_TXSREQ, USIF_RIS_TXLSREQ } } },
	};
	uint32_t txfifo_cfg = USIF_TXFIFO_CFG_TXBS_4_WORD | USIF_TXFIFO_CFG_TXFA_1 | USIF_TXFIFO_CFG_TXFC;

	usif_configure(false, 0, txfifo_cfg, 0);
	USIF_TPS_CTRL = 8;
	stopwatch_usleep_wd(100);
	test_eq_u32("stopped interface raises no TX request", 0, USIF_RIS & USIF_TX_REQUESTS);
	test_eq_u32("stopped interface keeps TPS", 8, USIF_TPS_CTRL);

	for (uint32_t index = 0; index < ARRAY_SIZE(CASES); index++) {
		struct request_sequence sequence;
		usif_configure(false, 0, txfifo_cfg, 0);
		USIF_RUNCTRL = USIF_RUNCTRL_RUN;
		collect_tx_requests(CASES[index].size, &sequence);
		printf("# TX packet of %lu bytes, 4-word bursts\n", CASES[index].size);
		check_requests("TX request sequence", &CASES[index].expected, &sequence);
		test_check("TX packet ends", wait_for_pirq(USIF_PIRQSS_TXEND));
		test_eq_u32("TPS reads zero after the packet", 0, USIF_TPS_CTRL);
		test_eq_u32("TX FIFO drains", 0, USIF_TXFIFO_STAT);
	}
}

static void collect_rx_requests(uint32_t size, struct request_sequence *sequence, uint8_t *data) {
	uint32_t total_words = (size + 3) / 4;
	uint32_t written_words = 0;
	uint32_t read_words = 0;

	sequence->count = 0;
	USIF_MRPS_CTRL = size;
	USIF_TPS_CTRL = size;
	stopwatch_t start = stopwatch_get();
	while (sequence->count < USIF_MAX_REQUESTS && stopwatch_elapsed_ms(start) < USIF_TIMEOUT_MS) {
		if (written_words < total_words && (USIF_TXFIFO_STAT & USIF_TXFIFO_STAT_TXFFS) < USIF_FIFO_STAGES)
			USIF_TXD = tx_word(written_words++);
		uint32_t request = USIF_RIS & USIF_RX_REQUESTS;
		if (request == 0)
			continue;
		sequence->requests[sequence->count++] = request;
		uint32_t words = (request & USIF_BURST_REQUESTS) != 0 ? 4 : 1;
		for (uint32_t word = 0; word < words; word++) {
			uint32_t value = USIF_RXD;
			if (read_words < total_words)
				memcpy(&data[read_words * 4], &value, sizeof(value));
			read_words++;
		}
		USIF_ICR = request;
		if ((request & USIF_RX_LAST_REQUESTS) != 0)
			break;
	}
}

static void test_rx_requests(void) {
	static const struct request_case CASES[] = {
		{ 7, { 2, { USIF_RIS_RXSREQ, USIF_RIS_RXLSREQ } } },
		{ 16, { 1, { USIF_RIS_RXLBREQ } } },
		{ 40, { 4, { USIF_RIS_RXBREQ, USIF_RIS_RXBREQ, USIF_RIS_RXSREQ, USIF_RIS_RXLSREQ } } },
	};
	uint32_t rxfifo_cfg = USIF_RXFIFO_CFG_RXBS_4_WORD | USIF_RXFIFO_CFG_RXFA_1 | USIF_RXFIFO_CFG_RXFC;

	for (uint32_t index = 0; index < ARRAY_SIZE(CASES); index++) {
		struct request_sequence sequence;
		uint8_t data[USIF_MAX_PACKET] = { 0 };
		usif_configure(true, 0, 0, rxfifo_cfg);
		USIF_RUNCTRL = USIF_RUNCTRL_RUN;
		collect_rx_requests(CASES[index].size, &sequence, data);
		printf("# RX packet of %lu bytes, 4-word bursts, MRPS = packet size\n", CASES[index].size);
		check_requests("RX request sequence", &CASES[index].expected, &sequence);
		test_eq_memory("RX request data", tx_bytes, data, CASES[index].size);
		test_eq_u32("RPS reports the packet size", CASES[index].size, USIF_RPS_STAT);
		test_eq_u32("RX FIFO drains", 0, USIF_RXFIFO_STAT);
		test_eq_u32("no RX FIFO error", 0, USIF_ERRIRQSS);
	}
}

static void test_rx_packet_end(void) {
	usif_configure(true, 0, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 7;
	USIF_TXD = tx_word(0);
	USIF_TXD = tx_word(1);
	test_check("idle timeout ends the RX packet", wait_for_pirq(USIF_PIRQSS_RXEND));
	test_eq_u32("RPS counts the packet", 7, USIF_RPS_STAT);
	test_eq_u32("RX FIFO holds two words", 2, USIF_RXFIFO_STAT);
	test_eq_u32("RX burst request without flow control", USIF_RIS_RXBREQ, USIF_RIS & USIF_RX_REQUESTS);
	uint32_t data[2] = { USIF_RXD, USIF_RXD };
	test_eq_memory("RX packet data", tx_bytes, data, 7);
	test_eq_u32("RX FIFO empty after reads", 0, USIF_RXFIFO_STAT);
}

static void test_tx_fifo_status(void) {
	usif_configure(false, 63, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = USIF_FIFO_STAGES * 4;
	for (uint32_t word = 0; word < USIF_FIFO_STAGES; word++)
		USIF_TXD = tx_word(word);
	uint32_t status = USIF_TXFIFO_STAT;
	printf("# TX FIFO after %u fast writes: %lu stages, %s\n", USIF_FIFO_STAGES, status & USIF_TXFIFO_STAT_TXFFS,
		(status & USIF_TXFIFO_STAT_TXBUSY) != 0 ? "busy" : "idle");
	test_check("transmitter is busy", (status & USIF_TXFIFO_STAT_TXBUSY) != 0);
	test_eq_u32("TX FIFO counts every written word", USIF_FIFO_STAGES, status & USIF_TXFIFO_STAT_TXFFS);
	test_eq_u32("no TX FIFO overflow", 0, USIF_ERRIRQSS);
	test_eq_u32("RUNCTRL reads RUN", USIF_RUNCTRL_RUN, USIF_RUNCTRL);

	USIF_RUNCTRL = 0;
	test_eq_u32("stopping flushes the TX FIFO", 0, USIF_TXFIFO_STAT);
	test_eq_u32("stopped packet does not end", 0, USIF_PIRQSS & USIF_PIRQSS_TXEND);

	usif_configure(false, 0, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 8;
	USIF_TXD = tx_word(0);
	USIF_TXD = tx_word(1);
	test_check("packet ends at full speed", wait_for_pirq(USIF_PIRQSS_TXEND));
	test_eq_u32("finished transmitter is idle", 0, USIF_TXFIFO_STAT);
}

static void test_tx_resumes_after_clock_gap(void) {
	usif_configure(false, 63, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 8;
	USIF_TXD = tx_word(0);
	USIF_TXD = tx_word(1);
	uint32_t con3 = CGU_CON3;
	CGU_CON3 = (con3 & ~CGU_CON3_MMCI_CLKSEL) | CGU_CON3_MMCI_CLKSEL_DISABLE;
	stopwatch_usleep_wd(5000);
	CGU_CON3 = con3;
	test_check("TX packet ends after the kernel clock returns", wait_for_pirq(USIF_PIRQSS_TXEND));
	test_eq_u32("resumed transmitter drains", 0, USIF_TXFIFO_STAT);
}

static void test_rx_fifo_errors(void) {
	uint32_t words = USIF_FIFO_STAGES + 4;

	usif_configure(true, 0, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = words * 4;
	for (uint32_t word = 0; word < words; word++) {
		while ((USIF_TXFIFO_STAT & USIF_TXFIFO_STAT_TXFFS) >= USIF_FIFO_STAGES)
			test_watchdog_serve();
		USIF_TXD = tx_word(word);
	}
	test_check("overflowing packet ends", wait_for_pirq(USIF_PIRQSS_RXEND));
	test_eq_u32("RX FIFO holds eight words", USIF_FIFO_STAGES, USIF_RXFIFO_STAT);
	test_eq_u32("RX FIFO overflow", USIF_ERRIRQSS_RXFOFL, USIF_ERRIRQSS);
	USIF_ERRIRQSC = 0xFFFFFFFF;
	uint32_t data[USIF_FIFO_STAGES];
	for (uint32_t word = 0; word < USIF_FIFO_STAGES; word++)
		data[word] = USIF_RXD;
	test_eq_memory("RX FIFO keeps the oldest words", tx_bytes, data, sizeof(data));
	test_eq_u32("draining reports no error", 0, USIF_ERRIRQSS);
	(void) USIF_RXD;
	test_eq_u32("reading an empty RX FIFO underflows", USIF_ERRIRQSS_RXFUFL, USIF_ERRIRQSS);

	USIF_RUNCTRL = 0;
	USIF_ERRIRQSC = 0xFFFFFFFF;
	(void) USIF_RXD;
	test_eq_u32("stopped RX FIFO read does not underflow", 0, USIF_ERRIRQSS);
}

static void test_stop_flushes_rx(void) {
	usif_configure(true, 0, 0, 0);
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 8;
	USIF_TXD = tx_word(0);
	USIF_TXD = tx_word(1);
	test_check("loopback packet received", wait_for_pirq(USIF_PIRQSS_RXEND));
	USIF_RUNCTRL = 0;
	test_eq_u32("stopping flushes the RX FIFO", 0, USIF_RXFIFO_STAT);
	test_eq_u32("stopping keeps RPS of the last packet", 8, USIF_RPS_STAT);
}

static void test_modem_lines(void) {
	usif_configure(false, 0, 0, 0);
	USIF_MSS_SET = USIF_MSS_SET_FCO;
	test_eq_u32("MSS_SET asserts FCO", USIF_MSS_STAT_FCO, USIF_MSS_STAT & USIF_MSS_STAT_FCO);
	test_eq_u32("MSS_SET reads as zero", 0, USIF_MSS_SET);
	USIF_MSS_CLR = USIF_MSS_CLR_FCO;
	test_eq_u32("MSS_CLR deasserts FCO", 0, USIF_MSS_STAT & USIF_MSS_STAT_FCO);
	test_eq_u32("idle CTS input reads inactive", 0, USIF_MSS_STAT & USIF_MSS_STAT_FCI);
	USIF_PRTC_CFG |= USIF_PRTC_CFG_FCIINV;
	test_eq_u32("FCIINV inverts the CTS input", USIF_MSS_STAT_FCI, USIF_MSS_STAT & USIF_MSS_STAT_FCI);

	usif_configure(false, 0, 0, 0);
	USIF_PRTC_CFG |= USIF_PRTC_CFG_FCIEN;
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 4;
	USIF_TXD = tx_word(0);
	stopwatch_usleep_wd(1000);
	test_eq_u32("inactive CTS holds the TX FIFO", 1, USIF_TXFIFO_STAT & USIF_TXFIFO_STAT_TXFFS);
	test_eq_u32("inactive CTS holds the packet", 0, USIF_PIRQSS & USIF_PIRQSS_TXEND);

	usif_configure(false, 0, 0, 0);
	USIF_PRTC_CFG |= USIF_PRTC_CFG_FCIEN | USIF_PRTC_CFG_FCIINV;
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 4;
	USIF_TXD = tx_word(0);
	test_check("active CTS lets the packet end", wait_for_pirq(USIF_PIRQSS_TXEND));
}

static void run_dma_loopback(uint32_t size, bool peripheral_flow) {
	uint32_t channels = BIT(DMA_RX_CHANNEL) | BIT(DMA_TX_CHANNEL);
	uint32_t words = (size + 3) / 4;
	uint32_t txfifo_cfg = USIF_TXFIFO_CFG_TXBS_4_WORD | USIF_TXFIFO_CFG_TXFA_1 |
		(peripheral_flow ? USIF_TXFIFO_CFG_TXFC : 0);
	uint32_t rxfifo_cfg = USIF_RXFIFO_CFG_RXBS_4_WORD | USIF_RXFIFO_CFG_RXFA_1 |
		(peripheral_flow ? USIF_RXFIFO_CFG_RXFC : 0);
	uint32_t tx_control = peripheral_flow ? DMAC_CH_CONTROL_S_WIDTH_BYTE : (DMAC_CH_CONTROL_S_WIDTH_DWORD | words);
	uint32_t rx_transfer_size = peripheral_flow ? 0 : words;

	usif_configure(true, 0, txfifo_cfg, rxfifo_cfg);
	memset(rx_words, 0, sizeof(rx_words));
	cpu_enable_irq(false);
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DMAC_TC_CLEAR = channels;
	DMAC_ERR_CLEAR = channels;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	dmac_request_select(USIF_DMA_RX_REQUEST, USIF_DMA_RX_SELECT);
	dmac_request_select(USIF_DMA_TX_REQUEST, USIF_DMA_TX_SELECT);

	DMAC_CH_SRC_ADDR(DMA_RX_CHANNEL) = (uint32_t) &USIF_RXD;
	DMAC_CH_DST_ADDR(DMA_RX_CHANNEL) = (uint32_t) rx_words;
	DMAC_CH_CONTROL(DMA_RX_CHANNEL) = (
		rx_transfer_size | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | USIF_DMA_RX_AHB |
		DMAC_CH_CONTROL_D_AHB1 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = (
		(USIF_DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		(peripheral_flow ? DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER : DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM) |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE
	);

	DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL) = (uint32_t) tx_bytes;
	DMAC_CH_DST_ADDR(DMA_TX_CHANNEL) = (uint32_t) &USIF_TXD;
	DMAC_CH_CONTROL(DMA_TX_CHANNEL) = (
		tx_control | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB1 | USIF_DMA_TX_AHB |
		DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(USIF_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		(peripheral_flow ? DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER : DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER) |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE
	);

	USIF_IMSC = USIF_RX_REQUESTS | USIF_TX_REQUESTS;
	USIF_DMAE = USIF_RX_REQUESTS | USIF_TX_REQUESTS;
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_MRPS_CTRL = peripheral_flow ? size : 0;
	USIF_TPS_CTRL = size;
	stopwatch_t start = stopwatch_get();
	while ((DMAC_RAW_TC_STATUS & channels) != channels &&
		(DMAC_RAW_ERR_STATUS & channels) == 0 &&
		stopwatch_elapsed_ms(start) < USIF_TIMEOUT_MS)
		test_watchdog_serve();

	uint32_t tc_status = DMAC_RAW_TC_STATUS & channels;
	uint32_t err_status = DMAC_RAW_ERR_STATUS & channels;
	uint32_t packet_events = USIF_PIRQSS_TXEND | (peripheral_flow ? 0 : USIF_PIRQSS_RXEND);
	bool packet_ended = wait_for_pirq(packet_events);
	printf("# DMA %s flow, %lu bytes: TC %02lX, error %02lX, TPS %lu, TX FIFO %08lX, RX FIFO %lu, RPS %lu, "
		"PIRQSS %02lX\n", peripheral_flow ? "peripheral" : "DMAC", size, tc_status, err_status, USIF_TPS_CTRL,
		USIF_TXFIFO_STAT, USIF_RXFIFO_STAT, USIF_RPS_STAT, USIF_PIRQSS);
	USIF_DMAE = 0;
	DMAC_CH_CONFIG(DMA_RX_CHANNEL) = 0;
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;

	test_eq_u32("both DMA channels reach terminal count", channels, tc_status);
	test_eq_u32("DMA channels report no error", 0, err_status);
	test_check("DMA TX packet ends", packet_ended);
	test_eq_u32("RX packet end follows the end condition", packet_events, USIF_PIRQSS);
	test_eq_memory("DMA loopback data", tx_bytes, rx_words, size);
	test_eq_u32("DMA leaves TPS zero", 0, USIF_TPS_CTRL);
	test_eq_u32("DMA leaves the transmitter idle", 0, USIF_TXFIFO_STAT);
	test_eq_u32("DMA drains the RX FIFO", 0, USIF_RXFIFO_STAT);
	test_eq_u32("DMA RPS reports the packet", size, USIF_RPS_STAT);
}

static void test_dma_requires_imsc(void) {
	usif_configure(false, 0, USIF_TXFIFO_CFG_TXBS_4_WORD | USIF_TXFIFO_CFG_TXFA_1 | USIF_TXFIFO_CFG_TXFC, 0);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(DMA_TX_CHANNEL);
	DMAC_ERR_CLEAR = BIT(DMA_TX_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	dmac_request_select(USIF_DMA_TX_REQUEST, USIF_DMA_TX_SELECT);
	DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL) = (uint32_t) tx_bytes;
	DMAC_CH_DST_ADDR(DMA_TX_CHANNEL) = (uint32_t) &USIF_TXD;
	DMAC_CH_CONTROL(DMA_TX_CHANNEL) = (
		DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
		DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB1 | USIF_DMA_TX_AHB | DMAC_CH_CONTROL_SI |
		DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(USIF_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER |
		DMAC_CH_CONFIG_INT_MASK_ERR | DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE
	);
	USIF_DMAE = USIF_TX_REQUESTS;
	USIF_RUNCTRL = USIF_RUNCTRL_RUN;
	USIF_TPS_CTRL = 8;
	stopwatch_usleep_wd(1000);
	test_eq_u32("DMAE without IMSC moves no data", 0, USIF_TXFIFO_STAT);
	test_eq_u32("DMAE without IMSC reaches no terminal count", 0, DMAC_RAW_TC_STATUS & BIT(DMA_TX_CHANNEL));
	USIF_IMSC = USIF_TX_REQUESTS;
	test_check("IMSC releases the pending request", wait_for_pirq(USIF_PIRQSS_TXEND));
	test_eq_u32("released DMA reaches terminal count",
		BIT(DMA_TX_CHANNEL), DMAC_RAW_TC_STATUS & BIT(DMA_TX_CHANNEL));
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
}

static void test_dma_peripheral_flow(void) {
	static const uint32_t SIZES[] = { 8, 16, 40, 61 };

	for (uint32_t index = 0; index < ARRAY_SIZE(SIZES); index++)
		run_dma_loopback(SIZES[index], true);
}

static void test_dma_controller_flow(void) {
	static const uint32_t SIZES[] = { 16, 40 };

	for (uint32_t index = 0; index < ARRAY_SIZE(SIZES); index++)
		run_dma_loopback(SIZES[index], false);
}

static void test_dma_back_to_back(void) {
	run_dma_loopback(8, true);
	run_dma_loopback(8, true);
}

static void usif_enable(void) {
	USIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	CGU_CON3 = (CGU_CON3 & ~(CGU_CON3_MMCI_CLKSEL | CGU_CON3_MMCI_CLKDIV)) |
		CGU_CON3_MMCI_CLKSEL_OSC | CGU_CON3_MMCI_CLKDIV_DIV1;
	stopwatch_usleep_wd(100);
	fill_tx_bytes();
}

int usif_test(void) {
	stopwatch_init();
	test_start("USIF");
	usif_enable();
	/* No SCU_RST_REQ bit resets USIF and it keeps its registers across boots, so reset values are unknown. */
	test_module_id("module ID", 0xF051C000, USIF_ID);

	test_category("Registers");
	test_registers();
	test_category("Interrupts");
	test_interrupt_set_clear();
	test_protocol_group();
	test_vic_lines();
	test_category("FIFO requests");
	test_tx_requests();
	test_rx_requests();
	test_category("FIFO data");
	test_rx_packet_end();
	test_tx_fifo_status();
	test_tx_resumes_after_clock_gap();
	test_rx_fifo_errors();
	test_stop_flushes_rx();
	test_category("Modem lines");
	test_modem_lines();
	return test_finish();
}

int usif_dma_test(void) {
	stopwatch_init();
	test_start("USIF DMA");
	usif_enable();
	test_category("Request gating");
	test_dma_requires_imsc();
	test_category("Peripheral flow");
	test_dma_peripheral_flow();
	test_category("DMAC flow");
	test_dma_controller_flow();
	test_category("Back to back");
	test_dma_back_to_back();
	return test_finish();
}
