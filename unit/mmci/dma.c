#include <pmb887x.h>

#include "dmac.h"
#include "test.h"

#define MCI_WRITE_GAP_US 10
#define MCI_SLOW_CLKDIV 255
#define MCI_DATA_TIMEOUT_CYCLES 0x00FFFFFF
#define MCI_DMA_CHANNEL 0
#define MCI_DMA_WAIT_US 200
#define MCI_TX_WORDS 16
#define MCI_TX_BLOCK_SIZE_LOG2 6
#define MCI_FIFO_WINDOW_BASE 0xF7304000
#define MCI_FIFO_WINDOW_SIZE 0x4000

#ifdef PMB8876
static uint32_t tx_source[MCI_TX_WORDS];

static void mci_configure(void) {
	MMCI_CLC = 1 << MOD_CLC_RMC_SHIFT;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_CLOCK = MCI_CLOCK_ENABLE | (MCI_SLOW_CLKDIV << MCI_CLOCK_CLKDIV_SHIFT);
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
}

static void dmac_start_tx(uint32_t request, uint32_t select) {
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(MCI_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(MCI_DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(MCI_DMA_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;
	dmac_request_select(request, select);

	DMAC_CH_SRC_ADDR(MCI_DMA_CHANNEL) = (uint32_t) tx_source;
	DMAC_CH_DST_ADDR(MCI_DMA_CHANNEL) = (uint32_t) &MCI_FIFO(0);
	DMAC_CH_LLI(MCI_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(MCI_DMA_CHANNEL) = (
		MCI_TX_WORDS | DMAC_CH_CONTROL_SB_SIZE_SZ_8 | DMAC_CH_CONTROL_DB_SIZE_SZ_8 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD |
		DMAC_CH_CONTROL_S_AHB1 | MCI_DMA_TX_AHB | DMAC_CH_CONTROL_SI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(MCI_DMA_CHANNEL) = (
		(request << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER_PER |
		DMAC_CH_CONFIG_ENABLE
	);
}

static void mci_start_tx(void) {
	MCI_DATACTRL = 0;
	MCI_DATATIMER = MCI_DATA_TIMEOUT_CYCLES;
	MCI_DATALENGTH = MCI_TX_WORDS * 4;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_DATACTRL = MCI_DATACTRL_ENABLE | MCI_DATACTRL_DIRECTION_WRITE | MCI_DATACTRL_MODE_BLOCK |
		MCI_DATACTRL_DMAENABLE | (MCI_TX_BLOCK_SIZE_LOG2 << MCI_DATACTRL_BLOCKSIZE_SHIFT);
}

static void test_tx_request(uint32_t request, uint32_t select, bool is_routed) {
	char category[48];
	snprintf(category, sizeof(category), "Transmit DMA on request %lu, select %lu", request, select);
	test_category(category);

	dmac_start_tx(request, select);
	mci_start_tx();
	stopwatch_usleep_wd(MCI_DMA_WAIT_US);
	uint32_t status = MCI_STATUS;
	uint32_t fifo_count = MCI_FIFOCNT;
	bool is_channel_enabled = (DMAC_CH_CONFIG(MCI_DMA_CHANNEL) & DMAC_CH_CONFIG_ENABLE) != 0;
	bool is_complete = (DMAC_RAW_TC_STATUS & BIT(MCI_DMA_CHANNEL)) != 0;
	MCI_DATACTRL = 0;
	DMAC_CH_CONFIG(MCI_DMA_CHANNEL) = 0;

	const char *fifo_state = (status & MCI_STATUS_TXFIFOFULL) != 0 ? "full" : "not full";
	const char *channel_state = is_complete ? "completed" : is_channel_enabled ? "waiting" : "stopped";
	printf("# FIFOCNT=%lu, FIFO %s, channel %s\n", fifo_count, fifo_state, channel_state);
	if (is_routed) {
		test_eq_u32("DMA fills the transmit FIFO", MCI_STATUS_TXFIFOFULL, status & MCI_STATUS_TXFIFOFULL);
		test_check("last burst request completes the channel", is_complete && !is_channel_enabled);
		test_check("FIFOCNT counts words still to be sent to the card", fifo_count >= MCI_TX_WORDS - 1);
	} else {
		test_eq_u32("transmit FIFO stays empty", MCI_STATUS_TXFIFOEMPTY, status & MCI_STATUS_TXFIFOEMPTY);
		test_check("channel keeps waiting for a request", is_channel_enabled && !is_complete);
	}
}

static void test_tx_window(uint32_t address) {
	char category[48];
	snprintf(category, sizeof(category), "Transmit FIFO window at %08lX", address);
	test_category(category);

	MCI_DATACTRL = 0;
	MCI_DATATIMER = MCI_DATA_TIMEOUT_CYCLES;
	MCI_DATALENGTH = MCI_TX_WORDS * 4;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	MCI_DATACTRL = MCI_DATACTRL_ENABLE | MCI_DATACTRL_DIRECTION_WRITE | MCI_DATACTRL_MODE_BLOCK |
		(MCI_TX_BLOCK_SIZE_LOG2 << MCI_DATACTRL_BLOCKSIZE_SHIFT);

	volatile uint32_t *window = (volatile uint32_t *) address;
	for (size_t i = 0; i < MCI_TX_WORDS; i++)
		window[i] = tx_source[i];
	uint32_t status = MCI_STATUS;
	uint32_t fifo_count = MCI_FIFOCNT;
	MCI_DATACTRL = 0;

	printf("# STATUS=%08lX FIFOCNT=%lu\n", status, fifo_count);
	test_eq_u32("window writes fill the transmit FIFO", MCI_STATUS_TXFIFOFULL, status & MCI_STATUS_TXFIFOFULL);
}
#endif

int main(void) {
	test_start("MMCI DMA test");

#ifdef PMB8876
	for (size_t i = 0; i < MCI_TX_WORDS; i++)
		tx_source[i] = 0x01020304 * (i + 1);
	mci_configure();
	test_tx_request(MCI_DMA_TX_REQUEST, MCI_DMA_TX_SELECT, true);
	test_tx_request(MCI_DMA_RX_REQUEST, MCI_DMA_RX_SELECT, true);
	test_tx_request(MCI_DMA_TX_REQUEST, !MCI_DMA_TX_SELECT, false);
	test_tx_request(MCI_DMA_RX_REQUEST, !MCI_DMA_RX_SELECT, false);
	test_tx_window(MCI_FIFO_WINDOW_BASE);
	test_tx_window(MCI_FIFO_WINDOW_BASE + MCI_FIFO_WINDOW_SIZE - (MCI_TX_WORDS * 4));
#else
	test_skip("MMCI DMA", "PMB8876 only");
#endif
	return test_finish();
}
