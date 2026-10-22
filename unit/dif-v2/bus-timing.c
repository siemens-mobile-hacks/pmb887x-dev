#include <pmb887x.h>

#include "bus-timing.h"
#include "dmac.h"
#include "test.h"

#ifdef PMB8876

#define DIF_TIMEOUT_MS 500
#define DMA_TX_CHANNEL 1
#define LONG_WORDS 4092
#define SHORT_WORDS 1020
/* Internal SRAM: 96 KB at 0x00080000, unused above the linked .sram section. */
#define SRAM_END 0x00098000

/*
 * Every measurement streams the same DMA blit twice, LONG_WORDS and SHORT_WORDS 32-bit TXD writes, with
 * no chip select active. The difference between the two removes DMA setup and FIFO drain latency, so
 * the slope is the steady-state parallel bus time per byte. Internal SRAM feeds DMAC faster than
 * external RAM.
 */
extern uint32_t _sram_end;

static uint32_t extram_tx[LONG_WORDS] __attribute__((aligned(16)));
static uint32_t *sram_tx;

const struct dif_lane_config DIF_LANE_1x8 = {
	"1x8 byte per 8-bit lane",
	DIF_CON_BM_8,
	DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_1,
	DIF_CSREG_BSCONF_1x8BIT,
};

const struct dif_lane_config DIF_LANE_2x8 = {
	"2x8 bytes per 16-bit lane",
	DIF_CON_BM_16,
	DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_2,
	DIF_CSREG_BSCONF_2x8BIT,
};

const struct dif_lane_config DIF_LANE_4x8 = {
	"4x8 bytes per 32-bit lane",
	DIF_CON_BM_8,
	DIF_TXFIFO_CFG_TXBS_8_WORD | DIF_TXFIFO_CFG_TXFA_4,
	DIF_CSREG_BSCONF_4x8BIT,
};

bool dif_bus_timing_init(void) {
	sram_tx = (uint32_t *) (((uint32_t) &_sram_end + 15) & ~15U);
	if ((uint32_t) (sram_tx + LONG_WORDS) > SRAM_END)
		return false;

	for (uint32_t word = 0; word < LONG_WORDS; word++) {
		extram_tx[word] = 0xE007E007;
		sram_tx[word] = 0xE007E007;
	}
	DIF_CLC = 1 << MOD_CLC_RMC_SHIFT;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	dmac_request_select(DIF_DMA_TX_REQUEST, DIF_DMA_TX_SELECT);
	return true;
}

uint32_t dif_lcdtim1(uint32_t addr_delay, uint32_t access_cycle, uint32_t data_delay) {
	return (
		addr_delay << DIF_LCDTIM1_ADDRDELAY_SHIFT |
		access_cycle << DIF_LCDTIM1_ACCESSCYCLE_SHIFT |
		data_delay << DIF_LCDTIM1_DATADELAY_SHIFT
	);
}

uint32_t dif_lcdtim2(uint32_t cs_act, uint32_t cs_deact, uint32_t wr_act, uint32_t wr_deact) {
	return (
		cs_act << DIF_LCDTIM2_CSACT_SHIFT |
		cs_deact << DIF_LCDTIM2_CSDEACT_SHIFT |
		wr_act << DIF_LCDTIM2_WRRDACT_SHIFT |
		wr_deact << DIF_LCDTIM2_WRRDDEACT_SHIFT
	);
}

static bool wait_until_idle(void) {
	stopwatch_t start = stopwatch_get();

	while ((DIF_STAT & DIF_STAT_BSY) != 0 && stopwatch_elapsed_ms(start) < DIF_TIMEOUT_MS)
		test_watchdog_serve();

	return (DIF_STAT & DIF_STAT_BSY) == 0;
}

static void configure_dif(const struct dif_lane_config *lane, uint32_t clc, uint32_t tim1, uint32_t tim2) {
	DIF_RUNCTRL = 0;
	DIF_CLC = clc;
	DIF_CON = lane->con;
	DIF_PERREG = DIF_PERREG_DIFPERMODE_PARALLEL;
	DIF_CSREG = lane->bsconf;
	DIF_PBCCON = 0;
	DIF_BCSEL0 = 0;
	DIF_BCSEL1 = 0;
	DIF_RXFIFO_CFG = DIF_RXFIFO_CFG_RXBS_4_WORD | DIF_RXFIFO_CFG_RXFA_1;
	DIF_TXFIFO_CFG = lane->txfifo_cfg;
	DIF_LCDTIM1 = tim1;
	DIF_LCDTIM2 = tim2;
	DIF_IMSC = DIF_IMSC_TXBREQ;
	DIF_DMAE = DIF_DMAE_TXBREQ;
	DIF_ICR = DIF_ICR_TXLSREQ | DIF_ICR_TXSREQ | DIF_ICR_TXLBREQ | DIF_ICR_TXBREQ | DIF_ICR_ERR;
	DIF_ERRIRQSC = DIF_ERRIRQSS;
	DIF_RUNCTRL = DIF_RUNCTRL_RUN;
}

/* Returns the time from the first DMA request until the parallel bus is idle, or 0 on failure. */
static uint32_t blit(const uint32_t *source, uint32_t words, struct dif_bus_timing *timing) {
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(DMA_TX_CHANNEL);
	DMAC_ERR_CLEAR = BIT(DMA_TX_CHANNEL);
	DMAC_CH_SRC_ADDR(DMA_TX_CHANNEL) = (uint32_t) source;
	DMAC_CH_DST_ADDR(DMA_TX_CHANNEL) = (uint32_t) &DIF_TXD;
	DMAC_CH_LLI(DMA_TX_CHANNEL) = 0;
	DMAC_CH_CONTROL(DMA_TX_CHANNEL) = (
		words | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD | DMAC_CH_CONTROL_S_AHB1 |
		DIF_DMA_TX_AHB | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = (
		(DIF_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER |
		DMAC_CH_CONFIG_ENABLE
	);

	stopwatch_t start = stopwatch_get();
	DIF_ISR = DIF_ISR_TXBREQ;
	while ((DMAC_RAW_TC_STATUS & BIT(DMA_TX_CHANNEL)) == 0 &&
		(DMAC_RAW_ERR_STATUS & BIT(DMA_TX_CHANNEL)) == 0 && stopwatch_elapsed_ms(start) < DIF_TIMEOUT_MS)
		test_watchdog_serve();
	bool idle = (DMAC_RAW_TC_STATUS & BIT(DMA_TX_CHANNEL)) != 0 && wait_until_idle();
	stopwatch_t elapsed = stopwatch_elapsed(start);
	bool errors = (DMAC_RAW_ERR_STATUS & BIT(DMA_TX_CHANNEL)) != 0 || (DIF_ERRIRQSS & DIF_ERRIRQSS_TXFOFL) != 0;
	bool ok = idle && !errors && DIF_TXFFS_STAT == 0;

	if (!ok && timing->failed_words == 0) {
		timing->failed_words = words;
		timing->failed_dma_remaining = DMAC_CH_CONTROL(DMA_TX_CHANNEL) & DMAC_CH_CONTROL_TRANSFER_SIZE;
		timing->failed_stat = DIF_STAT;
		timing->failed_txffs = DIF_TXFFS_STAT;
		timing->failed_errirqss = DIF_ERRIRQSS;
	}
	DMAC_CH_CONFIG(DMA_TX_CHANNEL) = 0;
	DIF_RUNCTRL = 0;
	return ok ? (uint32_t) (elapsed * 1000000000ULL / stopwatch_ticks_per_s()) : 0;
}

struct dif_bus_timing dif_bus_timing_measure(const struct dif_lane_config *lane, enum dif_blit_source source,
	uint32_t clc, uint32_t tim1, uint32_t tim2) {
	struct dif_bus_timing timing = { 0 };
	const uint32_t *buffer = source == DIF_BLIT_SRAM ? sram_tx : extram_tx;

	configure_dif(lane, clc, tim1, tim2);
	timing.long_ns = blit(buffer, LONG_WORDS, &timing);
	configure_dif(lane, clc, tim1, tim2);
	timing.short_ns = blit(buffer, SHORT_WORDS, &timing);
	timing.ok = timing.long_ns > timing.short_ns && timing.short_ns != 0;
	if (timing.ok) {
		uint32_t bytes = (LONG_WORDS - SHORT_WORDS) * 4;
		timing.byte_ns100 = (uint32_t) ((uint64_t) (timing.long_ns - timing.short_ns) * 100 / bytes);
	}

	return timing;
}

void dif_bus_timing_print_failure(const struct dif_bus_timing *timing) {
	if (timing->failed_words == 0)
		return;
	printf("# blit of %u words did not complete: DMA words left=%u, DIF STAT=%08X TX FIFO stages=%u "
		"ERRIRQSS=%08X\n",
		(unsigned int) timing->failed_words, (unsigned int) timing->failed_dma_remaining,
		(unsigned int) timing->failed_stat, (unsigned int) timing->failed_txffs,
		(unsigned int) timing->failed_errirqss);
}

#endif
