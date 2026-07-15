#include <pmb887x.h>

#include "test.h"

#ifdef PMB8876

#define SIM_DMA_CHANNEL 0
#define SIM_DMA_REQUEST 8
#define SIM_DMA_CHANNEL_MASK BIT(SIM_DMA_CHANNEL)
#define SIM_DMA_TIMEOUT_MS 100

static uint8_t dma_destination __attribute__((aligned(4)));

static bool wait_for_dma(void) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < SIM_DMA_TIMEOUT_MS) {
		if (DMAC_RAW_TC_STATUS & SIM_DMA_CHANNEL_MASK)
			return true;
		if (DMAC_RAW_ERR_STATUS & SIM_DMA_CHANNEL_MASK)
			return false;
		test_watchdog_serve();
	}

	return false;
}

static void reset_dma_channel(void) {
	SIM_DMAE = 0;
	SIM_ICR = SIM_ICR_OK;
	SIM_CON = 0;
	SIM_INS = 0;
	SIM_P3 = 0;
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = SIM_DMA_CHANNEL_MASK;
	DMAC_ERR_CLEAR = SIM_DMA_CHANNEL_MASK;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;
}

static void configure_per2mem(void) {
	DMAC_CH_SRC_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &SIM_RXB;
	DMAC_CH_DST_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &dma_destination;
	DMAC_CH_LLI(SIM_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(SIM_DMA_CHANNEL) = (
		1 | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = (
		(SIM_DMA_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM | DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE
	);
}

static void test_dma_gate(void) {
	reset_dma_channel();
	dma_destination = 0xA5;
	configure_per2mem();
	SIM_ISR = SIM_ISR_OK;

	stopwatch_t start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < 5)
		test_watchdog_serve();
	test_eq_u32("DMAE=0 suppresses SIM DMA request", 0, DMAC_RAW_TC_STATUS & SIM_DMA_CHANNEL_MASK);
	test_eq_u32("disabled request leaves memory unchanged", 0xA5, dma_destination);

	SIM_DMAE = SIM_DMAE_OK;
	start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < 5)
		test_watchdog_serve();
	test_eq_u32("DMAE does not replay an old OK event", 0, DMAC_RAW_TC_STATUS & SIM_DMA_CHANNEL_MASK);
	test_eq_u32("old event still leaves memory unchanged", 0xA5, dma_destination);
	SIM_ICR = SIM_ICR_OK;
	SIM_ISR = SIM_ISR_OK;
	test_check("new OK event completes PER2MEM DMA", wait_for_dma());
	test_eq_u32("PER2MEM DMA reads RXB", SIM_RXB, dma_destination);
	test_eq_u32("PER2MEM DMA has no bus error", 0, DMAC_RAW_ERR_STATUS & SIM_DMA_CHANNEL_MASK);
}

static void test_per2mem(void) {
	reset_dma_channel();
	dma_destination = 0xA5;
	SIM_INS = SIM_INS_INSDIR | 0xC0;
	SIM_P3 = 1;
	SIM_CON = SIM_CON_SIMT0;
	configure_per2mem();
	SIM_DMAE = SIM_DMAE_OK;
	SIM_ISR = SIM_ISR_OK;

	test_check("OK event completes PER2MEM DMA", wait_for_dma());
	test_eq_u32("PER2MEM DMA reads RXB", SIM_RXB, dma_destination);
	test_eq_u32("PER2MEM DMA has no bus error", 0, DMAC_RAW_ERR_STATUS & SIM_DMA_CHANNEL_MASK);
}

int main(void) {
	test_start("SIM DMA");

	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, SIM_CLC);
	SIM_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_module_clock("CLC enables module", SIM_CLC);
	test_eq_u32("DMAE reset value", 0, SIM_DMAE);

	test_category("DMA request gating");
	test_dma_gate();
	test_category("PER2MEM request");
	test_per2mem();

	reset_dma_channel();
	return test_finish();
}

#else

int main(void) {
	test_start("SIM DMA");
	test_skip("SIM DMA", "is only available on PMB8876");

	return test_finish();
}

#endif
