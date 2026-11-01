#include <pmb887x.h>
#if defined(BOARD_HAS_PMIC_PMB6812)
#include <pmic/PMB6812.h>
#elif defined(BOARD_HAS_PMIC_D1094XX)
#include <pmic/PASIC.h>
#endif

#include <string.h>

#include "i2c.h"
#include "pmb681x.h"

#define SIM_IRQ_MASK (SIM_IMSC_ERR | SIM_IMSC_IN | SIM_IMSC_OK)
#define SIM_DEFAULT_BRF 0x5D
#define SIM_RMC 4
#define SIM_MAX_ATR_SIZE 64
#define SIM_MAX_DATA_SIZE 256
#define SIM_STATUS_SIZE 2
#define SIM_HEADER_SIZE 5
#define SIM_HEADER_INS 1
#define SIM_HEADER_P3 4
#define SIM_NULL_BYTE 0x60
#define SIM_TX_TIMEOUT_MS 500
#define SIM_FIRST_ATR_TIMEOUT_MS 1000
#define SIM_NEXT_ATR_TIMEOUT_MS 100
#define SIM_RESPONSE_TIMEOUT_MS 3000
#define SIM_TURNAROUND_MS 5
#define SIM_SUPPLY_SETTLE_MS 255
#define SIM_CLOCK_SETTLE_MS 100
#define SIM_UART_SETTLE_MS 5

#define SIM_PPSS 0xFF
#define SIM_DEFAULT_FIDI 0x11
#define SIM_FIDI_FI_SHIFT 4
#define SIM_FIDI_DI_MASK 0x0F
#define SIM_CLOCKS_PER_BRF_ETU 4

#define HOST_SPEED 115200
#define SERIAL_SYNC 0x03
#define SERIAL_ACK 0x06
#define SERIAL_NAK 0x15

#define CCID_HEADER_SIZE 10
#define CCID_MAX_DATA_SIZE 261
#define CCID_OFFSET_TYPE 0
#define CCID_OFFSET_LENGTH 1
#define CCID_OFFSET_SLOT 5
#define CCID_OFFSET_SEQ 6
#define CCID_OFFSET_STATUS 7
#define CCID_OFFSET_ERROR 8
#define CCID_OFFSET_SPECIFIC 9
#define CCID_OFFSET_PROTOCOL 7

#define PC_TO_RDR_SET_PARAMETERS 0x61
#define PC_TO_RDR_ICC_POWER_ON 0x62
#define PC_TO_RDR_ICC_POWER_OFF 0x63
#define PC_TO_RDR_GET_SLOT_STATUS 0x65
#define PC_TO_RDR_ESCAPE 0x6B
#define PC_TO_RDR_GET_PARAMETERS 0x6C
#define PC_TO_RDR_RESET_PARAMETERS 0x6D
#define PC_TO_RDR_XFR_BLOCK 0x6F
#define RDR_TO_PC_DATA_BLOCK 0x80
#define RDR_TO_PC_SLOT_STATUS 0x81
#define RDR_TO_PC_PARAMETERS 0x82
#define RDR_TO_PC_ESCAPE 0x83

#define CCID_ICC_ACTIVE 0x00
#define CCID_ICC_INACTIVE 0x01
#define CCID_COMMAND_FAILED 0x40
#define CCID_ERROR_CMD_NOT_SUPPORTED 0x00
#define CCID_ERROR_BAD_FIDI 10
#define CCID_ERROR_HW_ERROR 0xFB
#define CCID_ERROR_ICC_MUTE 0xFE
#define CCID_T0_PARAMETERS_SIZE 5
#define CCID_T0_DEFAULT_WAITING_INTEGER 0x0A
#define CCID_T0_FIDI 0

#define ESCAPE_GET_FIRMWARE 0x02
#define ESCAPE_POWER_OFF 0xF0

struct ccid_message {
	uint8_t header[CCID_HEADER_SIZE];
	uint8_t data[CCID_MAX_DATA_SIZE];
	size_t size;
};

static const uint16_t FI_CLOCKS_PER_ETU[16] = {
	372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048
};
static const uint8_t DI_DIVISORS[16] = { 0, 1, 2, 4, 8, 16, 32, 64, 12, 20 };
static const uint8_t DEFAULT_T0_PARAMETERS[CCID_T0_PARAMETERS_SIZE] = {
	SIM_DEFAULT_FIDI, 0, 0, CCID_T0_DEFAULT_WAITING_INTEGER, 0
};
static const char FIRMWARE[] = "PMB887X SIM reader";

static uint8_t t0_parameters[CCID_T0_PARAMETERS_SIZE];
static bool icc_active;

static void delay_ms(uint32_t milliseconds) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < milliseconds)
		wdt_serve();
}

static bool sim_wait_ok(uint32_t timeout_ms) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < timeout_ms) {
		if (SIM_RIS & SIM_RIS_ERR) {
			SIM_ICR = SIM_ICR_ERR;
			return false;
		}
		if (SIM_RIS & SIM_RIS_OK)
			return true;
		wdt_serve();
	}
	return false;
}

static bool sim_send_byte(uint8_t value) {
	SIM_ICR = SIM_ICR_OK;
	SIM_TXB = value;
	if (!sim_wait_ok(SIM_TX_TIMEOUT_MS))
		return false;
	SIM_ICR = SIM_ICR_OK;
	return true;
}

static bool sim_receive_byte(uint8_t *value, uint32_t timeout_ms) {
	if (!sim_wait_ok(timeout_ms))
		return false;
	*value = SIM_RXB;
	SIM_ICR = SIM_ICR_OK;
	return true;
}

static bool sim_supply_set(bool _enabled __attribute__((unused))) {
#if defined(BOARD_HAS_PMIC_PMB6812)
	uint8_t control;

	if (!i2c_smbus_read_byte(PMB6812_I2C_ADDR, PMB6812_PWCTRL5, &control))
		return false;
	control &= ~PMB6812_PWCTRL5_LSIMMD;
	control |= _enabled ? PMB6812_PWCTRL5_LSIMMD_ON : PMB6812_PWCTRL5_LSIMMD_OFF;
	return pmb681x_reg_write(PMB6812_PWCTRL5, control);
#elif defined(BOARD_HAS_PMIC_D1094XX)
	uint8_t supply_enable;

	if (!i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1, &supply_enable))
		return false;
	if (_enabled) {
		supply_enable |= PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN;
	} else {
		supply_enable &= ~PASIC_SUPPLY_ENABLE_1_VAUDREGA_EN;
	}
	return i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_SUPPLY_ENABLE_1, supply_enable);
#else
	return true;
#endif
}

static void sim_power_off(void) {
	SIM_CON = 0;
	SIM_IRQEN = 0;
	SIM_ICR = SIM_IRQ_MASK;
	i2c_init();
	sim_supply_set(false);
}

static size_t sim_power_on(uint8_t atr[SIM_MAX_ATR_SIZE]) {
	sim_power_off();
	SIM_BRF = SIM_DEFAULT_BRF;
	SIM_IRQEN = SIM_IRQEN_ENOKINT | SIM_IRQEN_ENPAR | SIM_IRQEN_ENOVR;
	SIM_IMSC = 0;
	SIM_ICR = SIM_IRQ_MASK;
	SIM_CON = SIM_CON_SIMEN;
	if (!sim_supply_set(true))
		return 0;

	delay_ms(SIM_SUPPLY_SETTLE_MS);
	SIM_CON |= SIM_CON_SIMVCC;
	SIM_CON |= SIM_CON_SIMIOL | SIM_CON_ERROFF;
	SIM_CON |= SIM_CON_SIMON;
	delay_ms(SIM_CLOCK_SETTLE_MS);
	SIM_CON |= SIM_CON_UARTON;
	delay_ms(SIM_UART_SETTLE_MS);
	SIM_CON |= SIM_CON_SIMRST;

	size_t size = 0;
	while (size < SIM_MAX_ATR_SIZE) {
		uint32_t timeout_ms = size == 0 ? SIM_FIRST_ATR_TIMEOUT_MS : SIM_NEXT_ATR_TIMEOUT_MS;
		if (!sim_receive_byte(&atr[size], timeout_ms))
			break;
		size++;
	}
	return size;
}

static bool sim_receive_procedure(uint8_t *value) {
	do {
		if (!sim_receive_byte(value, SIM_RESPONSE_TIMEOUT_MS))
			return false;
	} while (*value == SIM_NULL_BYTE);
	return true;
}

static size_t sim_header_to_data_size(const uint8_t header[SIM_HEADER_SIZE]) {
	return header[SIM_HEADER_P3] != 0 ? header[SIM_HEADER_P3] : SIM_MAX_DATA_SIZE;
}

static bool sim_exchange_tpdu(const uint8_t *tpdu, size_t tpdu_size, uint8_t *response, size_t *response_size) {
	uint8_t ins = tpdu[SIM_HEADER_INS];
	const uint8_t *tx_data = tpdu_size > SIM_HEADER_SIZE ? tpdu + SIM_HEADER_SIZE : NULL;
	size_t size = tx_data ? tpdu_size - SIM_HEADER_SIZE : sim_header_to_data_size(tpdu);
	size_t transferred = 0;

	*response_size = 0;
	delay_ms(SIM_TURNAROUND_MS);
	for (size_t i = 0; i < SIM_HEADER_SIZE; i++) {
		if (!sim_send_byte(tpdu[i]))
			return false;
	}

	while (true) {
		uint8_t procedure;
		if (!sim_receive_procedure(&procedure))
			return false;
		size_t chunk;
		if (procedure == ins) {
			chunk = size - transferred;
		} else if (procedure == (uint8_t) ~ins) {
			chunk = transferred < size ? 1 : 0;
		} else {
			response[(*response_size)++] = procedure;
			return sim_receive_byte(&response[(*response_size)++], SIM_RESPONSE_TIMEOUT_MS);
		}
		for (size_t i = 0; i < chunk; i++) {
			if (tx_data) {
				if (!sim_send_byte(tx_data[transferred]))
					return false;
			} else {
				if (!sim_receive_byte(&response[(*response_size)++], SIM_RESPONSE_TIMEOUT_MS))
					return false;
			}
			transferred++;
		}
	}
}

static bool sim_exchange_pps(const uint8_t *request, size_t size, uint8_t *response) {
	delay_ms(SIM_TURNAROUND_MS);
	for (size_t i = 0; i < size; i++) {
		if (!sim_send_byte(request[i]))
			return false;
	}
	for (size_t i = 0; i < size; i++) {
		if (!sim_receive_byte(&response[i], SIM_RESPONSE_TIMEOUT_MS))
			return false;
	}
	return true;
}

static uint32_t sim_fidi_to_brf(uint8_t fidi) {
	uint32_t divisor = DI_DIVISORS[fidi & SIM_FIDI_DI_MASK];
	if (divisor == 0)
		return 0;
	return FI_CLOCKS_PER_ETU[fidi >> SIM_FIDI_FI_SHIFT] / divisor / SIM_CLOCKS_PER_BRF_ETU;
}

static uint8_t host_read_byte(void) {
	while (usart_get_rx_fifo_level(USART0) == 0)
		wdt_serve();
	return usart_getc(USART0);
}

static void host_write_frame(const uint8_t header[CCID_HEADER_SIZE], const uint8_t *data, size_t size) {
	uint8_t lrc = SERIAL_SYNC ^ SERIAL_ACK;
	for (size_t i = 0; i < CCID_HEADER_SIZE; i++)
		lrc ^= header[i];
	for (size_t i = 0; i < size; i++)
		lrc ^= data[i];

	usart_putc(USART0, SERIAL_SYNC);
	usart_putc(USART0, SERIAL_ACK);
	usart_write(USART0, header, CCID_HEADER_SIZE);
	usart_write(USART0, data, size);
	usart_putc(USART0, lrc);
}

static void host_write_nak(void) {
	usart_putc(USART0, SERIAL_SYNC);
	usart_putc(USART0, SERIAL_NAK);
	usart_putc(USART0, SERIAL_SYNC ^ SERIAL_NAK);
}

static bool host_read_message(struct ccid_message *message) {
	while (host_read_byte() != SERIAL_SYNC)
		;
	if (host_read_byte() != SERIAL_ACK)
		return false;

	uint8_t lrc = SERIAL_SYNC ^ SERIAL_ACK;
	for (size_t i = 0; i < CCID_HEADER_SIZE; i++) {
		message->header[i] = host_read_byte();
		lrc ^= message->header[i];
	}
	const uint8_t *length = &message->header[CCID_OFFSET_LENGTH];
	uint32_t size = length[0] | (length[1] << 8) | (length[2] << 16) | ((uint32_t) length[3] << 24);
	if (size > CCID_MAX_DATA_SIZE)
		return false;
	for (size_t i = 0; i < size; i++) {
		message->data[i] = host_read_byte();
		lrc ^= message->data[i];
	}
	message->size = size;
	return host_read_byte() == lrc;
}

static void host_echo_message(const struct ccid_message *message) {
	host_write_frame(message->header, message->data, message->size);
}

static void host_reply(
	const struct ccid_message *request,
	uint8_t type,
	bool failed,
	uint8_t error,
	uint8_t specific,
	const uint8_t *data,
	size_t size
) {
	uint8_t status = icc_active ? CCID_ICC_ACTIVE : CCID_ICC_INACTIVE;
	uint8_t header[CCID_HEADER_SIZE] = {
		[CCID_OFFSET_TYPE] = type,
		[CCID_OFFSET_LENGTH] = (uint8_t) size,
		[CCID_OFFSET_LENGTH + 1] = size >> 8,
		[CCID_OFFSET_SLOT] = request->header[CCID_OFFSET_SLOT],
		[CCID_OFFSET_SEQ] = request->header[CCID_OFFSET_SEQ],
		[CCID_OFFSET_STATUS] = failed ? (status | CCID_COMMAND_FAILED) : status,
		[CCID_OFFSET_ERROR] = error,
		[CCID_OFFSET_SPECIFIC] = specific,
	};
	host_write_frame(header, data, size);
}

static void host_reply_slot_status(const struct ccid_message *request, bool failed, uint8_t error) {
	host_reply(request, RDR_TO_PC_SLOT_STATUS, failed, error, 0, NULL, 0);
}

static void host_reply_parameters(const struct ccid_message *request, bool failed, uint8_t error) {
	host_reply(request, RDR_TO_PC_PARAMETERS, failed, error, 0, t0_parameters, sizeof(t0_parameters));
}

static void t0_parameters_reset(void) {
	memcpy(t0_parameters, DEFAULT_T0_PARAMETERS, sizeof(t0_parameters));
	SIM_BRF = SIM_DEFAULT_BRF;
}

static void host_handle_power_on(const struct ccid_message *request) {
	static uint8_t atr[SIM_MAX_ATR_SIZE];

	t0_parameters_reset();
	size_t size = sim_power_on(atr);
	icc_active = size != 0;
	if (!icc_active) {
		host_reply(request, RDR_TO_PC_DATA_BLOCK, true, CCID_ERROR_ICC_MUTE, 0, NULL, 0);
		return;
	}
	host_reply(request, RDR_TO_PC_DATA_BLOCK, false, 0, 0, atr, size);
}

static void host_handle_power_off(const struct ccid_message *request) {
	sim_power_off();
	icc_active = false;
	host_reply_slot_status(request, false, 0);
}

static void host_handle_xfr_block(const struct ccid_message *request) {
	static uint8_t response[SIM_MAX_DATA_SIZE + SIM_STATUS_SIZE];
	size_t size = 0;
	bool exchanged;
	bool is_pps = request->size != 0 && request->data[0] == SIM_PPSS;

	if (!icc_active || (!is_pps && request->size < SIM_HEADER_SIZE)) {
		host_reply(request, RDR_TO_PC_DATA_BLOCK, true, CCID_ERROR_HW_ERROR, 0, NULL, 0);
		return;
	}
	if (is_pps) {
		size = request->size;
		exchanged = sim_exchange_pps(request->data, size, response);
	} else {
		exchanged = sim_exchange_tpdu(request->data, request->size, response, &size);
	}
	if (!exchanged) {
		host_reply(request, RDR_TO_PC_DATA_BLOCK, true, CCID_ERROR_ICC_MUTE, 0, NULL, 0);
		return;
	}
	host_reply(request, RDR_TO_PC_DATA_BLOCK, false, 0, 0, response, size);
}

static void host_handle_set_parameters(const struct ccid_message *request) {
	bool is_t0 = request->header[CCID_OFFSET_PROTOCOL] == 0 && request->size == CCID_T0_PARAMETERS_SIZE;
	uint32_t brf = is_t0 ? sim_fidi_to_brf(request->data[CCID_T0_FIDI]) : 0;

	if (brf == 0) {
		host_reply_parameters(request, true, CCID_ERROR_BAD_FIDI);
		return;
	}
	memcpy(t0_parameters, request->data, sizeof(t0_parameters));
	SIM_BRF = brf;
	host_reply_parameters(request, false, 0);
}

static void phone_power_off(void) {
	sim_power_off();
#if defined(BOARD_HAS_PMIC_D1094XX)
	uint8_t power;
	if (i2c_smbus_read_byte(PASIC_I2C_ADDR, PASIC_POWER, &power))
		i2c_smbus_write_byte(PASIC_I2C_ADDR, PASIC_POWER, power | PASIC_POWER_POWEROFF);
#endif
	while (true)
		;
}

static void host_handle_escape(const struct ccid_message *request) {
	uint8_t command = request->size != 0 ? request->data[0] : 0;

	if (command == ESCAPE_GET_FIRMWARE) {
		host_reply(request, RDR_TO_PC_ESCAPE, false, 0, 0, (const uint8_t *) FIRMWARE, sizeof(FIRMWARE) - 1);
		return;
	}
	host_reply(request, RDR_TO_PC_ESCAPE, false, 0, 0, NULL, 0);
	if (command == ESCAPE_POWER_OFF) {
		usart_flush(USART0);
		phone_power_off();
	}
}

static void host_handle_message(const struct ccid_message *request) {
	switch (request->header[CCID_OFFSET_TYPE]) {
		case PC_TO_RDR_ICC_POWER_ON:
			host_handle_power_on(request);
			break;
		case PC_TO_RDR_ICC_POWER_OFF:
			host_handle_power_off(request);
			break;
		case PC_TO_RDR_GET_SLOT_STATUS:
			host_reply_slot_status(request, false, 0);
			break;
		case PC_TO_RDR_XFR_BLOCK:
			host_handle_xfr_block(request);
			break;
		case PC_TO_RDR_GET_PARAMETERS:
			host_reply_parameters(request, false, 0);
			break;
		case PC_TO_RDR_RESET_PARAMETERS:
			t0_parameters_reset();
			host_reply_parameters(request, false, 0);
			break;
		case PC_TO_RDR_SET_PARAMETERS:
			host_handle_set_parameters(request);
			break;
		case PC_TO_RDR_ESCAPE:
			host_handle_escape(request);
			break;
		default:
			host_reply_slot_status(request, true, CCID_ERROR_CMD_NOT_SUPPORTED);
			break;
	}
}

int main(void) {
	static struct ccid_message request;

	wdt_init();
	SIM_CLC = SIM_RMC << MOD_CLC_RMC_SHIFT;
	t0_parameters_reset();

	printf("SIM reader ready\n");
	usart_putc(USART0, 0);
	usart_flush(USART0);
	usart_set_speed(USART0, HOST_SPEED);
	USART_CON(USART0) = (USART_CON(USART0) & ~USART_CON_STP) | USART_CON_STP_TWO;

	while (true) {
		if (!host_read_message(&request)) {
			host_write_nak();
			continue;
		}
		host_echo_message(&request);
		host_handle_message(&request);
	}
}
