#include <pmb887x.h>

#include "test.h"

#define DMA_CHANNEL 7

static uint32_t source[16] __attribute__((aligned(16)));
static volatile uint32_t destination[ARRAY_SIZE(source)] __attribute__((aligned(16)));

static void start_transfer(void) {
	DMAC_CH_CONFIG(DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(DMA_CHANNEL);
	DMAC_CH_SRC_ADDR(DMA_CHANNEL) = (uint32_t) source;
	DMAC_CH_DST_ADDR(DMA_CHANNEL) = (uint32_t) destination;
	DMAC_CH_LLI(DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(DMA_CHANNEL) = ARRAY_SIZE(source) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_4 | DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD |
		DMAC_CH_CONTROL_S_AHB2 | DMAC_CH_CONTROL_D_AHB2 | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_DI |
		DMAC_CH_CONTROL_I;
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_CH_CONFIG(DMA_CHANNEL) = DMAC_CH_CONFIG_FLOW_CTRL_MEM2MEM | DMAC_CH_CONFIG_ENABLE;
}

int main(void) {
	test_start("CGU DMA clock test");

	for (uint32_t index = 0; index < ARRAY_SIZE(source); index++) {
		source[index] = 0x12340000 | index;
		destination[index] = 0;
	}

	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(DMA_CHANNEL) = 0;
	uint32_t initial_control = DMAC_CH_CONTROL(DMA_CHANNEL);
	CGU_CON3 |= CGU_CON3_DMA_CLK_DISABLE;
	start_transfer();
	stopwatch_usleep_wd(1000);
	bool gated = (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0;
	for (uint32_t index = 0; index < ARRAY_SIZE(destination); index++)
		gated = gated && destination[index] == 0;
	bool config_ignored = DMAC_CONFIG == 0 && DMAC_CH_CONFIG(DMA_CHANNEL) == 0 &&
		DMAC_CH_CONTROL(DMA_CHANNEL) == initial_control;

	CGU_CON3 &= ~CGU_CON3_DMA_CLK_DISABLE;
	start_transfer();
	stopwatch_t start = stopwatch_get();
	while ((DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) == 0 && stopwatch_elapsed_ms(start) < 20)
		test_watchdog_serve();
	bool completed = (DMAC_RAW_TC_STATUS & BIT(DMA_CHANNEL)) != 0;
	bool data_matches = true;
	for (uint32_t index = 0; index < ARRAY_SIZE(source); index++)
		data_matches = data_matches && destination[index] == source[index];

	test_check("DMA transfer does not run while its clock is disabled", gated);
	test_check("DMA configuration writes are ignored while its clock is disabled", config_ignored);
	test_check("DMA transfer completes after its clock is enabled", completed && data_matches);
	return test_finish();
}
