#include <pmb887x.h>

#include "dmac.h"
#include "test.h"

#define MCI_WRITE_GAP_US 10
#define MCI_COMMAND_WAIT_MS 100
#define MCI_DATA_WAIT_MS 1000
#define MCI_IDENTIFICATION_CLKDIV 4
#define MCI_TRANSFER_CLKDIV 0
#define MCI_DATA_TIMEOUT_CYCLES 0x00FFFFFF
#define MCI_FIFO_HALF_WORDS 8
#define MCI_DMA_CHANNEL 0

#define SD_POWER_RAMP_MS 10
#define SD_INITIALIZATION_CLOCKS_MS 1
#define SD_READY_WAIT_MS 1000
#define SD_BLOCK_SIZE 512
#define SD_BLOCK_SIZE_LOG2 9
#define SD_CMD8_CHECK_PATTERN 0x1AA
#define SD_CMD8_ECHO_MASK 0xFFF
#define SD_OCR_VOLTAGE_WINDOW 0x00FF8000
#define SD_OCR_HCS BIT(30)
#define SD_OCR_BUSY BIT(31)
#define SD_R1_APP_CMD BIT(5)
#define SD_R1_CURRENT_STATE GENMASK(12, 9)
#define SD_R1_CURRENT_STATE_SHIFT 9
#define SD_RESPONSE_RESERVED_INDEX 0x3F
#define SD_MBR_SIGNATURE_OFFSET 510

#define MCI_COMMAND_STATUS ( \
	MCI_STATUS_CMDCRCFAIL | \
	MCI_STATUS_CMDTIMEOUT | \
	MCI_STATUS_CMDRESPEND | \
	MCI_STATUS_CMDSENT \
)
#define MCI_COMMAND_CLEAR ( \
	MCI_CLEAR_CMDCRCFAILCLR | \
	MCI_CLEAR_CMDTIMEOUTCLR | \
	MCI_CLEAR_CMDRESPENDCLR | \
	MCI_CLEAR_CMDSENTCLR \
)
#define MCI_DATA_ERRORS ( \
	MCI_STATUS_DATACRCFAIL | \
	MCI_STATUS_DATATIMEOUT | \
	MCI_STATUS_RXOVERRUN | \
	MCI_STATUS_STARTBITERR \
)
#define MCI_DATA_CLEAR ( \
	MCI_CLEAR_DATACRCFAILCLR | \
	MCI_CLEAR_DATATIMEOUTCLR | \
	MCI_CLEAR_RXOVERRUNCLR | \
	MCI_CLEAR_DATAENDCLR | \
	MCI_CLEAR_STARTBITERRCLR | \
	MCI_CLEAR_DATABLOCKENDCLR \
)

enum sd_command {
	SD_GO_IDLE_STATE = 0,
	SD_ALL_SEND_CID = 2,
	SD_SEND_RELATIVE_ADDR = 3,
	SD_SELECT_CARD = 7,
	SD_SEND_IF_COND = 8,
	SD_SEND_CSD = 9,
	SD_SET_BLOCKLEN = 16,
	SD_READ_SINGLE_BLOCK = 17,
	SD_APP_SEND_OP_COND = 41,
	SD_APP_CMD = 55,
};

enum sd_response {
	SD_RESPONSE_NONE = 0,
	SD_RESPONSE_SHORT = MCI_COMMAND_RESPONSE,
	SD_RESPONSE_LONG = MCI_COMMAND_RESPONSE | MCI_COMMAND_LONGRSP,
};

enum sd_card_state {
	SD_STATE_STBY = 3,
	SD_STATE_TRAN = 4,
};

#if defined(PMB8876) && defined(GPIO_MMC_VCC_EN)
typedef struct sd_card_t sd_card_t;

struct sd_card_t {
	bool is_v2;
	bool is_high_capacity;
	uint32_t ocr;
	uint32_t rca;
	uint8_t cid[16];
	uint8_t csd[16];
};

static uint32_t fifo_block[SD_BLOCK_SIZE / 4];
static uint32_t dma_block[SD_BLOCK_SIZE / 4];

static uint8_t sd_crc7(const uint8_t *data, size_t size) {
	uint8_t crc = 0;
	for (size_t i = 0; i < size; i++) {
		for (int bit = 7; bit >= 0; bit--) {
			bool feedback = ((data[i] >> bit) & 1) ^ ((crc >> 6) & 1);
			crc = (crc << 1) & 0x7F;
			if (feedback)
				crc ^= 0x09;
		}
	}
	return crc;
}

static uint32_t sd_register_get_bits(const uint8_t *reg, uint32_t lsb, uint32_t width) {
	uint32_t value = 0;
	for (uint32_t bit = lsb + width; bit-- > lsb;) {
		uint32_t byte = 15 - (bit / 8);
		value = (value << 1) | ((reg[byte] >> (bit % 8)) & 1);
	}
	return value;
}

static uint64_t sd_csd_get_capacity(const uint8_t *csd) {
	uint32_t structure = sd_register_get_bits(csd, 126, 2);
	if (structure == 1) {
		uint32_t c_size = sd_register_get_bits(csd, 48, 22);
		return ((uint64_t) c_size + 1) * 512 * 1024;
	}
	uint32_t read_bl_len = sd_register_get_bits(csd, 80, 4);
	uint32_t c_size = sd_register_get_bits(csd, 62, 12);
	uint32_t c_size_mult = sd_register_get_bits(csd, 47, 3);
	return ((uint64_t) c_size + 1) << (c_size_mult + 2 + read_bl_len);
}

static void mci_set_clock(uint32_t clkdiv) {
	MCI_CLOCK = MCI_CLOCK_ENABLE | (clkdiv << MCI_CLOCK_CLKDIV_SHIFT);
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
}

static void mci_read_long_response(uint8_t *reg) {
	uint32_t words[4] = { MCI_RESPONSE0, MCI_RESPONSE1, MCI_RESPONSE2, MCI_RESPONSE3 };
	for (size_t i = 0; i < 16; i++)
		reg[i] = words[i / 4] >> (24 - ((i % 4) * 8));
}

static uint32_t mci_execute_command(enum sd_command command, uint32_t argument, enum sd_response response) {
	MCI_CLEAR = MCI_COMMAND_CLEAR;
	MCI_ARGUMENT = argument;
	MCI_COMMAND = command | response | MCI_COMMAND_ENABLE;

	stopwatch_t start = stopwatch_get();
	while ((MCI_STATUS & MCI_COMMAND_STATUS) == 0 && stopwatch_elapsed_ms(start) < MCI_COMMAND_WAIT_MS)
		test_watchdog_serve();
	uint32_t status = MCI_STATUS & MCI_COMMAND_STATUS;

	MCI_COMMAND = 0;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	return status;
}

static uint32_t sd_execute_app_command(enum sd_command command, uint32_t argument, enum sd_response response) {
	uint32_t status = mci_execute_command(SD_APP_CMD, 0, SD_RESPONSE_SHORT);
	if (status != MCI_STATUS_CMDRESPEND || (MCI_RESPONSE0 & SD_R1_APP_CMD) == 0)
		return status;
	return mci_execute_command(command, argument, response);
}

static uint32_t sd_r1_get_state(uint32_t r1) {
	return (r1 & SD_R1_CURRENT_STATE) >> SD_R1_CURRENT_STATE_SHIFT;
}

static void sd_power_on(void) {
	MMCI_CLC = 1 << MOD_CLC_RMC_SHIFT;
	MCI_MASK0 = 0;
	MCI_MASK1 = 0;
	MCI_COMMAND = 0;
	MCI_DATACTRL = 0;
	MCI_POWER = MCI_POWER_CTRL_POWER_OFF;
	gpio_init_output(GPIO_MMC_VCC_EN, GPIO_OS_NONE, GPIO_PS_MANUAL, false, GPIO_PPEN_PUSHPULL, GPIO_PDPU_NONE,
		GPIO_ENAQ_OFF);
	stopwatch_msleep_wd(SD_POWER_RAMP_MS);

	GPIO_PIN(GPIO_MMCI_CLK) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT;
	/* Without these pull-ups block reads fail with DATACRCFAIL on the phone. */
	GPIO_PIN(GPIO_MMCI_CMD) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_PDPU_PULLUP;
	GPIO_PIN(GPIO_MMCI_DAT0) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PS_ALT | GPIO_PDPU_PULLUP;
	gpio_set(GPIO_MMC_VCC_EN, true);
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	stopwatch_msleep_wd(SD_POWER_RAMP_MS);
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	mci_set_clock(MCI_IDENTIFICATION_CLKDIV);
	stopwatch_msleep_wd(SD_INITIALIZATION_CLOCKS_MS);
}

static bool sd_is_card_present(void) {
	gpio_init_input(GPIO_MMC_CD, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, GPIO_ENAQ_OFF);
	return !gpio_get(GPIO_MMC_CD);
}

static bool test_card_identification(sd_card_t *card) {
	test_category("Card identification");

	uint32_t status = mci_execute_command(SD_GO_IDLE_STATE, 0, SD_RESPONSE_NONE);
	test_eq_u32("CMD0 completes as a command without response", MCI_STATUS_CMDSENT, status);

	status = mci_execute_command(SD_SEND_IF_COND, SD_CMD8_CHECK_PATTERN, SD_RESPONSE_SHORT);
	card->is_v2 = status == MCI_STATUS_CMDRESPEND;
	printf("# CMD8: %s, response=%08lX\n", card->is_v2 ? "SD 2.0 card" : "no response, SD 1.x card",
		MCI_RESPONSE0);
	if (card->is_v2) {
		test_eq_u32("CMD8 response carries index 8", SD_SEND_IF_COND, MCI_RESPCMD);
		test_eq_u32("CMD8 echoes the voltage and check pattern", SD_CMD8_CHECK_PATTERN,
			MCI_RESPONSE0 & SD_CMD8_ECHO_MASK);
	} else {
		test_eq_u32("CMD8 without a card answer times out", MCI_STATUS_CMDTIMEOUT, status);
	}

	uint32_t ocr_request = SD_OCR_VOLTAGE_WINDOW | (card->is_v2 ? SD_OCR_HCS : 0);
	stopwatch_t start = stopwatch_get();
	do {
		status = sd_execute_app_command(SD_APP_SEND_OP_COND, ocr_request, SD_RESPONSE_SHORT);
		card->ocr = MCI_RESPONSE0;
		test_watchdog_serve();
	} while (status == MCI_STATUS_CMDCRCFAIL && (card->ocr & SD_OCR_BUSY) == 0 &&
		stopwatch_elapsed_ms(start) < SD_READY_WAIT_MS);

	card->is_high_capacity = (card->ocr & SD_OCR_HCS) != 0;
	printf("# ACMD41: OCR=%08lX after %lu ms, %s\n", card->ocr, (uint32_t) stopwatch_elapsed_ms(start),
		card->is_high_capacity ? "high capacity" : "standard capacity");
	test_eq_u32("ACMD41 R3 without CRC sets CMDCRCFAIL", MCI_STATUS_CMDCRCFAIL, status);
	test_eq_u32("ACMD41 R3 reports reserved index 0x3F", SD_RESPONSE_RESERVED_INDEX, MCI_RESPCMD);
	if (!test_check("card leaves the busy state", (card->ocr & SD_OCR_BUSY) != 0))
		return false;

	status = mci_execute_command(SD_ALL_SEND_CID, 0, SD_RESPONSE_LONG);
	test_eq_u32("CMD2 long response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	test_eq_u32("CMD2 long response reports index 0x3F", SD_RESPONSE_RESERVED_INDEX, MCI_RESPCMD);
	test_eq_u32("RESPONSE3 bit 0 reads zero", 0, MCI_RESPONSE3 & 1);
	mci_read_long_response(card->cid);
	printf("# CID: MID=%02X OID=%c%c PNM=%c%c%c%c%c PRV=%u.%u PSN=%08lX\n",
		card->cid[0], card->cid[1], card->cid[2], card->cid[3], card->cid[4], card->cid[5], card->cid[6],
		card->cid[7], card->cid[8] >> 4, card->cid[8] & 0xF,
		sd_register_get_bits(card->cid, 24, 32));
	test_eq_u32("CID CRC7 matches its contents", sd_crc7(card->cid, 15), card->cid[15] >> 1);

	status = mci_execute_command(SD_SEND_RELATIVE_ADDR, 0, SD_RESPONSE_SHORT);
	test_eq_u32("CMD3 response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	test_eq_u32("CMD3 response carries index 3", SD_SEND_RELATIVE_ADDR, MCI_RESPCMD);
	card->rca = MCI_RESPONSE0 >> 16;
	printf("# RCA: %04lX\n", card->rca);
	return test_check("card publishes a nonzero RCA", card->rca != 0);
}

static bool test_card_selection(sd_card_t *card) {
	test_category("Card selection");

	uint32_t status = mci_execute_command(SD_SEND_CSD, card->rca << 16, SD_RESPONSE_LONG);
	test_eq_u32("CMD9 long response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	mci_read_long_response(card->csd);
	uint64_t capacity = sd_csd_get_capacity(card->csd);
	printf("# CSD: structure=%lu capacity=%lu MiB\n", sd_register_get_bits(card->csd, 126, 2),
		(uint32_t) (capacity >> 20));
	test_eq_u32("CSD CRC7 matches its contents", sd_crc7(card->csd, 15), card->csd[15] >> 1);

	status = mci_execute_command(SD_SELECT_CARD, card->rca << 16, SD_RESPONSE_SHORT);
	test_eq_u32("CMD7 response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	test_eq_u32("CMD7 is received in the stand-by state", SD_STATE_STBY, sd_r1_get_state(MCI_RESPONSE0));

	mci_set_clock(MCI_TRANSFER_CLKDIV);
	if (!card->is_high_capacity) {
		status = mci_execute_command(SD_SET_BLOCKLEN, SD_BLOCK_SIZE, SD_RESPONSE_SHORT);
		test_eq_u32("CMD16 sets the 512-byte block length", MCI_STATUS_CMDRESPEND, status);
	}
	return status == MCI_STATUS_CMDRESPEND;
}

static void test_block_read(void) {
	test_category("Single block read");

	MCI_CLEAR = MCI_DATA_CLEAR;
	MCI_DATATIMER = MCI_DATA_TIMEOUT_CYCLES;
	MCI_DATALENGTH = SD_BLOCK_SIZE;
	MCI_DATACTRL = MCI_DATACTRL_ENABLE | MCI_DATACTRL_DIRECTION_READ | MCI_DATACTRL_MODE_BLOCK |
		(SD_BLOCK_SIZE_LOG2 << MCI_DATACTRL_BLOCKSIZE_SHIFT);
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);

	uint32_t status = mci_execute_command(SD_READ_SINGLE_BLOCK, 0, SD_RESPONSE_SHORT);
	uint32_t r1 = MCI_RESPONSE0;

	size_t words = 0;
	stopwatch_t start = stopwatch_get();
	while (words < ARRAY_SIZE(fifo_block)) {
		uint32_t data_status = MCI_STATUS;
		if ((data_status & MCI_STATUS_RXFIFOHALFFULL) != 0) {
			for (size_t i = 0; i < MCI_FIFO_HALF_WORDS; i++)
				fifo_block[words++] = MCI_FIFO(0);
		} else if ((data_status & MCI_STATUS_RXDATAAVLBL) != 0) {
			fifo_block[words++] = MCI_FIFO(0);
		} else if ((data_status & MCI_DATA_ERRORS) != 0 || stopwatch_elapsed_ms(start) >= MCI_DATA_WAIT_MS) {
			break;
		}
	}
	while ((MCI_STATUS & (MCI_STATUS_DATAEND | MCI_DATA_ERRORS)) == 0 &&
		stopwatch_elapsed_ms(start) < MCI_DATA_WAIT_MS)
		test_watchdog_serve();
	uint32_t data_status = MCI_STATUS;

	test_eq_u32("CMD17 response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	test_eq_u32("CMD17 is received in the transfer state", SD_STATE_TRAN, sd_r1_get_state(r1));
	printf("# CMD17 data: %lu words, DATACNT=%lu FIFOCNT=%lu\n", (uint32_t) words, MCI_DATACNT, MCI_FIFOCNT);
	test_eq_u32("block data completes without errors", MCI_STATUS_DATAEND | MCI_STATUS_DATABLOCKEND,
		data_status & (MCI_STATUS_DATAEND | MCI_STATUS_DATABLOCKEND | MCI_DATA_ERRORS));
	test_eq_u32("FIFO delivers the whole block", ARRAY_SIZE(fifo_block), words);
	test_eq_u32("receive FIFO is empty after the block", 0, data_status & MCI_STATUS_RXDATAAVLBL);
	test_eq_u32("DATACNT reaches zero", 0, MCI_DATACNT);
	test_eq_u32("FIFOCNT reaches zero", 0, MCI_FIFOCNT);

	const uint8_t *bytes = (const uint8_t *) fifo_block;
	printf("# block 0 head:");
	for (size_t i = 0; i < 16; i++)
		printf(" %02X", bytes[i]);
	printf("\n");
	uint32_t signature = (bytes[SD_MBR_SIGNATURE_OFFSET] << 8) | bytes[SD_MBR_SIGNATURE_OFFSET + 1];
	test_eq_u32("block 0 ends with the 0x55AA boot signature", 0x55AA, signature);
}

static void dmac_start_rx(void) {
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(MCI_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(MCI_DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(MCI_DMA_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;
	dmac_request_select(MCI_DMA_RX_REQUEST, MCI_DMA_RX_SELECT);

	DMAC_CH_SRC_ADDR(MCI_DMA_CHANNEL) = (uint32_t) &MCI_FIFO(0);
	DMAC_CH_DST_ADDR(MCI_DMA_CHANNEL) = (uint32_t) dma_block;
	DMAC_CH_LLI(MCI_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(MCI_DMA_CHANNEL) = (
		ARRAY_SIZE(dma_block) | DMAC_CH_CONTROL_SB_SIZE_SZ_8 | DMAC_CH_CONTROL_DB_SIZE_SZ_8 |
		DMAC_CH_CONTROL_S_WIDTH_DWORD | DMAC_CH_CONTROL_D_WIDTH_DWORD |
		MCI_DMA_RX_AHB | DMAC_CH_CONTROL_D_AHB1 | DMAC_CH_CONTROL_DI | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(MCI_DMA_CHANNEL) = (
		(MCI_DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) | DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER |
		DMAC_CH_CONFIG_ENABLE
	);
}

static void test_dma_block_read(void) {
	test_category("Single block read through DMA");

	dmac_start_rx();
	MCI_CLEAR = MCI_DATA_CLEAR;
	MCI_DATATIMER = MCI_DATA_TIMEOUT_CYCLES;
	MCI_DATALENGTH = SD_BLOCK_SIZE;
	MCI_DATACTRL = MCI_DATACTRL_ENABLE | MCI_DATACTRL_DIRECTION_READ | MCI_DATACTRL_MODE_BLOCK |
		MCI_DATACTRL_DMAENABLE | (SD_BLOCK_SIZE_LOG2 << MCI_DATACTRL_BLOCKSIZE_SHIFT);
	stopwatch_usleep_wd(MCI_WRITE_GAP_US);
	uint32_t status = mci_execute_command(SD_READ_SINGLE_BLOCK, 0, SD_RESPONSE_SHORT);

	stopwatch_t start = stopwatch_get();
	while ((DMAC_RAW_TC_STATUS & BIT(MCI_DMA_CHANNEL)) == 0 && stopwatch_elapsed_ms(start) < MCI_DATA_WAIT_MS)
		test_watchdog_serve();
	while ((MCI_STATUS & (MCI_STATUS_DATAEND | MCI_DATA_ERRORS)) == 0 &&
		stopwatch_elapsed_ms(start) < MCI_DATA_WAIT_MS)
		test_watchdog_serve();
	uint32_t data_status = MCI_STATUS;

	test_eq_u32("CMD17 response passes the CRC check", MCI_STATUS_CMDRESPEND, status);
	test_check("last request completes the DMA channel", (DMAC_RAW_TC_STATUS & BIT(MCI_DMA_CHANNEL)) != 0);
	test_eq_u32("block data completes without errors", MCI_STATUS_DATAEND | MCI_STATUS_DATABLOCKEND,
		data_status & (MCI_STATUS_DATAEND | MCI_STATUS_DATABLOCKEND | MCI_DATA_ERRORS));
	test_eq_u32("DMA leaves the receive FIFO empty", 0, data_status & MCI_STATUS_RXDATAAVLBL);
	test_eq_u32("FIFOCNT reaches zero", 0, MCI_FIFOCNT);
	test_eq_memory("DMA block matches the FIFO read", fifo_block, dma_block, sizeof(dma_block));
}
#endif

int main(void) {
	test_start("MMCI SD card test");

#if defined(PMB8876) && defined(GPIO_MMC_VCC_EN)
	if (sd_is_card_present()) {
		sd_card_t card = { 0 };
		sd_power_on();
		if (test_card_identification(&card) && test_card_selection(&card)) {
			test_block_read();
			test_dma_block_read();
		}
	} else {
		test_skip("SD card", "MMC_CD reports an empty slot");
	}
#else
	test_skip("SD card", "board without an MMCI card slot");
#endif
	return test_finish();
}
