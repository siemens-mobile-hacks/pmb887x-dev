#include <pmb887x.h>
#include <string.h>
#if defined(BOARD_HAS_PMIC_PMB6812)
#include <pmic/PMB6812.h>
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/D1094XX.h>
#endif

#if defined(SIM_TIMING_TEST) || defined(SIM_LATENCY_TEST)
#include "cgu/clock.h"
#endif
#include "dmac.h"
#include "i2c.h"
#include "pmb681x.h"
#include "test.h"

#ifdef SIM_TIMER_TEST
#define SIM_TEST_NAME "SIM card timers"
#elif defined(SIM_TIMING_TEST)
#define SIM_TEST_NAME "SIM card timing"
#elif defined(SIM_LATENCY_TEST)
#define SIM_TEST_NAME "SIM card latency"
#elif defined(SIM_RW_TEST)
#define SIM_TEST_NAME "SIM card read and write"
#elif defined(SIM_HARDWARE_T0)
#define SIM_TEST_NAME "SIM card hardware T=0"
#elif defined(SIM_NO_CARD)
#define SIM_TEST_NAME "SIM no card"
#elif defined(SIM_DUMP_CARD_INFO)
#define SIM_TEST_NAME "SIM card info"
#else
#define SIM_TEST_NAME "SIM card"
#endif

#define SIM_IRQ_MASK (SIM_IMSC_ERR | SIM_IMSC_IN | SIM_IMSC_OK)
#define SIM_MAX_ATR_SIZE 64
#define SIM_MAX_APDU_DATA 256
#define SIM_DMA_CHANNEL 0
#define SIM_MISSING_FILE_ID 0x6FFF
#define SIM_T0_NULL_BYTE 0x60
#define SIM_RECORD_NEXT 0x02
#define SIM_RECORD_PREVIOUS 0x03
#define SIM_RECORD_ABSOLUTE 0x04
/* ISO 7816-3 needs 16 ETU between card and terminal characters; 5 ms covers RMC=4 at FPI1 OSC. */
#define SIM_TURNAROUND_MS 5
// CX75 and EL71 firmware use privileged DMA accesses with a four-byte memory burst for SIM.
#define SIM_DMA_PROTECTION_PRIVILEGED (1 << DMAC_CH_CONTROL_PROTECTION_SHIFT)
#ifdef PMB8875
#define SIM_CARD_RMC 1
#else
#define SIM_CARD_RMC 4
#endif

struct apdu_timing {
	uint32_t header_sent_tick;
	uint32_t procedure_tick;
	uint32_t data_done_tick;
	uint32_t sw1_tick;
	uint32_t null_bytes;
};

struct apdu_response {
	uint8_t data[SIM_MAX_APDU_DATA];
	size_t size;
	uint8_t sw1;
	uint8_t sw2;
	struct apdu_timing timing;
};

#if defined(SIM_HARDWARE_T0) || defined(SIM_TIMER_TEST)

static const uint8_t T0_HEADER[] __attribute__((aligned(4))) = {
	0xA0, 0xA4, 0x00, 0x00, 0x02,
};

#ifdef SIM_HARDWARE_T0

struct dma_lli {
	uint32_t source;
	uint32_t destination;
	uint32_t next;
	uint32_t control;
};

static const uint8_t T0_SELECT_MF[] __attribute__((aligned(4))) = {0x3F, 0x00};
static struct dma_lli t0_data_lli __attribute__((aligned(16)));

#endif

#endif

static uint32_t sim_last_tx_tick;
static uint32_t sim_last_rx_tick;
static uint32_t sim_null_bytes;
static uint32_t sim_reset_tick;

static void delay_ms(uint32_t milliseconds) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < milliseconds)
		test_watchdog_serve();
}

static void sim_clear_ok(void) {
	SIM_ICR = SIM_ICR_OK;
}

static bool sim_wait_ok(uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < timeout_ms) {
		if (SIM_RIS & SIM_RIS_ERR) {
			printf("# SIM error: STAT=%04X RIS=%X\n", (unsigned int) SIM_STAT, (unsigned int) SIM_RIS);
			SIM_ICR = SIM_ICR_ERR;
			return false;
		}
		if (SIM_RIS & SIM_RIS_OK)
			return true;
		test_watchdog_serve();
	}

	return false;
}

static bool sim_send_byte(uint8_t value) {
	sim_clear_ok();
	SIM_TXB = value;
	if (!sim_wait_ok(500))
		return false;
	sim_last_tx_tick = STM_TIM0;
	sim_clear_ok();

	return true;
}

static bool sim_receive_byte(uint8_t *value, uint32_t timeout_ms) {
	if (!sim_wait_ok(timeout_ms))
		return false;
	sim_last_rx_tick = STM_TIM0;
	*value = SIM_RXB;
	sim_clear_ok();

	return true;
}

static bool sim_receive_procedure(uint8_t *value) {
	while (sim_receive_byte(value, 1000)) {
		if (*value != SIM_T0_NULL_BYTE)
			return true;
		sim_null_bytes++;
	}

	return false;
}

static bool sim_supply_set(bool _enabled __attribute__((unused))) {
#if defined(BOARD_HAS_PMIC_PMB6812)
	uint8_t control;

	if (!i2c_smbus_read_byte(PMB6812_I2C_ADDR, PMB6812_PWCTRL5, &control))
		return false;
	control &= ~PMB6812_PWCTRL5_LSIMMD;
	control |= (_enabled ? PMB6812_PWCTRL5_LSIMMD_ON : PMB6812_PWCTRL5_LSIMMD_OFF);
	return pmb681x_reg_write(PMB6812_PWCTRL5, control);
#elif defined(BOARD_HAS_PMIC_D1094XX)
	uint8_t supply_enable;

	if (!i2c_smbus_read_byte(D1094XX_I2C_ADDR, D1094XX_SUPPLY_ENABLE_1, &supply_enable))
		return false;
	if (_enabled) {
		supply_enable |= D1094XX_SUPPLY_ENABLE_1_VAUDREGA_EN;
	} else {
		supply_enable &= ~D1094XX_SUPPLY_ENABLE_1_VAUDREGA_EN;
	}

	return i2c_smbus_write_byte(D1094XX_I2C_ADDR, D1094XX_SUPPLY_ENABLE_1, supply_enable);
#else
	return true;
#endif
}

static bool sim_power_on(uint32_t rmc, uint8_t atr[SIM_MAX_ATR_SIZE], uint32_t *atr_ticks, size_t *atr_size) {
	i2c_init();

	SIM_CLC = rmc << MOD_CLC_RMC_SHIFT;
	SIM_CON = 0;
	SIM_BRF = 0x5D;
	SIM_RXSPC = 0x28;
	SIM_IRQEN = SIM_IRQEN_ENOKINT | SIM_IRQEN_ENPAR | SIM_IRQEN_ENOVR;
	SIM_IMSC = 0;
	SIM_ICR = SIM_IRQ_MASK;
	SIM_CON = SIM_CON_SIMEN;
	if (!sim_supply_set(true))
		return false;
	GPIO_PIN(GPIO_I2C_SCL) = 0x1211;
	GPIO_PIN(GPIO_I2C_SDA) = 0x1211;

	delay_ms(255);
	SIM_CON |= SIM_CON_SIMVCC;
	SIM_CON |= SIM_CON_SIMIOL | SIM_CON_ERROFF;
	SIM_CON |= SIM_CON_SIMON;
	delay_ms(100);
	SIM_CON |= SIM_CON_UARTON;
	delay_ms(5);
	sim_reset_tick = STM_TIM0;
	SIM_CON |= SIM_CON_SIMRST;

	*atr_size = 0;
	while (*atr_size < SIM_MAX_ATR_SIZE &&
		sim_receive_byte(&atr[*atr_size], *atr_size == 0 ? 1000 : 100)) {
		if (atr_ticks)
			atr_ticks[*atr_size] = sim_last_rx_tick;
		(*atr_size)++;
	}

	return *atr_size != 0;
}

static void sim_power_off(void) {
	SIM_DMAE = 0;
	SIM_CON = 0;
	SIM_IRQEN = 0;
	SIM_ICR = SIM_IRQ_MASK;

	i2c_init();
	sim_supply_set(false);
}

#ifdef SIM_HARDWARE_T0

static bool sim_hardware_t0_select_dma(void) {
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;

	DMAC_CH_SRC_ADDR(SIM_DMA_CHANNEL) = (uint32_t) T0_HEADER;
	DMAC_CH_DST_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &SIM_TXB;
	t0_data_lli = (struct dma_lli) {
		.source = (uint32_t) T0_SELECT_MF,
		.destination = (uint32_t) &SIM_TXB,
		.next = 0,
		.control = (
			ARRAY_SIZE(T0_SELECT_MF) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
			DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
			SIM_DMA_TX_AHB | DMAC_CH_CONTROL_SI | SIM_DMA_PROTECTION_PRIVILEGED | DMAC_CH_CONTROL_I
		),
	};
	DMAC_CH_LLI(SIM_DMA_CHANNEL) = (uint32_t) &t0_data_lli;
	DMAC_CH_CONTROL(SIM_DMA_CHANNEL) = (
		ARRAY_SIZE(T0_HEADER) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		SIM_DMA_TX_AHB | DMAC_CH_CONTROL_SI | SIM_DMA_PROTECTION_PRIVILEGED
	);
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = (
		(SIM_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC
	);

	SIM_CON |= SIM_CON_UARTON | SIM_CON_SIMT0;
	SIM_IRQEN = SIM_IRQEN_ENOVR | SIM_IRQEN_ENT0END;
	SIM_CON &= ~(SIM_CON_ERROFF | SIM_CON_RPTOFF);
	SIM_DMAE = SIM_DMAE_OK;
	SIM_INS = 0xA4;
	SIM_P3 = 2;
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;

	stopwatch_t start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < 1000) {
		if (SIM_STAT & SIM_STAT_T0END)
			return true;
		if (DMAC_RAW_ERR_STATUS & BIT(SIM_DMA_CHANNEL))
			return false;
		test_watchdog_serve();
	}

	return false;
}

static uint8_t t0_getresp_header[5] __attribute__((aligned(4)));

static void sim_dma_reset(void) {
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;
}

static bool sim_hardware_t0_getresponse_dma(uint8_t length, struct apdu_response *response) {
	t0_getresp_header[0] = 0xA0;
	t0_getresp_header[1] = 0xC0;
	t0_getresp_header[2] = 0x00;
	t0_getresp_header[3] = 0x00;
	t0_getresp_header[4] = length;

	SIM_IRQEN = SIM_IRQEN_ENOVR | SIM_IRQEN_ENT0END;
	SIM_ICR = SIM_ICR_ERR | SIM_ICR_OK;

	// Phase 1: DMA the 5 header bytes to SIMTX (MEM2PER, DMAC-flow, count terminates it).
	DMAC_CONFIG = 0;
	sim_dma_reset();
	DMAC_CH_SRC_ADDR(SIM_DMA_CHANNEL) = (uint32_t) t0_getresp_header;
	DMAC_CH_DST_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &SIM_TXB;
	DMAC_CH_LLI(SIM_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(SIM_DMA_CHANNEL) = (
		ARRAY_SIZE(t0_getresp_header) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_DB_SIZE_SZ_1 | DMAC_CH_CONTROL_S_WIDTH_BYTE |
		DMAC_CH_CONTROL_D_WIDTH_BYTE | SIM_DMA_TX_AHB | DMAC_CH_CONTROL_SI |
		SIM_DMA_PROTECTION_PRIVILEGED | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = (
		(SIM_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC
	);

	SIM_INS = SIM_INS_INSDIR | 0xC0;
	SIM_P3 = length;
	SIM_DMAE = SIM_DMAE_OK;
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;

	// First UARTOK-equivalent: header fully clocked out (DMA terminal count).
	stopwatch_t start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < 1000) {
		if (DMAC_RAW_TC_STATUS & BIT(SIM_DMA_CHANNEL))
			break;
		if (DMAC_RAW_ERR_STATUS & BIT(SIM_DMA_CHANNEL))
			return false;
		test_watchdog_serve();
	}
	if (!(DMAC_RAW_TC_STATUS & BIT(SIM_DMA_CHANNEL)))
		return false;

	// Phase 2: re-program the same channel to receive the response body from SIMRX.
	sim_dma_reset();
	DMAC_CH_SRC_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &SIM_RXB;
	DMAC_CH_DST_ADDR(SIM_DMA_CHANNEL) = (uint32_t) response->data;
	DMAC_CH_LLI(SIM_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(SIM_DMA_CHANNEL) = (
		length | DMAC_CH_CONTROL_SB_SIZE_SZ_1 | DMAC_CH_CONTROL_DB_SIZE_SZ_4 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		SIM_DMA_RX_AHB | DMAC_CH_CONTROL_DI | SIM_DMA_PROTECTION_PRIVILEGED | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = (
		(SIM_DMA_RX_REQUEST << DMAC_CH_CONFIG_SRC_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_PER2MEM_PER | DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC | DMAC_CH_CONFIG_ENABLE
	);

	// Await T0END: the T=0 controller received the body (into the DMA) and SW1/SW2.
	start = stopwatch_get();
	while (stopwatch_elapsed_ms(start) < 1000) {
		if (SIM_STAT & SIM_STAT_T0END)
			break;
		if (DMAC_RAW_ERR_STATUS & BIT(SIM_DMA_CHANNEL))
			return false;
		test_watchdog_serve();
	}
	if (!(SIM_STAT & SIM_STAT_T0END))
		return false;

	response->size = length;
	response->sw1 = SIM_SW1;
	response->sw2 = SIM_SW2;
	return true;
}

#endif

#ifdef SIM_TIMER_TEST

static uint32_t sim_probe_character_timer(uint32_t ticks, uint32_t *elapsed_us) {
	DMAC_CONFIG = 0;
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = 0;
	DMAC_TC_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_ERR_CLEAR = BIT(SIM_DMA_CHANNEL);
	DMAC_CONFIG = DMAC_CONFIG_ENABLE;
	DMAC_SYNC = 0;

	DMAC_CH_SRC_ADDR(SIM_DMA_CHANNEL) = (uint32_t) T0_HEADER;
	DMAC_CH_DST_ADDR(SIM_DMA_CHANNEL) = (uint32_t) &SIM_TXB;
	DMAC_CH_LLI(SIM_DMA_CHANNEL) = 0;
	DMAC_CH_CONTROL(SIM_DMA_CHANNEL) = (
		ARRAY_SIZE(T0_HEADER) | DMAC_CH_CONTROL_SB_SIZE_SZ_4 | DMAC_CH_CONTROL_DB_SIZE_SZ_1 |
		DMAC_CH_CONTROL_S_WIDTH_BYTE | DMAC_CH_CONTROL_D_WIDTH_BYTE |
		SIM_DMA_TX_AHB | DMAC_CH_CONTROL_SI | SIM_DMA_PROTECTION_PRIVILEGED | DMAC_CH_CONTROL_I
	);
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) = (
		(SIM_DMA_TX_REQUEST << DMAC_CH_CONFIG_DST_PERIPH_SHIFT) |
		DMAC_CH_CONFIG_FLOW_CTRL_MEM2PER | DMAC_CH_CONFIG_INT_MASK_ERR |
		DMAC_CH_CONFIG_INT_MASK_TC
	);

	SIM_ICR = SIM_ICR_ERR | SIM_ICR_OK;
	SIM_CON |= SIM_CON_UARTON | SIM_CON_SIMT0;
	SIM_CHTIMER = ticks;
	SIM_IRQEN = SIM_IRQEN_ENOVR | SIM_IRQEN_ENCHTIMER;
	SIM_CON &= ~(SIM_CON_ERROFF | SIM_CON_RPTOFF);
	SIM_DMAE = SIM_DMAE_OK;
	SIM_INS = 0xA4;
	SIM_P3 = 2;

	stopwatch_t start = stopwatch_get();
	DMAC_CH_CONFIG(SIM_DMA_CHANNEL) |= DMAC_CH_CONFIG_ENABLE;
	while (stopwatch_elapsed_us(start) < 100000) {
		if (SIM_STAT & SIM_STAT_CHTIMEOUT)
			break;
		test_watchdog_serve();
	}
	*elapsed_us = stopwatch_elapsed_us(start);

	return SIM_STAT;
}

#endif

static bool sim_is_status_byte(uint8_t value) {
	return (value & 0xF0) == 0x60 || (value & 0xF0) == 0x90;
}

static bool sim_exchange(
	const uint8_t header[5],
	const uint8_t *tx_data,
	size_t tx_size,
	size_t rx_size,
	struct apdu_response *response
) {
	response->size = 0;
	delay_ms(SIM_TURNAROUND_MS);
#ifdef SIM_DUMP_CARD_INFO
	printf("# APDU TX:");
	for (size_t i = 0; i < 5; i++)
		printf(" %02X", header[i]);
	for (size_t i = 0; i < tx_size; i++)
		printf(" %02X", tx_data[i]);
	printf("\n");
#endif
	sim_null_bytes = 0;
	for (size_t i = 0; i < 5; i++) {
		if (!sim_send_byte(header[i]))
			return false;
	}
	struct apdu_timing *timing = &response->timing;
	timing->header_sent_tick = sim_last_tx_tick;
	timing->data_done_tick = sim_last_tx_tick;

	uint8_t procedure;
	if (!sim_receive_procedure(&procedure))
		return false;
	timing->procedure_tick = sim_last_rx_tick;

	size_t transferred = 0;
	size_t data_size = tx_size != 0 ? tx_size : rx_size;
	while (procedure == header[1] || procedure == (uint8_t) ~header[1]) {
		if (transferred == data_size)
			return false;
		size_t chunk = procedure == header[1] ? data_size - transferred : 1;
		for (size_t i = 0; i < chunk; i++) {
			if (tx_size != 0) {
				if (!sim_send_byte(tx_data[transferred]))
					return false;
				timing->data_done_tick = sim_last_tx_tick;
			} else {
				if (!sim_receive_byte(&response->data[transferred], 1000))
					return false;
				timing->data_done_tick = sim_last_rx_tick;
			}
			transferred++;
		}
		if (!sim_receive_procedure(&procedure))
			return false;
	}
	if (!sim_is_status_byte(procedure))
		return false;

	response->size = tx_size == 0 ? transferred : 0;
	response->sw1 = procedure;
	timing->sw1_tick = sim_last_rx_tick;
	timing->null_bytes = sim_null_bytes;
	if (!sim_receive_byte(&response->sw2, 1000))
		return false;

#ifdef SIM_DUMP_CARD_INFO
	printf("# APDU RX:");
	for (size_t i = 0; i < response->size; i++)
		printf(" %02X", response->data[i]);
	printf(" | SW1 SW2: %02X %02X\n", response->sw1, response->sw2);
#endif

	return true;
}

static bool sim_select(uint16_t file_id, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xA4, 0x00, 0x00, 0x02};
	const uint8_t data[2] = {(uint8_t) (file_id >> 8), (uint8_t) file_id};

	return sim_exchange(header, data, ARRAY_SIZE(data), 0, response);
}

static bool sim_get_response(uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xC0, 0x00, 0x00, length};

	return sim_exchange(header, NULL, 0, length == 0 ? SIM_MAX_APDU_DATA : length, response);
}

static bool sim_read_binary(uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xB0, 0x00, 0x00, length};

	return sim_exchange(header, NULL, 0, length == 0 ? SIM_MAX_APDU_DATA : length, response);
}

#ifdef SIM_DUMP_CARD_INFO

static bool sim_status(uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xF2, 0x00, 0x00, length};

	return sim_exchange(header, NULL, 0, length == 0 ? SIM_MAX_APDU_DATA : length, response);
}

#endif

#if defined(SIM_DUMP_CARD_INFO) || defined(SIM_RW_TEST) || defined(SIM_LATENCY_TEST)

static bool sim_read_record(uint8_t record, uint8_t mode, uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xB2, record, mode, length};

	return sim_exchange(header, NULL, 0, length, response);
}

#endif

static void print_bytes(const char *name, const uint8_t *data, size_t size) {
	printf("# %s (%u bytes):", name, (unsigned int) size);
	for (size_t i = 0; i < size; i++)
		printf(" %02X", data[i]);
	printf("\n");
}

static bool response_has_data(uint8_t sw1) {
	return sw1 == 0x9F || sw1 == 0x61;
}

#if defined(SIM_RW_TEST) || defined(SIM_LATENCY_TEST)

#define GSM_MF 0x3F00
#define GSM_DF_TELECOM 0x7F10
#define GSM_DF_GSM 0x7F20
#define GSM_EF_ICCID 0x2FE2
#define GSM_EF_SST 0x6F38
#define GSM_EF_ACM 0x6F39
#define GSM_EF_ADN 0x6F3A
#define GSM_EF_SMSS 0x6F43
#define GSM_EF_LND 0x6F44
#define GSM_EF_IMSI 0x6F07
#define GSM_EF_SIZE_MSB_OFFSET 2
#define GSM_EF_SIZE_LSB_OFFSET 3
#define GSM_EF_INCREASE_OFFSET 7
#define GSM_EF_ACCESS_OFFSET 8
#define GSM_EF_FILE_STATUS_OFFSET 11
#define GSM_EF_STRUCTURE_OFFSET 13
#define GSM_EF_RECORD_SIZE_OFFSET 14
#define GSM_EF_TRANSPARENT 0x00
#define GSM_EF_LINEAR_FIXED 0x01
#define GSM_EF_CYCLIC 0x03
#define GSM_SEEK_TYPE_2_FROM_BEGINNING 0x10
#define GSM_SEEK_TYPE_2_NEXT_FORWARD 0x12
#define GSM_ADN_EMPTY_ALPHA 0xFF
#define GSM_SEEK_RECORD_NUMBER_SIZE 1
#define GSM_ACM_SIZE 3
#define GSM_INCREASE_RESPONSE_SIZE (2 * GSM_ACM_SIZE)
#define GSM_STATUS_SUCCESS 0x9000
#define GSM_STATUS_OUT_OF_RANGE 0x9402
#define GSM_STATUS_ACCESS_DENIED 0x9804
#define GSM_STATUS_WRONG_PARAMETERS 0x6B00
#define GSM_SW1_WRONG_LENGTH 0x67
#define GSM_SW1_PROACTIVE_COMMAND 0x91
#define GSM_RAND_SIZE 16
#define GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE 12
#define GSM_ACCESS_SIZE 3
#define SIM_RECORD_CURRENT 0

struct sim_ef_info {
	uint16_t size;
	uint8_t structure;
	uint8_t record_size;
	uint16_t records;
	uint8_t increase;
	uint8_t access[GSM_ACCESS_SIZE];
	uint8_t file_status;
};

static uint16_t sim_status_word(const struct apdu_response *response) {
	return (response->sw1 << 8) | response->sw2;
}

static bool sim_select_ef(uint16_t df, uint16_t ef, struct sim_ef_info *info) {
	static struct apdu_response response;

	if (!sim_select(GSM_MF, &response) || !sim_select(df, &response))
		return false;
	if (!sim_select(ef, &response) || !response_has_data(response.sw1))
		return false;
	if (!sim_get_response(response.sw2, &response) || sim_status_word(&response) != GSM_STATUS_SUCCESS)
		return false;

	info->size = (response.data[GSM_EF_SIZE_MSB_OFFSET] << 8) | response.data[GSM_EF_SIZE_LSB_OFFSET];
	info->structure = response.data[GSM_EF_STRUCTURE_OFFSET];
	info->record_size = response.data[GSM_EF_RECORD_SIZE_OFFSET];
	info->records = info->record_size ? info->size / info->record_size : 0;
	info->increase = response.data[GSM_EF_INCREASE_OFFSET];
	memcpy(info->access, response.data + GSM_EF_ACCESS_OFFSET, GSM_ACCESS_SIZE);
	info->file_status = response.data[GSM_EF_FILE_STATUS_OFFSET];
	return true;
}

static bool sim_read_binary_at(uint16_t offset, uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xB0, (uint8_t) (offset >> 8), (uint8_t) offset, length};

	return sim_exchange(header, NULL, 0, length, response);
}

static bool sim_update_binary(uint16_t offset, const uint8_t *data, uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xD6, (uint8_t) (offset >> 8), (uint8_t) offset, length};

	return sim_exchange(header, data, length, 0, response);
}

static bool sim_update_record(
	uint8_t record,
	uint8_t mode,
	const uint8_t *data,
	uint8_t length,
	struct apdu_response *response
) {
	const uint8_t header[5] = {0xA0, 0xDC, record, mode, length};

	return sim_exchange(header, data, length, 0, response);
}

static bool sim_seek(uint8_t type_mode, const uint8_t *pattern, uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xA2, 0x00, type_mode, length};

	return sim_exchange(header, pattern, length, 0, response);
}

static bool sim_increase(const uint8_t value[GSM_ACM_SIZE], struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0x32, 0x00, 0x00, GSM_ACM_SIZE};

	return sim_exchange(header, value, GSM_ACM_SIZE, 0, response);
}

static bool sim_run_gsm_algorithm(const uint8_t rand[GSM_RAND_SIZE], uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0x88, 0x00, 0x00, length};

	return sim_exchange(header, rand, length, 0, response);
}

static void print_status(const char *operation, const struct apdu_response *response) {
	printf("# %s: status %02X%02X\n", operation, response->sw1, response->sw2);
}

#endif

#ifdef SIM_DUMP_CARD_INFO

static size_t decode_swapped_bcd(
	const uint8_t *data,
	size_t size,
	char *output,
	size_t output_size
) {
	size_t length = 0;

	for (size_t i = 0; i < size; i++) {
		uint8_t digits[2] = {data[i] & 0x0F, data[i] >> 4};
		for (size_t j = 0; j < ARRAY_SIZE(digits); j++) {
			if (digits[j] == 0x0F)
				continue;
			if (digits[j] > 9 || length + 1 >= output_size)
				goto done;
			output[length++] = '0' + digits[j];
		}
	}

done:
	output[length] = '\0';
	return length;
}

static size_t decode_imsi(const uint8_t *data, size_t size, char *output, size_t output_size) {
	if (size < 2 || output_size < 2 || (data[1] >> 4) > 9) {
		output[0] = '\0';
		return 0;
	}

	output[0] = '0' + (data[1] >> 4);
	size_t length = 1 + decode_swapped_bcd(data + 2, size - 2, output + 1, output_size - 1);
	output[length] = '\0';

	return length;
}

static bool iccid_luhn_valid(const char *iccid, size_t length) {
	uint32_t sum = 0;

	for (size_t i = 0; i < length; i++) {
		uint32_t digit = iccid[i] - '0';
		if (((length - 1 - i) & 1) != 0) {
			digit *= 2;
			if (digit > 9)
				digit -= 9;
		}
		sum += digit;
	}

	return length != 0 && sum % 10 == 0;
}

static bool sim_select_with_metadata(uint16_t file_id, struct apdu_response *metadata) {
	struct apdu_response selected;

	if (!sim_select(file_id, &selected) || !response_has_data(selected.sw1))
		return false;
	if (!sim_get_response(selected.sw2, metadata))
		return false;

	return metadata->sw1 == 0x90 && metadata->sw2 == 0x00 && metadata->size >= 15;
}

static bool sim_read_transparent(
	uint16_t file_id,
	uint8_t *data,
	size_t capacity,
	size_t *size
) {
	struct apdu_response metadata;
	struct apdu_response response;

	if (!sim_select_with_metadata(file_id, &metadata))
		return false;
	uint32_t file_size = (metadata.data[2] << 8) | metadata.data[3];
	if (metadata.data[13] != 0 || file_size == 0 || file_size > capacity || file_size > 256)
		return false;
	if (!sim_read_binary((uint8_t) file_size, &response) || response.sw1 != 0x90 || response.sw2 != 0)
		return false;
	for (size_t i = 0; i < file_size; i++)
		data[i] = response.data[i];
	*size = file_size;

	return true;
}

static void print_spn(const uint8_t *data, size_t size) {
	printf("# SPN display condition: %02X\n", data[0]);
	printf("# SPN: ");
	for (size_t i = 1; i < size && data[i] != 0xFF; i++) {
		if (data[i] >= 0x20 && data[i] <= 0x7E) {
			printf("%c", data[i]);
		} else {
			printf("\\x%02X", data[i]);
		}
	}
	printf("\n");
}

static void dump_msisdn(void) {
	struct apdu_response metadata;
	struct apdu_response record;

	if (!sim_select(0x3F00, &metadata) || !sim_select(0x7F10, &metadata) ||
		!response_has_data(metadata.sw1) ||
		!sim_select_with_metadata(0x6F40, &metadata)) {
		printf("# MSISDN: EF not available\n");
		return;
	}
	uint32_t file_size = (metadata.data[2] << 8) | metadata.data[3];
	uint8_t record_size = metadata.data[14];
	if (metadata.data[13] != 1 || record_size < 14 || file_size < record_size) {
		printf("# MSISDN: unexpected EF structure\n");
		return;
	}
	uint32_t records = file_size / record_size;
	for (uint32_t i = 1; i <= records; i++) {
		if (!sim_read_record(i, SIM_RECORD_ABSOLUTE, record_size, &record) || record.sw1 != 0x90 || record.sw2 != 0)
			break;
		size_t footer = record_size - 14;
		uint8_t bcd_size = record.data[footer];
		if (bcd_size == 0xFF || bcd_size <= 1 || bcd_size > 11)
			continue;
		char number[24];
		decode_swapped_bcd(record.data + footer + 2, bcd_size - 1, number, sizeof(number));
		printf(
			"# MSISDN record %u: %s%s (TON/NPI=%02X)\n",
			(unsigned int) i,
			record.data[footer + 1] == 0x91 ? "+" : "",
			number,
			record.data[footer + 1]
		);
	}
}

static void dump_card_identity(const uint8_t *iccid_data, size_t iccid_size) {
	uint8_t data[SIM_MAX_APDU_DATA];
	size_t size;
	char iccid[24];
	char imsi[20];
	uint8_t mnc_length = 0;

	print_bytes("EF_ICCID raw", iccid_data, iccid_size);
	size_t iccid_length = decode_swapped_bcd(iccid_data, iccid_size, iccid, sizeof(iccid));
	printf("# ICCID: %s\n", iccid);
	test_check("ICCID check digit is valid", iccid_luhn_valid(iccid, iccid_length));

	struct apdu_response selected;
	bool gsm_selected = sim_select(0x3F00, &selected) && sim_select(0x7F20, &selected) &&
		(selected.sw1 == 0x90 || response_has_data(selected.sw1));
	test_check("SELECT DF_GSM for identity dump", gsm_selected);
	if (!gsm_selected)
		return;

	if (sim_read_transparent(0x6FAD, data, sizeof(data), &size)) {
		print_bytes("EF_AD raw", data, size);
		if (size >= 4 && (data[3] & 0x0F) >= 2 && (data[3] & 0x0F) <= 3)
			mnc_length = data[3] & 0x0F;
	}

	bool imsi_read = sim_read_transparent(0x6F07, data, sizeof(data), &size);
	test_check("read EF_IMSI", imsi_read);
	if (imsi_read) {
		print_bytes("EF_IMSI raw", data, size);
		size_t imsi_length = decode_imsi(data, size, imsi, sizeof(imsi));
		printf("# IMSI: %s\n", imsi);
		if (imsi_length >= 5) {
			if (mnc_length == 0)
				mnc_length = 2;
			printf("# MCC: %c%c%c\n", imsi[0], imsi[1], imsi[2]);
			printf("# MNC: ");
			for (uint32_t i = 0; i < mnc_length; i++)
				printf("%c", imsi[3 + i]);
			printf("\n");
			printf("# MSIN: %s\n", imsi + 3 + mnc_length);
		}
	}

	if (sim_read_transparent(0x6F46, data, sizeof(data), &size)) {
		print_bytes("EF_SPN raw", data, size);
		print_spn(data, size);
	} else {
		printf("# SPN: EF not available\n");
	}

	dump_msisdn();
}

#endif

#if defined(SIM_TIMING_TEST) || defined(SIM_LATENCY_TEST)

#define SIM_CHARACTER_ETU 12
/* One ETU is 4 * BRF SIM clocks, and the SIM clock is FPI1 / 4 / RMC. */
#define SIM_FPI1_CLOCKS_PER_BRF_RMC_ETU 16
#define SIM_DEFAULT_BRF 0x5D
#define SIM_FPI1_OSC_HZ 26000000
#define SIM_CLOCKS_PER_BRF_ETU 4
#define SIM_ATR_T0_TA1_PRESENT 0x10
#define SIM_ATR_TA1_OFFSET 2
#define SIM_FIDI_FI_SHIFT 4
#define SIM_FIDI_DI_MASK 0x0F
#define SIM_PPS_SIZE 4
#define SIM_PPSS 0xFF
#define SIM_PPS0_PPS1_PRESENT 0x10
#define SIM_PPS_RMC 4

static const uint16_t FI_CLOCKS_PER_ETU[16] = {
	372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048
};
static const uint8_t DI_DIVISORS[16] = { 0, 1, 2, 4, 8, 16, 32, 64, 12, 20 };

static uint32_t sim_character_ticks(uint32_t rmc, uint32_t brf) {
	uint32_t fpi1_clocks = SIM_CHARACTER_ETU * SIM_FPI1_CLOCKS_PER_BRF_RMC_ETU * brf * rmc;
	return (uint64_t) fpi1_clocks * stopwatch_ticks_per_ms() * 1000 / SIM_FPI1_OSC_HZ;
}

#ifdef SIM_TIMING_TEST

#define SIM_READ_BINARY_SIZE 10

static const uint32_t TIMING_RMC_VALUES[] = { 2, 4 };

static uint32_t mean_interval(const uint32_t *ticks, size_t count) {
	return (ticks[count - 1] - ticks[0]) / (count - 1);
}

static bool sim_read_binary_timed(uint32_t ticks[SIM_READ_BINARY_SIZE]) {
	const uint8_t header[5] = {0xA0, 0xB0, 0x00, 0x00, SIM_READ_BINARY_SIZE};
	uint8_t value;

	delay_ms(SIM_TURNAROUND_MS);
	for (size_t i = 0; i < ARRAY_SIZE(header); i++) {
		if (!sim_send_byte(header[i]))
			return false;
	}
	if (!sim_receive_procedure(&value) || value != header[1])
		return false;
	for (size_t i = 0; i < SIM_READ_BINARY_SIZE; i++) {
		if (!sim_receive_byte(&value, 1000))
			return false;
		ticks[i] = sim_last_rx_tick;
	}
	uint8_t sw1;
	uint8_t sw2;
	return sim_receive_procedure(&sw1) && sim_receive_byte(&sw2, 1000) && sw1 == 0x90 && sw2 == 0x00;
}

static bool sim_select_iccid(void) {
	static struct apdu_response response;

	if (!sim_select(0x3F00, &response))
		return false;
	if (response_has_data(response.sw1) && !sim_get_response(response.sw2, &response))
		return false;
	if (!sim_select(0x2FE2, &response))
		return false;
	if (response_has_data(response.sw1) && !sim_get_response(response.sw2, &response))
		return false;
	return response.sw1 == 0x90 && response.sw2 == 0x00;
}

static void sim_test_character_timing(uint32_t rmc) {
	static uint8_t atr[SIM_MAX_ATR_SIZE];
	static uint32_t atr_ticks[SIM_MAX_ATR_SIZE];
	static uint32_t read_ticks[SIM_READ_BINARY_SIZE];
	size_t atr_size = 0;
	uint32_t expected = sim_character_ticks(rmc, SIM_DEFAULT_BRF);
	char name[64];

	bool powered = sim_power_on(rmc, atr, atr_ticks, &atr_size);
	snprintf(name, sizeof(name), "ATR arrives with RMC=%lu", rmc);
	test_check(name, powered && atr_size >= 2);
	if (!powered || atr_size < 2) {
		sim_power_off();
		return;
	}
	uint32_t atr_spacing = mean_interval(atr_ticks, atr_size);
	printf("# RMC=%lu: ATR %u bytes, mean spacing %lu STM ticks, expected %lu\n", rmc,
		(unsigned int) atr_size, atr_spacing, expected);
	snprintf(name, sizeof(name), "ATR characters are 12 ETU apart with RMC=%lu", rmc);
	test_check(name, test_u32_in_interval(atr_spacing, expected * 98 / 100, expected * 102 / 100));

	bool read = sim_select_iccid() && sim_read_binary_timed(read_ticks);
	snprintf(name, sizeof(name), "READ BINARY completes with RMC=%lu", rmc);
	test_check(name, read);
	if (read) {
		uint32_t read_spacing = mean_interval(read_ticks, SIM_READ_BINARY_SIZE);
		printf("# RMC=%lu: READ BINARY mean spacing %lu STM ticks, expected %lu\n", rmc, read_spacing, expected);
		snprintf(name, sizeof(name), "READ BINARY data characters are 12 ETU apart with RMC=%lu", rmc);
		test_check(name, test_u32_in_interval(read_spacing, expected * 98 / 100, expected * 102 / 100));
	}
	sim_power_off();
	delay_ms(100);
}

#endif

static uint32_t sim_ta1_to_brf(uint8_t ta1) {
	uint32_t divisor = DI_DIVISORS[ta1 & SIM_FIDI_DI_MASK];
	if (divisor == 0)
		return 0;
	return FI_CLOCKS_PER_ETU[ta1 >> SIM_FIDI_FI_SHIFT] / divisor / SIM_CLOCKS_PER_BRF_ETU;
}

static bool sim_exchange_pps(uint8_t ta1) {
	uint8_t request[SIM_PPS_SIZE] = { SIM_PPSS, SIM_PPS0_PPS1_PRESENT, ta1 };
	request[SIM_PPS_SIZE - 1] = request[0] ^ request[1] ^ request[2];

	delay_ms(SIM_TURNAROUND_MS);
	for (size_t i = 0; i < SIM_PPS_SIZE; i++) {
		if (!sim_send_byte(request[i]))
			return false;
	}
	for (size_t i = 0; i < SIM_PPS_SIZE; i++) {
		uint8_t value;
		if (!sim_receive_byte(&value, 1000) || value != request[i])
			return false;
	}
	return true;
}

#ifdef SIM_TIMING_TEST

static void sim_test_pps(void) {
	static uint8_t atr[SIM_MAX_ATR_SIZE];
	static uint32_t read_ticks[SIM_READ_BINARY_SIZE];
	size_t atr_size = 0;

	bool powered = sim_power_on(SIM_PPS_RMC, atr, NULL, &atr_size);
	bool has_ta1 = powered && atr_size > SIM_ATR_TA1_OFFSET && (atr[1] & SIM_ATR_T0_TA1_PRESENT) != 0;
	uint8_t ta1 = has_ta1 ? atr[SIM_ATR_TA1_OFFSET] : 0;
	uint32_t brf = has_ta1 ? sim_ta1_to_brf(ta1) : 0;
	test_check("ATR offers a supported TA1 rate", brf != 0);
	if (brf == 0) {
		sim_power_off();
		return;
	}
	printf("# TA1=%02X: BRF %lu after PPS\n", ta1, brf);

	test_check("Card echoes PPS that selects TA1", sim_exchange_pps(ta1));
	SIM_BRF = brf;
	bool read = sim_select_iccid() && sim_read_binary_timed(read_ticks);
	test_check("READ BINARY completes at the PPS rate", read);
	if (read) {
		uint32_t expected = sim_character_ticks(SIM_PPS_RMC, brf);
		uint32_t read_spacing = mean_interval(read_ticks, SIM_READ_BINARY_SIZE);
		printf("# PPS: READ BINARY mean spacing %lu STM ticks, expected %lu\n", read_spacing, expected);
		test_check("READ BINARY data characters are 12 PPS ETU apart",
			test_u32_in_interval(read_spacing, expected * 98 / 100, expected * 102 / 100));
	}
	sim_power_off();
}

#endif

#endif

#ifdef SIM_LATENCY_TEST

/* FPI1 OSC with RMC=2 gives the 3.25 MHz SIM clock that firmware gets from PLL/2 with RMC=4. */
#define SIM_LATENCY_RMC 2
#define SIM_CLOCK_FPI1_DIVIDER 4
#define SIM_ATR_MIN_CLOCKS 400
#define SIM_ATR_MAX_CLOCKS 40000
#define GSM_CLA 0xA0
#define GSM_INS_SELECT 0xA4
#define GSM_INS_STATUS 0xF2
#define GSM_INS_READ_BINARY 0xB0
#define GSM_INS_READ_RECORD 0xB2
#define GSM_INS_GET_RESPONSE 0xC0
#define GSM_INS_TERMINAL_PROFILE 0x10
#define GSM_INS_FETCH 0x12
#define GSM_INS_TERMINAL_RESPONSE 0x14
#define GSM_INS_INCREASE 0x32
#define GSM_INS_RUN_GSM_ALGORITHM 0x88
#define GSM_INS_SEEK 0xA2
#define GSM_INS_ENVELOPE 0xC2
#define GSM_INS_UPDATE_BINARY 0xD6
#define GSM_INS_UPDATE_RECORD 0xDC
#define SIM_LATENCY_REPEATS 4
#define SIM_LATENCY_MARKER 0x7F
#define SIM_SEEK_TYPE_1_FROM_BEGINNING 0x00
#define SAT_PROACTIVE_COMMAND_TAG 0xD0
#define SAT_MENU_SELECTION_TAG 0xD3
#define SAT_COMMAND_DETAILS_TAG 0x01
#define SAT_ITEM_TAG 0x0F
#define SAT_COMPREHENSION_REQUIRED 0x80
#define SAT_COMMAND_DETAILS_SIZE 3
#define SAT_COMMAND_SET_UP_MENU 0x25
#define SAT_RESULT_OK 0x00
#define SAT_RESULT_BEYOND_TERMINAL_CAPABILITIES 0x30
#define SAT_MAX_FOLLOW_UP_COMMANDS 4
#define SAT_FETCH_BASE_US 152
#define SAT_FETCH_NS_PER_BYTE 5640

struct firmware_apdu {
	uint8_t ins;
	uint8_t p1;
	uint8_t p2;
	uint8_t p3;
	uint16_t file_id;
};

struct apdu_latency_stats {
	uint8_t ins;
	const char *name;
	uint32_t reference_us;
	uint32_t count;
	uint32_t processing_us_sum;
	uint32_t processing_us_max;
	uint32_t null_bytes;
};

static const struct firmware_apdu FIRMWARE_BOOT_APDUS[] = {
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_STATUS, 0x00, 0x00, 0x17, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6FAE },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x01, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6FB7 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x3F00 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x2F05 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F05 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x07, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x3F00 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x2FE2 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x0A, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6FAE },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x01, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F38 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x0A, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x2FF2 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x3F00 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x2FE6 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F07 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F7E },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F10 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F3A },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F3B },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F4B },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F78 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x02, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F16 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F14 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F07 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x09, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6FAD },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x03, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F7E },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x0B, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F20 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x09, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F74 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x10, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F7B },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x0C, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F31 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x01, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F39 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_READ_RECORD, 0x01, 0x04, 0x03, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F37 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F37 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x03, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F41 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x05, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F45 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x14, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F10 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F44 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F43 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x02, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F4A },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F3D },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F42 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F52 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x09, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F53 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F13 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F11 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0xEA03 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0xEA00 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F43 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F10 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x2FF1 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F20 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F30 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0E, 0 },
	{ GSM_INS_READ_BINARY, 0x00, 0x00, 0x30, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x7F10 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F3C },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_SELECT, 0x00, 0x00, 0x02, 0x6F40 },
	{ GSM_INS_GET_RESPONSE, 0x00, 0x00, 0x0F, 0 },
	{ GSM_INS_READ_RECORD, 0x01, 0x04, 0x1C, 0 },
	{ GSM_INS_READ_RECORD, 0x02, 0x04, 0x1C, 0 },
	{ GSM_INS_STATUS, 0x00, 0x00, 0x17, 0 },
};

/* Mean processing times of the reference SIM (ATR 3B 1E 95 80 67 21 54 4D 4D ...). */
static struct apdu_latency_stats apdu_latency_stats[] = {
	{ .ins = GSM_INS_SELECT, .name = "SELECT", .reference_us = 3138 },
	{ .ins = GSM_INS_STATUS, .name = "STATUS", .reference_us = 3770 },
	{ .ins = GSM_INS_READ_BINARY, .name = "READ BINARY", .reference_us = 295 },
	{ .ins = GSM_INS_READ_RECORD, .name = "READ RECORD", .reference_us = 313 },
	{ .ins = GSM_INS_GET_RESPONSE, .name = "GET RESPONSE", .reference_us = 91 },
	{ .ins = GSM_INS_UPDATE_BINARY, .name = "UPDATE BINARY", .reference_us = 929 },
	{ .ins = GSM_INS_UPDATE_RECORD, .name = "UPDATE RECORD", .reference_us = 3219 },
	{ .ins = GSM_INS_SEEK, .name = "SEEK", .reference_us = 925 },
	{ .ins = GSM_INS_INCREASE, .name = "INCREASE", .reference_us = 1851 },
	{ .ins = GSM_INS_RUN_GSM_ALGORITHM, .name = "RUN GSM ALGORITHM", .reference_us = 52815 },
	{ .ins = GSM_INS_TERMINAL_PROFILE, .name = "TERMINAL PROFILE", .reference_us = 2541 },
	{ .ins = GSM_INS_TERMINAL_RESPONSE, .name = "TERMINAL RESPONSE", .reference_us = 83 },
	{ .ins = GSM_INS_ENVELOPE, .name = "ENVELOPE", .reference_us = 488 },
};

static const uint8_t SAT_TERMINAL_PROFILE[] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x0F, 0x00, 0xDF, 0x7F, 0x00, 0x00, 0x1F, 0x23, 0x00, 0x00, 0x00, 0x03
};

static uint32_t ticks_to_us(uint32_t ticks) {
	return (uint64_t) ticks * 1000 / stopwatch_ticks_per_ms();
}

static uint32_t card_delay_us(uint32_t start_tick, uint32_t end_tick, uint32_t transfer_ticks) {
	uint32_t elapsed_ticks = end_tick - start_tick;
	return elapsed_ticks > transfer_ticks ? ticks_to_us(elapsed_ticks - transfer_ticks) : 0;
}

static struct apdu_latency_stats *apdu_latency_stats_find(uint8_t ins) {
	for (size_t i = 0; i < ARRAY_SIZE(apdu_latency_stats); i++) {
		if (apdu_latency_stats[i].ins == ins)
			return &apdu_latency_stats[i];
	}
	return NULL;
}

static uint32_t apdu_latency_record(
	uint8_t ins,
	const struct apdu_response *response,
	bool card_receives_data,
	uint32_t character_ticks
) {
	const struct apdu_timing *timing = &response->timing;
	bool acknowledged = timing->procedure_tick != timing->sw1_tick;
	uint32_t ack_us = card_delay_us(timing->header_sent_tick, timing->procedure_tick, character_ticks);
	uint32_t status_us = card_delay_us(timing->data_done_tick, timing->sw1_tick, character_ticks);
	uint32_t processing_us = card_receives_data || !acknowledged ? status_us : ack_us;

	struct apdu_latency_stats *stats = apdu_latency_stats_find(ins);
	stats->count++;
	stats->null_bytes += timing->null_bytes;
	stats->processing_us_sum += processing_us;
	stats->processing_us_max = MAX(stats->processing_us_max, processing_us);
	return processing_us;
}

static bool sim_firmware_apdu_exchange(const struct firmware_apdu *apdu, struct apdu_response *response) {
	const uint8_t header[5] = { GSM_CLA, apdu->ins, apdu->p1, apdu->p2, apdu->p3 };

	if (apdu->ins == GSM_INS_SELECT) {
		const uint8_t data[2] = { (uint8_t) (apdu->file_id >> 8), (uint8_t) apdu->file_id };
		return sim_exchange(header, data, ARRAY_SIZE(data), 0, response);
	}
	return sim_exchange(header, NULL, 0, apdu->p3, response);
}

static void sim_test_firmware_apdus(uint32_t character_ticks) {
	static struct apdu_response response;

	size_t completed = 0;
	while (completed < ARRAY_SIZE(FIRMWARE_BOOT_APDUS)) {
		const struct firmware_apdu *apdu = &FIRMWARE_BOOT_APDUS[completed];
		if (!sim_firmware_apdu_exchange(apdu, &response))
			break;

		const struct apdu_timing *timing = &response.timing;
		bool acknowledged = timing->procedure_tick != timing->sw1_tick;
		uint32_t ack_us = acknowledged ?
			card_delay_us(timing->header_sent_tick, timing->procedure_tick, character_ticks) : 0;
		uint32_t status_us = card_delay_us(timing->data_done_tick, timing->sw1_tick, character_ticks);
		apdu_latency_record(apdu->ins, &response, apdu->ins == GSM_INS_SELECT, character_ticks);
		const struct apdu_latency_stats *stats = apdu_latency_stats_find(apdu->ins);

		printf("# %3u %s", (unsigned int) completed, stats->name);
		if (apdu->ins == GSM_INS_SELECT) {
			printf(" %04X", apdu->file_id);
		} else {
			printf(" P3=%u", apdu->p3);
		}
		if (acknowledged) {
			printf(": ACK after %lu us", ack_us);
		} else {
			printf(": no ACK");
		}
		printf(", status %02X%02X %lu us after data, %lu NULL bytes\n",
			response.sw1, response.sw2, status_us, timing->null_bytes);
		completed++;
	}
	if (completed < ARRAY_SIZE(FIRMWARE_BOOT_APDUS))
		printf("# APDU %u got no complete answer\n", (unsigned int) completed);
	test_check("Card answers every firmware boot APDU", completed == ARRAY_SIZE(FIRMWARE_BOOT_APDUS));
}

static void apdu_latency_stats_check(void) {
	for (size_t i = 0; i < ARRAY_SIZE(apdu_latency_stats); i++) {
		const struct apdu_latency_stats *stats = &apdu_latency_stats[i];
		if (stats->count == 0)
			continue;
		uint32_t mean_us = stats->processing_us_sum / stats->count;
		printf("# %s: %lu APDUs, processing mean %lu us max %lu us, reference %lu us, %lu NULL bytes\n",
			stats->name, stats->count, mean_us, stats->processing_us_max, stats->reference_us,
			stats->null_bytes);
		char name[96];
		snprintf(name, sizeof(name), "%s processing is within 25%% of the reference card", stats->name);
		test_check(name, test_u32_in_interval(mean_us, stats->reference_us * 75 / 100,
			stats->reference_us * 125 / 100));
	}
}

static void sim_measure_update_binary(uint32_t character_ticks) {
	static struct apdu_response response;
	uint8_t original[SIM_MAX_APDU_DATA];
	uint8_t marked[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;

	bool smss = sim_select_ef(GSM_DF_TELECOM, GSM_EF_SMSS, &info) && sim_read_binary_at(0, info.size, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS;
	test_check("Read EF_SMSS", smss);
	if (!smss)
		return;
	memcpy(original, response.data, info.size);
	memcpy(marked, original, info.size);
	marked[1] ^= SIM_LATENCY_MARKER;

	bool updated = true;
	for (size_t i = 0; i < SIM_LATENCY_REPEATS; i++) {
		const uint8_t *content = i % 2 == 0 ? marked : original;
		updated = sim_update_binary(0, content, info.size, &response) &&
			sim_status_word(&response) == GSM_STATUS_SUCCESS && updated;
		apdu_latency_record(GSM_INS_UPDATE_BINARY, &response, true, character_ticks);
	}
	bool restored = sim_read_binary_at(0, info.size, &response) && memcmp(response.data, original, info.size) == 0;
	test_check("UPDATE BINARY EF_SMSS changes and restores the file", updated && restored);
}

static void sim_measure_update_record(uint32_t character_ticks) {
	static struct apdu_response response;
	static const uint8_t empty_alpha = GSM_ADN_EMPTY_ALPHA;
	uint8_t empty[SIM_MAX_APDU_DATA];
	uint8_t marked[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;

	bool selected = sim_select_ef(GSM_DF_TELECOM, GSM_EF_ADN, &info);
	test_check("SELECT EF_ADN", selected);
	if (!selected)
		return;
	for (size_t i = 0; i < SIM_LATENCY_REPEATS; i++) {
		sim_seek(SIM_SEEK_TYPE_1_FROM_BEGINNING, &empty_alpha, 1, &response);
		apdu_latency_record(GSM_INS_SEEK, &response, true, character_ticks);
	}
	print_status("SEEK type 1 for an empty name", &response);
	memset(empty, 0xFF, info.record_size);
	bool found = sim_status_word(&response) == GSM_STATUS_SUCCESS &&
		sim_read_record(SIM_RECORD_CURRENT, SIM_RECORD_ABSOLUTE, info.record_size, &response) &&
		memcmp(response.data, empty, info.record_size) == 0;
	test_check("SEEK type 1 finds an empty EF_ADN record", found);
	if (!found)
		return;
	memcpy(marked, empty, info.record_size);
	marked[0] = SIM_LATENCY_MARKER;

	bool updated = true;
	for (size_t i = 0; i < SIM_LATENCY_REPEATS; i++) {
		const uint8_t *content = i % 2 == 0 ? marked : empty;
		updated = sim_update_record(SIM_RECORD_CURRENT, SIM_RECORD_ABSOLUTE, content, info.record_size, &response) &&
			sim_status_word(&response) == GSM_STATUS_SUCCESS && updated;
		apdu_latency_record(GSM_INS_UPDATE_RECORD, &response, true, character_ticks);
	}
	bool restored = sim_read_record(SIM_RECORD_CURRENT, SIM_RECORD_ABSOLUTE, info.record_size, &response) &&
		memcmp(response.data, empty, info.record_size) == 0;
	test_check("UPDATE RECORD changes and restores an empty EF_ADN record", updated && restored);
}

static void sim_measure_writes(uint32_t character_ticks) {
	static struct apdu_response response;
	static const uint8_t zero[GSM_ACM_SIZE] = { 0 };
	static const uint8_t rand[GSM_RAND_SIZE] = { 0 };
	struct sim_ef_info info;

	sim_measure_update_binary(character_ticks);
	sim_measure_update_record(character_ticks);

	bool acm = sim_select_ef(GSM_DF_GSM, GSM_EF_ACM, &info);
	test_check("SELECT EF_ACM", acm);
	if (acm) {
		sim_increase(zero, &response);
		apdu_latency_record(GSM_INS_INCREASE, &response, true, character_ticks);
		print_status("INCREASE by zero", &response);
	}

	for (size_t i = 0; i < SIM_LATENCY_REPEATS; i++) {
		sim_run_gsm_algorithm(rand, GSM_RAND_SIZE, &response);
		apdu_latency_record(GSM_INS_RUN_GSM_ALGORITHM, &response, true, character_ticks);
		if (response_has_data(response.sw1)) {
			sim_get_response(response.sw2, &response);
			apdu_latency_record(GSM_INS_GET_RESPONSE, &response, false, character_ticks);
		}
	}
	test_check("RUN GSM ALGORITHM completes", sim_status_word(&response) == GSM_STATUS_SUCCESS);
}

static bool sat_exchange(
	uint8_t ins,
	const uint8_t *data,
	uint8_t length,
	struct apdu_response *response,
	uint32_t character_ticks
) {
	const uint8_t header[5] = { GSM_CLA, ins, 0x00, 0x00, length };

	if (!sim_exchange(header, data, length, 0, response))
		return false;
	apdu_latency_record(ins, response, true, character_ticks);
	return true;
}

static bool sat_fetch(uint8_t length, struct apdu_response *response, uint32_t character_ticks) {
	const uint8_t header[5] = { GSM_CLA, GSM_INS_FETCH, 0x00, 0x00, length };

	if (!sim_exchange(header, NULL, 0, length, response))
		return false;
	uint32_t processing_us = card_delay_us(response->timing.header_sent_tick, response->timing.procedure_tick,
		character_ticks);
	uint32_t reference_us = SAT_FETCH_BASE_US + length * SAT_FETCH_NS_PER_BYTE / 1000;
	printf("# FETCH %u bytes: processing %lu us, reference %lu us\n", length, processing_us, reference_us);
	print_bytes("Proactive command", response->data, response->size);
	test_check("FETCH processing is within 25% of the reference card",
		test_u32_in_interval(processing_us, reference_us * 75 / 100, reference_us * 125 / 100));
	return response->size >= 2 && response->data[0] == SAT_PROACTIVE_COMMAND_TAG;
}

static const uint8_t *sat_find_tag(const struct apdu_response *command, uint8_t tag) {
	size_t offset = 2;
	while (offset + 2 <= command->size) {
		uint8_t current = command->data[offset] & ~SAT_COMPREHENSION_REQUIRED;
		uint8_t length = command->data[offset + 1];
		if (current == tag)
			return &command->data[offset];
		offset += 2 + length;
	}
	return NULL;
}

static bool sat_respond(
	const struct apdu_response *command,
	uint8_t result,
	struct apdu_response *response,
	uint32_t character_ticks
) {
	const uint8_t *details = sat_find_tag(command, SAT_COMMAND_DETAILS_TAG);
	if (!details || details[1] != SAT_COMMAND_DETAILS_SIZE)
		return false;

	const uint8_t terminal_response[] = {
		0x81, SAT_COMMAND_DETAILS_SIZE, details[2], details[3], details[4],
		0x82, 0x02, 0x82, 0x81,
		0x83, 0x01, result,
	};
	return sat_exchange(GSM_INS_TERMINAL_RESPONSE, terminal_response, sizeof(terminal_response), response,
		character_ticks);
}

static bool sat_complete_session(struct apdu_response *response, uint8_t result, uint32_t character_ticks) {
	static struct apdu_response command;

	for (size_t i = 0; i < SAT_MAX_FOLLOW_UP_COMMANDS && response->sw1 == GSM_SW1_PROACTIVE_COMMAND; i++) {
		if (!sat_fetch(response->sw2, &command, character_ticks))
			return false;
		if (!sat_respond(&command, result, response, character_ticks))
			return false;
		print_status("TERMINAL RESPONSE", response);
	}
	return sim_status_word(response) == GSM_STATUS_SUCCESS;
}

static void sim_measure_toolkit(uint32_t character_ticks) {
	static struct apdu_response response;
	static struct apdu_response command;

	test_category("SIM Application Toolkit latency");
	bool profile = sat_exchange(GSM_INS_TERMINAL_PROFILE, SAT_TERMINAL_PROFILE, sizeof(SAT_TERMINAL_PROFILE),
		&response, character_ticks);
	print_status("TERMINAL PROFILE", &response);
	test_check("TERMINAL PROFILE reports a pending proactive command",
		profile && response.sw1 == GSM_SW1_PROACTIVE_COMMAND);
	if (!profile || response.sw1 != GSM_SW1_PROACTIVE_COMMAND)
		return;

	bool fetched = sat_fetch(response.sw2, &command, character_ticks);
	const uint8_t *details = fetched ? sat_find_tag(&command, SAT_COMMAND_DETAILS_TAG) : NULL;
	test_check("FETCH returns SET UP MENU", details && details[3] == SAT_COMMAND_SET_UP_MENU);
	if (!details || details[3] != SAT_COMMAND_SET_UP_MENU)
		return;
	const uint8_t *item = sat_find_tag(&command, SAT_ITEM_TAG);
	test_check("SET UP MENU has an item", item != NULL);
	if (!item)
		return;
	uint8_t item_id = item[2];

	bool responded = sat_respond(&command, SAT_RESULT_OK, &response, character_ticks);
	print_status("TERMINAL RESPONSE", &response);
	test_check("TERMINAL RESPONSE to SET UP MENU is accepted", responded &&
		(sim_status_word(&response) == GSM_STATUS_SUCCESS || response.sw1 == GSM_SW1_PROACTIVE_COMMAND));
	test_check("Proactive commands after SET UP MENU complete",
		sat_complete_session(&response, SAT_RESULT_OK, character_ticks));

	const uint8_t menu_selection[] = { SAT_MENU_SELECTION_TAG, 0x07, 0x82, 0x02, 0x01, 0x81, 0x90, 0x01, item_id };
	bool selected = sat_exchange(GSM_INS_ENVELOPE, menu_selection, sizeof(menu_selection), &response,
		character_ticks);
	print_status("ENVELOPE MENU SELECTION", &response);
	test_check("ENVELOPE MENU SELECTION is accepted", selected &&
		(sim_status_word(&response) == GSM_STATUS_SUCCESS || response.sw1 == GSM_SW1_PROACTIVE_COMMAND));
	test_check("Proactive commands after MENU SELECTION complete",
		sat_complete_session(&response, SAT_RESULT_BEYOND_TERMINAL_CAPABILITIES, character_ticks));
}

static void sim_test_firmware_latency(void) {
	static uint8_t atr[SIM_MAX_ATR_SIZE];
	static uint32_t atr_ticks[SIM_MAX_ATR_SIZE];
	size_t atr_size = 0;

	test_category("Firmware boot APDU latency");
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	uint32_t sim_clock_hz = cpu_get_fpi1_freq() / SIM_CLOCK_FPI1_DIVIDER / SIM_LATENCY_RMC;
	printf("# SIM clock %lu Hz\n", sim_clock_hz);

	bool powered = sim_power_on(SIM_LATENCY_RMC, atr, atr_ticks, &atr_size);
	test_check("ATR arrives", powered);
	if (!powered)
		return;
	print_bytes("ATR", atr, atr_size);
	uint32_t character_ticks = sim_character_ticks(SIM_LATENCY_RMC, SIM_DEFAULT_BRF);
	uint32_t atr_delay_us = card_delay_us(sim_reset_tick, atr_ticks[0], character_ticks);
	uint32_t atr_delay_clocks = (uint64_t) atr_delay_us * sim_clock_hz / 1000000;
	printf("# ATR starts %lu us (%lu SIM clocks) after RST\n", atr_delay_us, atr_delay_clocks);
	test_check("ATR starts within the ISO 7816-3 window of 400 to 40000 SIM clocks after RST",
		test_u32_in_interval(atr_delay_clocks, SIM_ATR_MIN_CLOCKS, SIM_ATR_MAX_CLOCKS));

	bool has_ta1 = atr_size > SIM_ATR_TA1_OFFSET && (atr[1] & SIM_ATR_T0_TA1_PRESENT) != 0;
	uint8_t ta1 = has_ta1 ? atr[SIM_ATR_TA1_OFFSET] : 0;
	uint32_t brf = has_ta1 ? sim_ta1_to_brf(ta1) : 0;
	if (brf != 0) {
		bool negotiated = sim_exchange_pps(ta1);
		test_check("Card echoes PPS that selects TA1", negotiated);
		if (!negotiated)
			return;
		uint32_t pps_us = card_delay_us(sim_last_tx_tick, sim_last_rx_tick, SIM_PPS_SIZE * character_ticks);
		printf("# PPS TA1=%02X: card answers %lu us after the request, BRF %lu\n", ta1, pps_us, brf);
		SIM_BRF = brf;
		character_ticks = sim_character_ticks(SIM_LATENCY_RMC, brf);
	} else {
		printf("# ATR offers no supported TA1, APDUs use the default rate\n");
	}

	sim_test_firmware_apdus(character_ticks);
	test_category("Command latency");
	sim_measure_writes(character_ticks);
	sim_measure_toolkit(character_ticks);
	apdu_latency_stats_check();
}

#endif

#ifdef SIM_RW_TEST

#define SIM_RW_MAX_RECORDS 16
#define SIM_RW_MARKER 0x7F
#define GSM_EF_INCREASE_ALLOWED 0x40
#define GSM_EF_INVALIDATE_MASK 0x0F
#define GSM_ACCESS_ALWAYS 0x0
#define GSM_ACCESS_CHV1 0x1
#define GSM_STATUS_RESPONSE_AVAILABLE_SW1 0x9F
#define GSM_INVALIDATE_ACCESS_BYTE 2
#define GSM_STATUS_FILE_INCONSISTENT 0x9408
#define GSM_DF_CHARACTERISTICS_OFFSET 13
#define GSM_DF_CHV1_STATUS_OFFSET 18
#define GSM_DF_CHV_STATUS_COUNT 4
#define GSM_DF_CHV1_DISABLED 0x80
#define GSM_SECRET_CODE_INITIALIZED 0x80

static uint8_t lnd_records[SIM_RW_MAX_RECORDS][SIM_MAX_APDU_DATA];

static bool sim_select_df(uint16_t df) {
	static struct apdu_response response;

	return sim_select(GSM_MF, &response) && sim_select(df, &response);
}

static bool sim_invalidate(struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0x04, 0x00, 0x00, 0x00};

	return sim_exchange(header, NULL, 0, 0, response);
}

static bool sim_rehabilitate(struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0x44, 0x00, 0x00, 0x00};

	return sim_exchange(header, NULL, 0, 0, response);
}

static bool sim_read_record_matches(uint8_t record, uint8_t mode, const uint8_t *expected, uint8_t length) {
	static struct apdu_response response;

	if (!sim_read_record(record, mode, length, &response))
		return false;
	return sim_status_word(&response) == GSM_STATUS_SUCCESS && memcmp(response.data, expected, length) == 0;
}

static void sim_test_transparent(void) {
	static struct apdu_response response;
	uint8_t original[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;

	test_category("Transparent EF_SMSS");
	bool selected = sim_select_ef(GSM_DF_TELECOM, GSM_EF_SMSS, &info);
	bool usable = selected && info.structure == GSM_EF_TRANSPARENT && info.size >= 2;
	test_check("SELECT transparent EF_SMSS", usable);
	if (!usable)
		return;
	bool saved = sim_read_binary_at(0, info.size, &response) && sim_status_word(&response) == GSM_STATUS_SUCCESS;
	test_check("READ BINARY reads the whole file", saved);
	if (!saved)
		return;
	memcpy(original, response.data, info.size);

	const uint8_t marker = SIM_RW_MARKER;
	bool updated = sim_update_binary(1, &marker, 1, &response);
	print_status("UPDATE BINARY at offset 1", &response);
	test_check("UPDATE BINARY at an offset succeeds", updated && sim_status_word(&response) == GSM_STATUS_SUCCESS);
	bool read = sim_read_binary_at(0, info.size, &response) && sim_status_word(&response) == GSM_STATUS_SUCCESS;
	test_check("UPDATE BINARY changes only the addressed byte",
		read && response.data[0] == original[0] && response.data[1] == marker);

	sim_read_binary_at(info.size, 1, &response);
	print_status("READ BINARY past the end", &response);
	test_check("READ BINARY past the end reports wrong parameters",
		sim_status_word(&response) == GSM_STATUS_WRONG_PARAMETERS);
	sim_read_binary_at(0, info.size + 1, &response);
	print_status("READ BINARY longer than the file", &response);
	test_check("READ BINARY longer than the file reports the available length",
		response.sw1 == GSM_SW1_WRONG_LENGTH && response.sw2 == info.size);

	bool restored = sim_update_binary(0, original, info.size, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS &&
		sim_read_binary_at(0, info.size, &response) &&
		memcmp(response.data, original, info.size) == 0;
	test_check("EF_SMSS is restored", restored);
}

static void sim_test_update_denied(const char *name, uint16_t df, uint16_t ef) {
	static struct apdu_response response;
	uint8_t content[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;
	char check[64];

	bool selected = sim_select_ef(df, ef, &info) && sim_read_binary_at(0, info.size, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS;
	snprintf(check, sizeof(check), "READ %s", name);
	test_check(check, selected);
	if (!selected)
		return;
	memcpy(content, response.data, info.size);

	sim_update_binary(0, content, info.size, &response);
	snprintf(check, sizeof(check), "UPDATE BINARY %s with its own content", name);
	print_status(check, &response);
	snprintf(check, sizeof(check), "UPDATE BINARY %s is denied by its access condition", name);
	test_check(check, sim_status_word(&response) == GSM_STATUS_ACCESS_DENIED);
}

static bool sim_seek_record_number(uint8_t type_mode, const uint8_t *pattern, uint8_t *record) {
	static struct apdu_response response;

	if (!sim_seek(type_mode, pattern, 1, &response) || !response_has_data(response.sw1))
		return false;
	if (!sim_get_response(GSM_SEEK_RECORD_NUMBER_SIZE, &response))
		return false;
	if (sim_status_word(&response) != GSM_STATUS_SUCCESS)
		return false;
	*record = response.data[0];
	return true;
}

static uint8_t sim_find_empty_record(const struct sim_ef_info *info, const uint8_t *empty) {
	uint8_t record;
	if (!sim_seek_record_number(GSM_SEEK_TYPE_2_FROM_BEGINNING, empty, &record))
		return 0;
	for (uint16_t i = 0; i < info->records; i++) {
		if (sim_read_record_matches(record, SIM_RECORD_ABSOLUTE, empty, info->record_size))
			return record;
		if (!sim_seek_record_number(GSM_SEEK_TYPE_2_NEXT_FORWARD, empty, &record))
			return 0;
	}
	return 0;
}

static void sim_test_linear_fixed(void) {
	static struct apdu_response response;
	uint8_t empty[SIM_MAX_APDU_DATA];
	uint8_t marker[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;

	test_category("Linear fixed EF_ADN");
	bool selected = sim_select_ef(GSM_DF_TELECOM, GSM_EF_ADN, &info);
	bool usable = selected && info.structure == GSM_EF_LINEAR_FIXED && info.records >= 2;
	test_check("SELECT linear fixed EF_ADN", usable);
	if (!usable)
		return;
	printf("# EF_ADN: %u records of %u bytes\n", info.records, info.record_size);

	memset(empty, GSM_ADN_EMPTY_ALPHA, info.record_size);
	uint8_t first_match;
	bool found = sim_seek_record_number(GSM_SEEK_TYPE_2_FROM_BEGINNING, empty, &first_match);
	test_check("SEEK type 2 returns a record number", found);
	if (!found)
		return;
	bool previous_records_differ = true;
	for (uint8_t i = 1; i < first_match; i++) {
		if (sim_read_record(i, SIM_RECORD_ABSOLUTE, info.record_size, &response) &&
			response.data[0] == GSM_ADN_EMPTY_ALPHA) {
			previous_records_differ = false;
		}
	}
	printf("# First EF_ADN record without a name: %u\n", first_match);
	test_check("SEEK type 2 finds the first matching record", previous_records_differ);

	uint8_t record = sim_find_empty_record(&info, empty);
	printf("# First empty EF_ADN record: %u\n", record);
	test_check("SEEK type 2 next forward finds an empty record", record != 0);
	if (record == 0)
		return;

	memset(marker, 0xFF, info.record_size);
	marker[0] = SIM_RW_MARKER;
	bool updated = sim_update_record(record, SIM_RECORD_ABSOLUTE, marker, info.record_size, &response);
	print_status("UPDATE RECORD absolute", &response);
	test_check("UPDATE RECORD absolute succeeds", updated && sim_status_word(&response) == GSM_STATUS_SUCCESS);
	test_check("READ RECORD absolute returns the update",
		sim_read_record_matches(record, SIM_RECORD_ABSOLUTE, marker, info.record_size));

	uint8_t marked;
	bool marker_found = sim_seek_record_number(GSM_SEEK_TYPE_2_FROM_BEGINNING, marker, &marked);
	test_check("SEEK type 2 finds the updated record", marker_found && marked == record);
	test_check("READ RECORD current returns the record found by SEEK",
		sim_read_record_matches(SIM_RECORD_CURRENT, SIM_RECORD_ABSOLUTE, marker, info.record_size));

	sim_read_record(1, SIM_RECORD_ABSOLUTE, info.record_size + 1, &response);
	print_status("READ RECORD with a wrong length", &response);
	test_check("READ RECORD with a wrong length reports the record size",
		response.sw1 == GSM_SW1_WRONG_LENGTH && response.sw2 == info.record_size);
	sim_read_record(info.records + 1, SIM_RECORD_ABSOLUTE, info.record_size, &response);
	print_status("READ RECORD past the last record", &response);
	test_check("READ RECORD past the last record is out of range",
		sim_status_word(&response) == GSM_STATUS_OUT_OF_RANGE);

	uint8_t last[SIM_MAX_APDU_DATA];
	bool last_read = sim_read_record(info.records, SIM_RECORD_ABSOLUTE, info.record_size, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS;
	memcpy(last, response.data, info.record_size);
	sim_select_ef(GSM_DF_TELECOM, GSM_EF_ADN, &info);
	test_check("READ RECORD previous without a current record reads the last record",
		last_read && sim_read_record_matches(0, SIM_RECORD_PREVIOUS, last, info.record_size));
	sim_read_record(0, SIM_RECORD_NEXT, info.record_size, &response);
	print_status("READ RECORD next after the last record", &response);
	test_check("READ RECORD next does not wrap on a linear fixed EF",
		sim_status_word(&response) == GSM_STATUS_OUT_OF_RANGE);

	bool restored = sim_update_record(record, SIM_RECORD_ABSOLUTE, empty, info.record_size, &response) &&
		sim_read_record_matches(record, SIM_RECORD_ABSOLUTE, empty, info.record_size);
	test_check("EF_ADN record is restored", restored);
}

static bool sim_restore_cyclic(struct sim_ef_info *info) {
	static struct apdu_response response;

	for (uint16_t i = info->records; i-- > 0;) {
		if (!sim_update_record(0, SIM_RECORD_PREVIOUS, lnd_records[i], info->record_size, &response))
			return false;
		if (sim_status_word(&response) != GSM_STATUS_SUCCESS)
			return false;
	}
	for (uint16_t i = 0; i < info->records; i++) {
		if (!sim_read_record_matches(i + 1, SIM_RECORD_ABSOLUTE, lnd_records[i], info->record_size))
			return false;
	}
	return true;
}

static void sim_test_cyclic(void) {
	static struct apdu_response response;
	uint8_t marker[SIM_MAX_APDU_DATA];
	struct sim_ef_info info;

	test_category("Cyclic EF_LND");
	bool selected = sim_select_ef(GSM_DF_TELECOM, GSM_EF_LND, &info);
	bool usable = selected && info.structure == GSM_EF_CYCLIC && info.records >= 3 &&
		info.records <= SIM_RW_MAX_RECORDS;
	test_check("SELECT cyclic EF_LND", usable);
	if (!usable)
		return;
	printf("# EF_LND: %u records of %u bytes\n", info.records, info.record_size);

	bool saved = true;
	for (uint16_t i = 0; i < info.records && saved; i++) {
		saved = sim_read_record(i + 1, SIM_RECORD_ABSOLUTE, info.record_size, &response) &&
			sim_status_word(&response) == GSM_STATUS_SUCCESS;
		memcpy(lnd_records[i], response.data, info.record_size);
	}
	test_check("READ RECORD reads every EF_LND record", saved);
	if (!saved)
		return;

	uint16_t last = info.records - 1;
	test_check("READ RECORD next without a current record reads record 1",
		sim_read_record_matches(0, SIM_RECORD_NEXT, lnd_records[0], info.record_size));
	test_check("READ RECORD previous from record 1 wraps to the oldest record",
		sim_read_record_matches(0, SIM_RECORD_PREVIOUS, lnd_records[last], info.record_size));
	test_check("READ RECORD next from the oldest record wraps to record 1",
		sim_read_record_matches(0, SIM_RECORD_NEXT, lnd_records[0], info.record_size));
	sim_select_ef(GSM_DF_TELECOM, GSM_EF_LND, &info);
	test_check("READ RECORD previous without a current record reads the oldest record",
		sim_read_record_matches(0, SIM_RECORD_PREVIOUS, lnd_records[last], info.record_size));

	memset(marker, 0xFF, info.record_size);
	marker[0] = SIM_RW_MARKER;
	sim_update_record(0, SIM_RECORD_PREVIOUS, marker, info.record_size, &response);
	print_status("UPDATE RECORD previous", &response);
	test_check("UPDATE RECORD previous succeeds on a cyclic EF", sim_status_word(&response) == GSM_STATUS_SUCCESS);
	test_check("UPDATE RECORD previous writes record 1",
		sim_read_record_matches(1, SIM_RECORD_ABSOLUTE, marker, info.record_size));
	test_check("UPDATE RECORD previous shifts the newest record to record 2",
		sim_read_record_matches(2, SIM_RECORD_ABSOLUTE, lnd_records[0], info.record_size));

	sim_update_record(1, SIM_RECORD_ABSOLUTE, marker, info.record_size, &response);
	print_status("UPDATE RECORD absolute on a cyclic EF", &response);
	test_check("UPDATE RECORD absolute on a cyclic EF reports wrong parameters",
		sim_status_word(&response) == GSM_STATUS_WRONG_PARAMETERS);

	test_check("EF_LND is restored", sim_restore_cyclic(&info));
}

static void sim_test_increase(void) {
	static struct apdu_response response;
	uint8_t newest[SIM_MAX_APDU_DATA];
	const uint8_t zero[GSM_ACM_SIZE] = { 0 };
	struct sim_ef_info info;

	test_category("INCREASE EF_ACM");
	bool selected = sim_select_ef(GSM_DF_GSM, GSM_EF_ACM, &info) && info.structure == GSM_EF_CYCLIC &&
		info.record_size == GSM_ACM_SIZE && sim_read_record(1, SIM_RECORD_ABSOLUTE, GSM_ACM_SIZE, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS;
	test_check("READ cyclic EF_ACM", selected);
	if (!selected)
		return;
	memcpy(newest, response.data, GSM_ACM_SIZE);

	bool increased = sim_increase(zero, &response) && response_has_data(response.sw1);
	print_status("INCREASE by zero", &response);
	test_check("INCREASE by zero provides response data", increased);
	if (!increased)
		return;
	bool result = sim_get_response(GSM_INCREASE_RESPONSE_SIZE, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS &&
		memcmp(response.data, newest, GSM_ACM_SIZE) == 0 &&
		memcmp(response.data + GSM_ACM_SIZE, zero, GSM_ACM_SIZE) == 0;
	test_check("INCREASE returns the new value and the added value", result);
	test_check("INCREASE writes the new value to record 1",
		sim_read_record_matches(1, SIM_RECORD_ABSOLUTE, newest, GSM_ACM_SIZE));
	test_check("INCREASE shifts the previous value to record 2",
		sim_read_record_matches(2, SIM_RECORD_ABSOLUTE, newest, GSM_ACM_SIZE));
}

static void sim_test_increase_allowed(void) {
	struct sim_ef_info info;

	test_category("INCREASE indicator");
	bool acm = sim_select_ef(GSM_DF_GSM, GSM_EF_ACM, &info);
	printf("# EF_ACM: byte 8 %02X, access %02X %02X %02X\n", info.increase, info.access[0], info.access[1],
		info.access[2]);
	test_check("Cyclic EF_ACM reports that INCREASE is allowed", acm && (info.increase & GSM_EF_INCREASE_ALLOWED));
	bool lnd = sim_select_ef(GSM_DF_TELECOM, GSM_EF_LND, &info);
	printf("# EF_LND: byte 8 %02X, access %02X %02X %02X\n", info.increase, info.access[0], info.access[1],
		info.access[2]);
	test_check("Cyclic EF_LND reports that INCREASE is not allowed",
		lnd && (info.increase & GSM_EF_INCREASE_ALLOWED) == 0);
}

static void sim_test_invalidate_denied(void) {
	static struct apdu_response response;
	struct sim_ef_info info;

	test_category("INVALIDATE and REHABILITATE");
	bool selected = sim_select_ef(GSM_DF_TELECOM, GSM_EF_ADN, &info);
	uint8_t invalidate_access = info.access[GSM_INVALIDATE_ACCESS_BYTE] & GSM_EF_INVALIDATE_MASK;
	printf("# EF_ADN: access %02X %02X %02X, file status %02X\n", info.access[0], info.access[1], info.access[2],
		info.file_status);
	bool needs_more_than_chv1 = invalidate_access != GSM_ACCESS_ALWAYS && invalidate_access != GSM_ACCESS_CHV1;
	test_check("EF_ADN INVALIDATE needs more than CHV1", selected && needs_more_than_chv1);
	if (!selected || !needs_more_than_chv1)
		return;

	sim_invalidate(&response);
	print_status("INVALIDATE EF_ADN", &response);
	test_check("INVALIDATE EF_ADN without its access condition is denied",
		sim_status_word(&response) == GSM_STATUS_ACCESS_DENIED);
	sim_rehabilitate(&response);
	print_status("REHABILITATE EF_ADN", &response);
	test_check("REHABILITATE EF_ADN without its access condition is denied",
		sim_status_word(&response) == GSM_STATUS_ACCESS_DENIED);
}

static bool sim_authenticate(const uint8_t rand[GSM_RAND_SIZE], uint8_t result[GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE]) {
	static struct apdu_response response;

	if (!sim_run_gsm_algorithm(rand, GSM_RAND_SIZE, &response))
		return false;
	if (response.sw1 != GSM_STATUS_RESPONSE_AVAILABLE_SW1 || response.sw2 != GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE)
		return false;
	if (!sim_get_response(GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE, &response))
		return false;
	if (sim_status_word(&response) != GSM_STATUS_SUCCESS)
		return false;
	memcpy(result, response.data, GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE);
	return true;
}

static void sim_test_run_gsm_algorithm(void) {
	static struct apdu_response response;
	static const uint8_t rand[GSM_RAND_SIZE] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	};
	uint8_t first[GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE];
	uint8_t second[GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE];

	test_category("RUN GSM ALGORITHM");
	test_check("SELECT DF_GSM", sim_select_df(GSM_DF_GSM));
	bool computed = sim_authenticate(rand, first);
	test_check("RUN GSM ALGORITHM provides SRES and Kc", computed);
	bool repeated = computed && sim_authenticate(rand, second);
	test_check("RUN GSM ALGORITHM is deterministic for one RAND",
		repeated && memcmp(first, second, GSM_RUN_GSM_ALGORITHM_RESPONSE_SIZE) == 0);

	sim_run_gsm_algorithm(rand, GSM_RAND_SIZE - 1, &response);
	print_status("RUN GSM ALGORITHM with a short RAND", &response);
	test_check("RUN GSM ALGORITHM with a short RAND reports the RAND length",
		response.sw1 == GSM_SW1_WRONG_LENGTH && response.sw2 == GSM_RAND_SIZE);

	test_check("SELECT DF_TELECOM", sim_select_df(GSM_DF_TELECOM));
	sim_run_gsm_algorithm(rand, GSM_RAND_SIZE, &response);
	print_status("RUN GSM ALGORITHM in DF_TELECOM", &response);
	test_check("RUN GSM ALGORITHM outside DF_GSM reports an inconsistent file",
		sim_status_word(&response) == GSM_STATUS_FILE_INCONSISTENT);
}

static void sim_test_chv_status(void) {
	static struct apdu_response response;
	struct sim_ef_info info;

	test_category("CHV status");
	bool selected = sim_select(GSM_MF, &response) && sim_select(GSM_DF_GSM, &response) &&
		response_has_data(response.sw1) && sim_get_response(response.sw2, &response) &&
		sim_status_word(&response) == GSM_STATUS_SUCCESS;
	test_check("SELECT DF_GSM returns its status", selected);
	if (!selected)
		return;
	print_bytes("DF_GSM", response.data, response.size);

	bool initialized = true;
	for (size_t i = 0; i < GSM_DF_CHV_STATUS_COUNT; i++)
		initialized = initialized && (response.data[GSM_DF_CHV1_STATUS_OFFSET + i] & GSM_SECRET_CODE_INITIALIZED);
	test_check("CHV1, UNBLOCK CHV1, CHV2 and UNBLOCK CHV2 are initialized", initialized);

	bool chv1_disabled = (response.data[GSM_DF_CHARACTERISTICS_OFFSET] & GSM_DF_CHV1_DISABLED) != 0;
	printf("# CHV1 is %s\n", chv1_disabled ? "disabled" : "enabled");
	bool imsi = sim_select_ef(GSM_DF_GSM, GSM_EF_IMSI, &info) && sim_read_binary_at(0, info.size, &response);
	print_status("READ BINARY EF_IMSI before CHV1 verification", &response);
	uint16_t expected = chv1_disabled ? GSM_STATUS_SUCCESS : GSM_STATUS_ACCESS_DENIED;
	test_check("CHV1 protected EF_IMSI follows the CHV1 enabled indicator",
		imsi && sim_status_word(&response) == expected);
}

static void sim_test_read_write(void) {
	static uint8_t atr[SIM_MAX_ATR_SIZE];
	size_t atr_size = 0;

	bool powered = sim_power_on(SIM_CARD_RMC, atr, NULL, &atr_size);
	test_check("ATR arrives", powered);
	if (!powered)
		return;

	sim_test_transparent();
	test_category("Access conditions");
	sim_test_update_denied("EF_SST", GSM_DF_GSM, GSM_EF_SST);
	sim_test_update_denied("EF_ICCID", GSM_MF, GSM_EF_ICCID);
	sim_test_linear_fixed();
	sim_test_cyclic();
	sim_test_increase();
	sim_test_increase_allowed();
	sim_test_invalidate_denied();
	sim_test_run_gsm_algorithm();
	sim_test_chv_status();
}

#endif

int main(void) {
	uint8_t atr[SIM_MAX_ATR_SIZE];
	size_t atr_size = 0;
	struct apdu_response response;

	test_start(SIM_TEST_NAME);
	dmac_request_select(SIM_DMA_TX_REQUEST, SIM_DMA_TX_SELECT);
	dmac_request_select(SIM_DMA_RX_REQUEST, SIM_DMA_RX_SELECT);
#ifdef SIM_LATENCY_TEST
	sim_test_firmware_latency();
	sim_power_off();
	return test_finish();
#endif
#ifdef SIM_RW_TEST
	sim_test_read_write();
	sim_power_off();
	return test_finish();
#endif
	test_category("Reset values");
	SCU_RST_REQ |= SCU_RST_REQ_SIM;
	SCU_RST_REQ &= ~SCU_RST_REQ_SIM;

	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, SIM_CLC);
	test_module_id("ID", 0xF000C032, SIM_ID);

#ifdef SIM_TIMING_TEST
	cgu_fpi1_select(CGU_FPI1_OSC, 0);
	test_category("Character timing");
	for (size_t i = 0; i < ARRAY_SIZE(TIMING_RMC_VALUES); i++)
		sim_test_character_timing(TIMING_RMC_VALUES[i]);
	test_category("PPS");
	sim_test_pps();
	goto done;
#endif

	test_category("Presence and activation");
	bool powered = sim_power_on(SIM_CARD_RMC, atr, NULL, &atr_size);
#ifdef SIM_NO_CARD
	test_eq_u32("STAT.SIMDET remains clear with no card", 0, SIM_STAT & SIM_STAT_SIMDET);
	test_check("No ATR arrives without a card", !powered);
	goto done;
#else
	test_eq_u32("STAT.SIMDET remains clear with an inserted card", 0, SIM_STAT & SIM_STAT_SIMDET);
	test_check("PMIC powers card and ATR arrives", powered);
	if (!powered)
		goto done;
	print_bytes("ATR", atr, atr_size);
	test_check("ATR contains TS and T0", atr_size >= 2);
	test_check("ATR convention byte is valid", atr[0] == 0x3B || atr[0] == 0x3F);
#endif

#ifdef SIM_HARDWARE_T0
	test_category("Hardware T=0 DMA start");
	test_check("SELECT MF reaches T0END", sim_hardware_t0_select_dma());
	test_check("header and data reach DMA terminal count", (DMAC_RAW_TC_STATUS & BIT(SIM_DMA_CHANNEL)) != 0);
	test_eq_u32("T=0 DMA has no bus error", 0, DMAC_RAW_ERR_STATUS & BIT(SIM_DMA_CHANNEL));
	test_check("T0END remains visible in STAT", (SIM_STAT & SIM_STAT_T0END) != 0);
	printf("# SELECT MF hardware status: %02X%02X\n", (unsigned int) SIM_SW1, (unsigned int) SIM_SW2);
	test_check("SELECT MF succeeds", SIM_SW1 == 0x90 || SIM_SW1 == 0x9F || SIM_SW1 == 0x61);

	if (response_has_data(SIM_SW1)) {
		uint8_t response_length = SIM_SW2;
		test_category("Hardware T=0 GET RESPONSE DMA");
		bool got = sim_hardware_t0_getresponse_dma(response_length, &response);
		test_check("GET RESPONSE PER2MEM_PER DMA completes", got);
		if (got) {
			test_eq_u32("GET RESPONSE DMA has no bus error", 0,
				DMAC_RAW_ERR_STATUS & BIT(SIM_DMA_CHANNEL));
			test_eq_u32("GET RESPONSE reads the requested length", response_length, response.size);
			printf("# GET RESPONSE status: %02X%02X\n", response.sw1, response.sw2);
			test_eq_u32("GET RESPONSE status is 9000", 0x9000,
				(response.sw1 << 8) | response.sw2);
			print_bytes("MF response", response.data, response.size);
		}
	}
	goto done;
#endif

#ifdef SIM_TIMER_TEST
	test_category("Character timer");
	uint32_t elapsed_us;
	uint32_t timer_status = sim_probe_character_timer(1, &elapsed_us);
	printf("# CHTIMER=1: elapsed=%u us, STAT=%03X\n", (unsigned int) elapsed_us,
		(unsigned int) timer_status);
	test_check("CHTIMER expires while T=0 waits for the next character",
		(timer_status & SIM_STAT_CHTIMEOUT) != 0);
	goto done;
#endif

	test_category("T=0 APDU exchange");
	bool exchanged = sim_select(0x3F00, &response);
	test_check("SELECT MF exchange completes", exchanged);
	if (!exchanged)
		goto done;
	printf("# SELECT MF status: %02X%02X\n", response.sw1, response.sw2);
	test_check("SELECT MF succeeds", response.sw1 == 0x90 || response_has_data(response.sw1));
	if (response_has_data(response.sw1)) {
		exchanged = sim_get_response(response.sw2, &response);
		test_check("MF GET RESPONSE completes", exchanged);
		if (!exchanged)
			goto done;
		print_bytes("MF response", response.data, response.size);
		test_eq_u32("MF GET RESPONSE status", 0x9000, (response.sw1 << 8) | response.sw2);
	}

#ifdef SIM_DUMP_CARD_INFO
	test_category("Raw MF and DF_GSM responses");
	exchanged = sim_select(0x7F20, &response);
	test_check("SELECT DF_GSM exchange completes", exchanged);
	if (!exchanged)
		goto done;
	test_check("SELECT DF_GSM requests response data", response_has_data(response.sw1));
	if (!response_has_data(response.sw1))
		goto done;
	exchanged = sim_get_response(response.sw2, &response);
	test_check("DF_GSM GET RESPONSE completes", exchanged);
	if (!exchanged)
		goto done;
	test_eq_u32("DF_GSM GET RESPONSE status", 0x9000, (response.sw1 << 8) | response.sw2);

	exchanged = sim_status(0x16, &response);
	test_check("DF_GSM STATUS completes", exchanged);
	if (!exchanged)
		goto done;
	test_eq_u32("DF_GSM STATUS returns 22 bytes", 0x16, response.size);
	test_eq_u32("DF_GSM STATUS status", 0x9000, (response.sw1 << 8) | response.sw2);

	exchanged = sim_select(0x3F00, &response);
	test_check("SELECT MF restore completes", exchanged);
	if (!exchanged)
		goto done;
#endif

	exchanged = sim_select(0x2FE2, &response);
	test_check("SELECT EF_ICCID exchange completes", exchanged);
	if (!exchanged)
		goto done;
	printf("# SELECT EF_ICCID status: %02X%02X\n", response.sw1, response.sw2);
	test_check("SELECT EF_ICCID succeeds", response.sw1 == 0x90 || response_has_data(response.sw1));
	if (response_has_data(response.sw1)) {
		exchanged = sim_get_response(response.sw2, &response);
		test_check("EF_ICCID GET RESPONSE completes", exchanged);
		if (!exchanged)
			goto done;
		print_bytes("EF_ICCID response", response.data, response.size);
		test_eq_u32("EF_ICCID GET RESPONSE status", 0x9000, (response.sw1 << 8) | response.sw2);
	}

	exchanged = sim_read_binary(10, &response);
	test_check("READ BINARY exchange completes", exchanged);
	if (!exchanged)
		goto done;
	test_eq_u32("READ BINARY returns ten bytes", 10, response.size);
	test_eq_u32("READ BINARY status", 0x9000, (response.sw1 << 8) | response.sw2);

#ifdef SIM_DUMP_CARD_INFO
	test_category("Decoded card identity");
	dump_card_identity(response.data, response.size);
#endif

	test_category("Missing file");
	exchanged = sim_select(SIM_MISSING_FILE_ID, &response);
	test_check("SELECT of a missing EF completes", exchanged);
	if (!exchanged)
		goto done;
	printf("# SELECT missing EF %04X status: %02X%02X\n", SIM_MISSING_FILE_ID, response.sw1, response.sw2);
	test_eq_u32("SELECT of a missing EF reports GSM file ID not found", 0x9404, (response.sw1 << 8) | response.sw2);

done:
	sim_power_off();
	return test_finish();
}
