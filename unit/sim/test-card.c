#include <pmb887x.h>

#include "i2c.h"
#include "test.h"

#ifdef SIM_DUMP_CARD_INFO
#define SIM_TEST_NAME "SIM card info"
#else
#define SIM_TEST_NAME "SIM card"
#endif

#ifdef PMB8876

#define SIM_IRQ_MASK (SIM_IMSC_ERR | SIM_IMSC_IN | SIM_IMSC_OK)
#define SIM_MAX_ATR_SIZE 64
#define SIM_MAX_APDU_DATA 256

struct apdu_response {
	uint8_t data[SIM_MAX_APDU_DATA];
	size_t size;
	uint8_t sw1;
	uint8_t sw2;
};

static uint8_t saved_pmic_reg06;
static uint8_t saved_pmic_reg0a;
static bool pmic_state_saved;

static void delay_ms(uint32_t milliseconds) {
	stopwatch_t start = stopwatch_get();

	while (stopwatch_elapsed_ms(start) < milliseconds)
		test_watchdog_serve();
}

static bool pmic_read_byte(uint8_t reg, uint8_t *value) {
	i2c_start();
	if (i2c_write(0x31 << 1) || i2c_write(reg))
		goto no_ack;
	i2c_stop();

	i2c_start();
	if (i2c_write((0x31 << 1) | 1))
		goto no_ack;
	*value = i2c_read(true);
	i2c_stop();

	return true;

no_ack:
	i2c_stop();
	return false;
}

static bool pmic_write_byte(uint8_t reg, uint8_t value) {
	i2c_start();
	bool acknowledged = !i2c_write(0x31 << 1) && !i2c_write(reg) && !i2c_write(value);
	i2c_stop();

	return acknowledged;
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
	sim_clear_ok();

	return true;
}

static bool sim_receive_byte(uint8_t *value, uint32_t timeout_ms) {
	if (!sim_wait_ok(timeout_ms))
		return false;
	*value = SIM_RXB;
	sim_clear_ok();

	return true;
}

static bool sim_receive_procedure(uint8_t *value) {
	do {
		if (!sim_receive_byte(value, 1000))
			return false;
	} while (*value == 0x60);

	return true;
}

static bool sim_power_on(uint8_t atr[SIM_MAX_ATR_SIZE], size_t *atr_size) {
	i2c_init();
	if (!pmic_read_byte(0x06, &saved_pmic_reg06) || !pmic_read_byte(0x0A, &saved_pmic_reg0a))
		return false;
	pmic_state_saved = true;

	SIM_CLC = 4 << MOD_CLC_RMC_SHIFT;
	SIM_CON = 0;
	SIM_BRF = 0x5D;
	SIM_RXSPC = 0x28;
	SIM_IRQEN = SIM_IRQEN_ENOKINT | SIM_IRQEN_ENPAR | SIM_IRQEN_ENOVR;
	SIM_IMSC = 0;
	SIM_ICR = SIM_IRQ_MASK;
	SIM_CON = SIM_CON_SIMEN;
	if (!pmic_write_byte(0x0A, saved_pmic_reg0a) || !pmic_write_byte(0x06, saved_pmic_reg06 | BIT(4)))
		return false;
	GPIO_PIN(GPIO_I2C_SCL) = 0x1211;
	GPIO_PIN(GPIO_I2C_SDA) = 0x1211;

	delay_ms(255);
	SIM_CON |= SIM_CON_SIMIOL | SIM_CON_ERROFF;
	SIM_CON |= SIM_CON_SIMON;
	delay_ms(100);
	SIM_CON |= SIM_CON_UARTON;
	delay_ms(5);
	SIM_CON |= SIM_CON_SIMRST;

	*atr_size = 0;
	while (*atr_size < SIM_MAX_ATR_SIZE &&
		sim_receive_byte(&atr[*atr_size], *atr_size == 0 ? 1000 : 100)) {
		(*atr_size)++;
	}

	return *atr_size != 0;
}

static void sim_power_off(void) {
	SIM_DMAE = 0;
	SIM_CON = 0;
	SIM_IRQEN = 0;
	SIM_ICR = SIM_IRQ_MASK;
	if (pmic_state_saved) {
		i2c_init();
		pmic_write_byte(0x06, saved_pmic_reg06);
	}
}

static bool sim_exchange(
	const uint8_t header[5],
	const uint8_t *tx_data,
	size_t tx_size,
	size_t rx_size,
	struct apdu_response *response
) {
	response->size = 0;
	for (size_t i = 0; i < 5; i++) {
		if (!sim_send_byte(header[i]))
			return false;
	}

	size_t transferred = 0;
	while (transferred < tx_size || transferred < rx_size) {
		uint8_t procedure;
		if (!sim_receive_procedure(&procedure))
			return false;
		size_t chunk;
		if (procedure == header[1]) {
			chunk = tx_size != 0 ? tx_size - transferred : rx_size - transferred;
		} else if (procedure == (uint8_t) ~header[1]) {
			chunk = 1;
		} else {
			return false;
		}
		for (size_t i = 0; i < chunk; i++) {
			if (tx_size != 0) {
				if (!sim_send_byte(tx_data[transferred]))
					return false;
			} else {
				if (!sim_receive_byte(&response->data[transferred], 1000))
					return false;
			}
			transferred++;
		}
	}
	response->size = rx_size;
	if (!sim_receive_procedure(&response->sw1) || !sim_receive_byte(&response->sw2, 1000))
		return false;

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

static bool sim_read_record(uint8_t record, uint8_t length, struct apdu_response *response) {
	const uint8_t header[5] = {0xA0, 0xB2, record, 0x04, length};

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
		if (data[i] >= 0x20 && data[i] <= 0x7E)
			printf("%c", data[i]);
		else
			printf("\\x%02X", data[i]);
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
		if (!sim_read_record(i, record_size, &record) || record.sw1 != 0x90 || record.sw2 != 0)
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

int main(void) {
	uint8_t atr[SIM_MAX_ATR_SIZE];
	size_t atr_size = 0;
	struct apdu_response response;

	test_start(SIM_TEST_NAME);
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, SIM_CLC);
	test_module_id("ID", 0xF000C032, SIM_ID);

	test_category("Activation and ATR");
	bool powered = sim_power_on(atr, &atr_size);
	test_check("PMIC powers card and ATR arrives", powered);
	if (!powered)
		goto done;
	print_bytes("ATR", atr, atr_size);
	test_check("ATR contains TS and T0", atr_size >= 2);
	test_check("ATR convention byte is valid", atr[0] == 0x3B || atr[0] == 0x3F);

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

done:
	sim_power_off();
	return test_finish();
}

#else

int main(void) {
	test_start(SIM_TEST_NAME);
	test_skip(SIM_TEST_NAME, "is only available on PMB8876");

	return test_finish();
}

#endif
