#include <pmb887x.h>

#include "test.h"

#ifdef PMB8875

#define I2C_WAIT_ITERATIONS 300000
#define I2C_MAX_IRQS 128
#define PMIC_I2C_ADDR 0x31
#define PMIC_LED_CONTROL_REG 0x14
#define I2C_BUS_CONFIG (I2C_BUSCON_SDAEN0 | I2C_BUSCON_SCLEN0 | (0x7E << I2C_BUSCON_BRP_SHIFT) | \
	I2C_BUSCON_PREDIV_8 | I2C_BUSCON_BRPMOD_MODE1)
#define I2C_CLEAR_STATE (I2C_WHBSYSCON_CLRAL | I2C_WHBSYSCON_CLRIRQD | I2C_WHBSYSCON_CLRIRQP | \
	I2C_WHBSYSCON_CLRIRQE | I2C_WHBSYSCON_CLRRSC | I2C_WHBSYSCON_CLRBUM | I2C_WHBSYSCON_CLRACKDIS | \
	I2C_WHBSYSCON_CLRTRX | I2C_WHBSYSCON_CLRSTP)

enum transfer_result {
	TRANSFER_PENDING,
	TRANSFER_DONE,
	TRANSFER_NACK,
	TRANSFER_TIMEOUT,
};

static volatile struct transfer_state {
	const uint8_t *tx;
	uint8_t *rx;
	uint32_t remaining;
	uint32_t transferred;
	bool reading;
	bool address_sent;
	bool repeated_start;
	bool nack;
	bool end_before_data;
	bool synthetic_irq;
	enum transfer_result result;
	uint32_t data_irqs;
	uint32_t protocol_irqs;
	uint32_t end_irqs;
	uint32_t total_irqs;
} transfer;

static void set_buffer_size(uint32_t size) {
	I2C_SYSCON = (I2C_SYSCON & ~I2C_SYSCON_CI) | ((size - 1) << I2C_SYSCON_CI_SHIFT);
}

static void write_buffer(uint32_t value, uint32_t offset) {
	uint32_t count = offset;

	while (count < sizeof(value) && transfer.remaining != 0) {
		value |= (uint32_t) *transfer.tx++ << (count * 8);
		transfer.remaining--;
		transfer.transferred++;
		count++;
	}
	set_buffer_size(count);
	I2C_RTB = value;
}

static void read_buffer(void) {
	uint32_t count = (I2C_SYSCON & I2C_SYSCON_CO) >> I2C_SYSCON_CO_SHIFT;
	uint32_t requested = ((I2C_SYSCON & I2C_SYSCON_CI) >> I2C_SYSCON_CI_SHIFT) + 1;

	if (count > requested)
		count = requested;
	if (count > transfer.remaining)
		count = transfer.remaining;

	I2C_SYSCON |= I2C_SYSCON_INT;
	uint32_t value = I2C_RTB;
	I2C_SYSCON &= ~I2C_SYSCON_INT;
	for (uint32_t i = 0; i < count; i++)
		*transfer.rx++ = value >> (i * 8);
	transfer.remaining -= count;
	transfer.transferred += count;
}

static void stop_transfer(void) {
	I2C_DATA_SRC = MOD_SRC_CLRR;
	I2C_PROTO_SRC = MOD_SRC_CLRR;
	I2C_END_SRC = MOD_SRC_CLRR;
	I2C_WHBSYSCON = I2C_CLEAR_STATE;
}

static void handle_data_irq(void) {
	transfer.data_irqs++;
	I2C_DATA_SRC |= MOD_SRC_CLRR;
	if (transfer.synthetic_irq) {
		I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQD;
		return;
	}

	bool address_nack = (I2C_SYSCON & I2C_SYSCON_LRB) != 0 && (!transfer.reading || !transfer.address_sent);
	if (address_nack) {
		transfer.nack = true;
		I2C_WHBSYSCON = I2C_WHBSYSCON_CLRBUM | I2C_WHBSYSCON_CLRIRQD;
		return;
	}

	if (!transfer.reading) {
		if (transfer.remaining != 0) {
			write_buffer(0, 0);
			return;
		}
		if (transfer.repeated_start) {
			I2C_WHBSYSCON = I2C_WHBSYSCON_SETRSC;
			I2C_DATA_SRC = MOD_SRC_CLRR;
			I2C_PROTO_SRC = MOD_SRC_CLRR;
			I2C_END_SRC = MOD_SRC_CLRR;
			transfer.result = TRANSFER_DONE;
		} else {
			I2C_WHBSYSCON = I2C_WHBSYSCON_CLRBUM | I2C_WHBSYSCON_CLRIRQD;
		}
		return;
	}

	if (transfer.address_sent) {
		read_buffer();
	} else {
		transfer.address_sent = true;
		I2C_WHBSYSCON = I2C_WHBSYSCON_CLRTRX;
	}
	if (transfer.remaining != 0) {
		uint32_t count = transfer.remaining < 4 ? transfer.remaining : 4;

		set_buffer_size(count);
		if (transfer.remaining <= 4)
			I2C_WHBSYSCON = I2C_WHBSYSCON_SETACKDIS | I2C_WHBSYSCON_SETSTP;
	}
	(void) I2C_RTB;
}

static void handle_protocol_irq(void) {
	transfer.protocol_irqs++;
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQP;
	I2C_PROTO_SRC |= MOD_SRC_CLRR;
}

static void handle_end_irq(void) {
	transfer.end_irqs++;
	transfer.end_before_data |= transfer.reading && transfer.remaining != 0;
	I2C_END_SRC |= MOD_SRC_CLRR;
	I2C_WHBSYSCON = I2C_WHBSYSCON_CLRIRQE;
	if (!transfer.synthetic_irq) {
		transfer.result = transfer.nack ? TRANSFER_NACK : TRANSFER_DONE;
		stop_transfer();
	}
}

__IRQ void irq_handler(void) {
	uint32_t irq = VIC_IRQ_CURRENT;

	transfer.total_irqs++;
	if (irq == VIC_I2C_DATA_IRQ)
		handle_data_irq();
	else if (irq == VIC_I2C_PROTO_IRQ)
		handle_protocol_irq();
	else if (irq == VIC_I2C_END_IRQ)
		handle_end_irq();

	if (transfer.total_irqs > I2C_MAX_IRQS) {
		transfer.result = TRANSFER_TIMEOUT;
		stop_transfer();
	}
	VIC_IRQ_ACK = 1;
}

static enum transfer_result transfer_bytes(
	uint8_t address,
	const uint8_t *tx,
	uint8_t *rx,
	uint32_t size,
	bool repeated_start
) {
	cpu_enable_irq(false);
	transfer = (struct transfer_state) {
		.tx = tx,
		.rx = rx,
		.remaining = size,
		.reading = rx != NULL,
		.repeated_start = repeated_start,
		.result = TRANSFER_PENDING,
	};
	if (tx != NULL) {
		I2C_SYSCON = I2C_SYSCON_MOD_MASTER;
		write_buffer(address << 1, 1);
		transfer.address_sent = true;
	} else {
		set_buffer_size(1);
		I2C_RTB = (address << 1) | 1;
	}
	I2C_DATA_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_PROTO_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_END_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETBUM;
	cpu_enable_irq(true);

	for (uint32_t i = 0; i < I2C_WAIT_ITERATIONS && transfer.result == TRANSFER_PENDING; i++)
		test_spin(1);
	if (transfer.result == TRANSFER_PENDING) {
		transfer.result = TRANSFER_TIMEOUT;
		stop_transfer();
	}
	return transfer.result;
}

static enum transfer_result probe_write_address(uint8_t address) {
	uint8_t unused;

	return transfer_bytes(address, &unused, NULL, 0, false);
}

static enum transfer_result probe_read_address(uint8_t address) {
	uint8_t unused;

	return transfer_bytes(address, NULL, &unused, 0, false);
}

static enum transfer_result smbus_read(uint8_t reg, uint8_t *data, uint32_t size, bool repeated_start) {
	enum transfer_result result = transfer_bytes(PMIC_I2C_ADDR, &reg, NULL, 1, repeated_start);

	if (result == TRANSFER_DONE)
		result = transfer_bytes(PMIC_I2C_ADDR, NULL, data, size, false);
	return result;
}

static enum transfer_result smbus_write(uint8_t reg, uint8_t value) {
	uint8_t data[] = {reg, value};

	return transfer_bytes(PMIC_I2C_ADDR, data, NULL, sizeof(data), false);
}

static void configure_i2c(void) {
	I2C_CLC = MOD_CLC_DISR;
	I2C_CLC = 16 << MOD_CLC_RMC_SHIFT;
	I2C_BUSCON = I2C_BUS_CONFIG;
	I2C_SYSCON = I2C_SYSCON_MOD_MASTER;
	VIC_CON(VIC_I2C_DATA_IRQ) = 3;
	VIC_CON(VIC_I2C_PROTO_IRQ) = 2;
	VIC_CON(VIC_I2C_END_IRQ) = 1;
}

static void test_registers(void) {
	test_category("Registers");
	test_module_id("module ID", 0x00004600, I2C_ID);
	test_module_clock("module clock", I2C_CLC);

	I2C_PISEL = I2C_PISEL_SCL_IS0 | I2C_PISEL_SDA_IS1;
	test_eq_u32("input selection", I2C_PISEL_SCL_IS0 | I2C_PISEL_SDA_IS1, I2C_PISEL);
	I2C_PISEL = 0;
	I2C_BUSCON = I2C_BUS_CONFIG;
	test_eq_u32("bus configuration", I2C_BUS_CONFIG, I2C_BUSCON);
}

static void test_reset_values(void) {
	test_category("Reset values");
	test_eq_u32("CLC reset value", MOD_CLC_DISR | MOD_CLC_DISS, I2C_CLC);
	I2C_CLC = 1 << MOD_CLC_RMC_SHIFT;
	test_eq_u32("PISEL reset value", 0, I2C_PISEL);
	test_eq_u32("SYSCON reset value", 0, I2C_SYSCON);
	test_eq_u32("BUSCON reset value", 0, I2C_BUSCON);
	test_eq_u32("END SRC reset value", 0, I2C_END_SRC);
	test_eq_u32("protocol SRC reset value", 0, I2C_PROTO_SRC);
	test_eq_u32("data SRC reset value", 0, I2C_DATA_SRC);
}

static void test_hardware_bits(void) {
	test_category("Hardware bits");
	I2C_SYSCON = I2C_SYSCON_MOD_DISABLED;
	I2C_WHBSYSCON = I2C_CLEAR_STATE;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETAL | I2C_WHBSYSCON_SETRSC | I2C_WHBSYSCON_SETTRX |
		I2C_WHBSYSCON_SETSTP;
	test_eq_u32(
		"set commands",
		I2C_SYSCON_AL | I2C_SYSCON_RSC | I2C_SYSCON_TRX | I2C_SYSCON_STP,
		I2C_SYSCON & (I2C_SYSCON_AL | I2C_SYSCON_RSC | I2C_SYSCON_TRX | I2C_SYSCON_STP)
	);
	I2C_WHBSYSCON = I2C_CLEAR_STATE;
	test_eq_u32(
		"clear commands",
		0,
		I2C_SYSCON & (I2C_SYSCON_AL | I2C_SYSCON_RSC | I2C_SYSCON_TRX | I2C_SYSCON_STP)
	);
}

static void test_software_irqs(void) {
	test_category("Interrupts");
	transfer = (struct transfer_state) {.synthetic_irq = true};
	I2C_DATA_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_PROTO_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_END_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_WHBSYSCON = I2C_WHBSYSCON_SETIRQD | I2C_WHBSYSCON_SETIRQP | I2C_WHBSYSCON_SETIRQE;
	test_spin(1000);
	test_eq_u32("data IRQ routed", 1, transfer.data_irqs);
	test_eq_u32("protocol IRQ routed", 1, transfer.protocol_irqs);
	test_eq_u32("end IRQ routed", 1, transfer.end_irqs);
	test_eq_u32("IRQ flags cleared", 0, I2C_SYSCON & (I2C_SYSCON_IRQD | I2C_SYSCON_IRQP | I2C_SYSCON_IRQE));
	stop_transfer();
}

static void test_pmic(void) {
	uint8_t before;
	uint8_t after;

	test_category("PMIC SMBus");
	test_eq_u32(
		"PMIC register read completes",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &before, sizeof(before), false)
	);
	test_eq_u32("PMIC register write completes", TRANSFER_DONE, smbus_write(PMIC_LED_CONTROL_REG, before));
	test_eq_u32("PMIC write byte count", 2, transfer.transferred);
	test_eq_u32(
		"PMIC register readback completes",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &after, sizeof(after), false)
	);
	test_eq_u32("PMIC write preserves register value", before, after);
}

static void test_repeated_start(void) {
	uint8_t reg = PMIC_LED_CONTROL_REG;
	uint8_t expected;
	uint8_t actual;

	test_category("Repeated START");
	test_eq_u32(
		"STOP/START reference read completes",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &expected, sizeof(expected), false)
	);
	test_eq_u32(
		"repeated START command completes",
		TRANSFER_DONE,
		transfer_bytes(PMIC_I2C_ADDR, &reg, NULL, sizeof(reg), true)
	);
	test_check("bus is held before repeated START", (I2C_SYSCON & (I2C_SYSCON_BB | I2C_SYSCON_BUM)) != 0);
	test_eq_u32(
		"repeated START read completes",
		TRANSFER_DONE,
		transfer_bytes(PMIC_I2C_ADDR, NULL, &actual, sizeof(actual), false)
	);
	test_eq_u32("repeated START data", expected, actual);
}

static void test_packet_sizes(void) {
	static const uint16_t sizes[] = {1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 255};
	uint8_t data[256];

	test_category("Packet sizes");
	for (uint32_t i = 0; i < ARRAY_SIZE(sizes); i++) {
		test_eq_u32("SMBus read completes", TRANSFER_DONE, smbus_read(0, data, sizes[i], true));
		test_eq_u32("received byte count", sizes[i], transfer.transferred);
		test_check("DATA IRQ precedes END IRQ", !transfer.end_before_data);
	}
}

static void test_register_dump(void) {
	uint8_t registers[256];

	test_category("PMIC register dump");
	test_eq_u32("all PMIC registers are readable", TRANSFER_DONE, smbus_read(0, registers, sizeof(registers), true));
	test_eq_u32("PMIC register byte count", sizeof(registers), transfer.transferred);
	for (uint32_t row = 0; row < sizeof(registers); row += 16) {
		printf("# %02X:", row);
		for (uint32_t column = 0; column < 16; column++)
			printf(" %02X", registers[row + column]);
		printf("\n");
	}
}

static void test_scan(void) {
	bool complete = true;
	bool pmic_found = false;
	uint32_t devices = 0;

	test_category("Bus scan");
	for (uint8_t address = 0x03; address <= 0x77; address++) {
		enum transfer_result result = probe_write_address(address);

		if (result == TRANSFER_DONE) {
			printf("# found I2C device at 0x%02X\n", address);
			devices++;
			pmic_found |= address == PMIC_I2C_ADDR;
		} else if (result != TRANSFER_NACK) {
			printf("# I2C scan failed at 0x%02X: result=%u\n", address, result);
			complete = false;
		}
	}
	printf("# found %u I2C device(s)\n", devices);
	test_check("I2C scan completes", complete);
	test_check("I2C scan finds PMIC at 0x31", pmic_found);
}

static void test_recovery(void) {
	uint8_t value;

	test_category("Recovery");
	test_eq_u32("missing slave write returns NACK", TRANSFER_NACK, probe_write_address(0x7F));
	test_eq_u32("missing slave read returns NACK", TRANSFER_NACK, probe_read_address(0x7F));
	test_eq_u32(
		"PMIC read recovers after NACK",
		TRANSFER_DONE,
		smbus_read(PMIC_LED_CONTROL_REG, &value, sizeof(value), true)
	);
	test_eq_u32("bus is released", 0, I2C_SYSCON & (I2C_SYSCON_BB | I2C_SYSCON_BUM));
}

int i2c_v1_test(void) {
	test_start("I2Cv1");
	test_reset_values();
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	configure_i2c();
	cpu_enable_irq(true);

	test_registers();
	test_hardware_bits();
	test_software_irqs();
	configure_i2c();
	test_pmic();
	test_repeated_start();
	test_packet_sizes();
	test_register_dump();
	test_scan();
	test_recovery();
	return test_finish();
}

#else

int i2c_v1_test(void) {
	test_start("I2Cv1");
	test_skip("I2Cv1", "unsupported");
	return test_finish();
}

#endif
