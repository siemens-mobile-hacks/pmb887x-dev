#include <pmb887x.h>
#include <d1601aa.h>
#include <printf.h>

struct i2c_state_t {
	bool is_write;
	uint8_t *buffer;
	int size;
	int code;
	bool addr_sent;
	bool restart;
};

enum {
	ERR_SUCCESS		= 0,
	ERR_BUSY		= -1,
	ERR_OVERFLOW	= -2,
	ERR_UNDERFLOW	= -3,
	ERR_NACK		= -4
};

static volatile struct i2c_state_t i2c_state = {0};

/*
 * WRITE[4] F4800000: 00000100 (I2C_CLC): RMC(0x01) (PC: A0A95FD4, LR: A0A95FCC)
 * WRITE[4] F4800014: A0007E11 (I2C_BUSCON): SDAEN0 | SCLEN0 | BRP(0x7E) | PREDIV(8) | BRPMOD(MODE1) (PC: A0A95FDC, LR: A0A95FCC)
 * READ[4] F4800010: 00000000 (I2C_SYSCON): MOD(DISABLED) | CI(1) (PC: A0A95FE0, LR: A0A95FCC)
 * WRITE[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0A95FEC, LR: A0A95FCC)
 * READ[4] F48000FC: 00000000 (I2C_DATA_SRC) (PC: A0ACC830, LR: A0ACC82C)
 * WRITE[4] F48000FC: 00004000 (I2C_DATA_SRC): CLRR (PC: A0ACC838, LR: A0ACC82C)
 * WRITE[4] F48000F8: 00004000 (I2C_PROTO_SRC): CLRR (PC: A0ACC840, LR: A0ACC82C)
 * WRITE[4] F48000F4: 00004000 (I2C_ERR_SRC): CLRR (PC: A0ACC844, LR: A0ACC82C)
 * READ[4] F48000FC: 00000000 (I2C_DATA_SRC) (PC: A0ACC870, LR: A0ACC870)
 * WRITE[4] F48000FC: 00001000 (I2C_DATA_SRC): SRE (PC: A0ACC878, LR: A0ACC870)
 * WRITE[4] F48000FC: 00004000 (I2C_DATA_SRC): CLRR (PC: A0ACC8B4, LR: A0A95EFC)
 * WRITE[4] F4800014: 00000000 (I2C_BUSCON): PREDIV(1) | BRPMOD(MODE0) (PC: A0A95F04, LR: A0A95EFC)
 * WRITE[4] F4800000: 00000001 (I2C_CLC): DISR (PC: A0A95F0C, LR: A0A95EFC)
 * WRITE[4] F4800000: 00000100 (I2C_CLC): RMC(0x01) (PC: A0A95FD4, LR: A0A95FCC)
 * WRITE[4] F4800014: A0007E11 (I2C_BUSCON): SDAEN0 | SCLEN0 | BRP(0x7E) | PREDIV(8) | BRPMOD(MODE1) (PC: A0A95FDC, LR: A0A95FCC)
 * READ[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0A95FE0, LR: A0A95FCC)
 * WRITE[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0A95FEC, LR: A0A95FCC)
 * READ[4] F48000FC: 00000000 (I2C_DATA_SRC) (PC: A0ACC890, LR: A0ACCBE4)
 * WRITE[4] F48000FC: 00004000 (I2C_DATA_SRC): CLRR (PC: A0ACC898, LR: A0ACCBE4)
 * READ[4] F48000FC: 00000000 (I2C_DATA_SRC) (PC: A0ACC89C, LR: A0ACCBE4)
 * WRITE[4] F48000FC: 00001000 (I2C_DATA_SRC): SRE (PC: A0ACC8A4, LR: A0ACCBE4)
 *
 * READ[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0ACCBEC, LR: A0ACCBE4)
 * READ[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0ACCBF0, LR: A0ACCBE4)
 * READ[4] F4800010: 00080000 (I2C_SYSCON): MOD(MASTER) | CI(1) (PC: A0ACCC0C, LR: A0ACCBE4)
 *
 * WRITE[4] F4800018: 00000362 (I2C_RTB): BYTE0(0x62) | BYTE1(0x03) (PC: A0ACCCBC, LR: A0ACCBE4)
 *
 * WRITE[4] F4800010: 04880000 (I2C_SYSCON): MOD(MASTER) | TRX | CI(2) (PC: A0ACCCD4, LR: A0ACCBE4)
 *
 * WRITE[4] F4800020: 00100000 (I2C_WHBSYSCON): SETBUM (PC: A0ACCCF4, LR: A0ACCBE4)
 *
*/

static void i2c_hw_init(void) {
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	
	I2C_CLC = 16 << MOD_CLC_RMC_SHIFT;
	I2C_BUSCON = I2C_BUSCON_SDAEN0 |
		I2C_BUSCON_SCLEN0 |
		(0xFF << I2C_BUSCON_BRP_SHIFT) |
		I2C_BUSCON_PREDIV_8 |
		I2C_BUSCON_BRPMOD_MODE0;
	I2C_SYSCON = I2C_SYSCON_MOD_MASTER;
}

static void i2c_update_buffer_size(int buffer_size) {
	I2C_SYSCON = (I2C_SYSCON & ~I2C_SYSCON_CI) | ((buffer_size - 1) << I2C_SYSCON_CI_SHIFT);
}

static void i2c_hw_write_fifo(uint32_t value, int offset) {
	if (!i2c_state.size) {
		printf("WTF, internal buffer underflow\n");
		while (true);
	}
	
	int write_size = offset;
	int size = MIN(4 - offset, i2c_state.size);
	for (int i = 0; i < size; i++) {
		value |= *i2c_state.buffer++ << (8 * (offset + i));
		i2c_state.size--;
		write_size++;
	}

	i2c_update_buffer_size(write_size);
	I2C_RTB = value;
}

static int i2c_hw_get_fifo_count() {
	return ((I2C_SYSCON & I2C_SYSCON_CO) >> I2C_SYSCON_CO_SHIFT);
}

static void i2c_hw_read_fifo(void) {
	int bytes_to_read = ((I2C_SYSCON & I2C_SYSCON_CI) >> I2C_SYSCON_CI_SHIFT) + 1;
	if (i2c_state.size < bytes_to_read || bytes_to_read != i2c_hw_get_fifo_count()) {
		printf("WTF, internal buffer underflow [%d < %d != %d]\n", i2c_state.size, bytes_to_read, i2c_hw_get_fifo_count());
		while (true);
	}

	printf("i2c_hw_read_fifo: size=%d, bytes_to_read=%d, fifo=%d\n", i2c_state.size, bytes_to_read, i2c_hw_get_fifo_count());

	uint32_t value = I2C_RTB;

	int size = MIN(4, i2c_state.size);
	for (int i = 0; i < size; i++) {
		*i2c_state.buffer++ = (value >> (8 * i));
		i2c_state.size--;
		bytes_to_read--;
	}
}

static void i2c_hw_start() {
	cpu_enable_irq(false);
	I2C_DATA_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_END_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_PROTO_SRC = MOD_SRC_CLRR | MOD_SRC_SRE;
	I2C_WHBSYSCON |= I2C_WHBSYSCON_SETBUM;
	cpu_enable_irq(true);
}

static void i2c_hw_stop() {
	cpu_enable_irq(false);
	I2C_DATA_SRC = MOD_SRC_CLRR;
	I2C_END_SRC = MOD_SRC_CLRR;
	I2C_PROTO_SRC = MOD_SRC_CLRR;
	I2C_WHBSYSCON |=
		I2C_WHBSYSCON_CLRIRQD |
		I2C_WHBSYSCON_CLRIRQE |
		I2C_WHBSYSCON_CLRIRQP |
		I2C_WHBSYSCON_CLRBUM |
		I2C_WHBSYSCON_CLRTRX |
		I2C_WHBSYSCON_CLRACKDIS |
		I2C_WHBSYSCON_CLRSTP;
	cpu_enable_irq(true);
}

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;

	if (irqn == NVIC_I2C_DATA_IRQ) {
		I2C_DATA_SRC |= MOD_SRC_CLRR;

		printf("NVIC_I2C_DATA_IRQ\n");

		printf("! BUSY %d\n", I2C_SYSCON & I2C_SYSCON_BB ? 1 : 0);
		printf("! BUM %d\n", I2C_SYSCON & I2C_SYSCON_BUM ? 1 : 0);
		printf("! TRX %d\n", I2C_SYSCON & I2C_SYSCON_TRX ? 1 : 0);
		printf("! RSC %d\n", I2C_SYSCON & I2C_SYSCON_RSC ? 1 : 0);
		printf("! IRQD %d\n", I2C_SYSCON & I2C_SYSCON_IRQD ? 1 : 0);
		printf("! IRQE %d\n", I2C_SYSCON & I2C_SYSCON_IRQE ? 1 : 0);
		printf("! STP %d\n", I2C_SYSCON & I2C_SYSCON_STP ? 1 : 0);

		if (i2c_state.is_write) {
			if (i2c_state.size > 0) {
				i2c_hw_write_fifo(0, 0);
			}

			if (!i2c_state.size) {
				if (i2c_state.restart) {
					I2C_WHBSYSCON |= I2C_WHBSYSCON_SETRSC;
					I2C_DATA_SRC = MOD_SRC_CLRR;
					I2C_END_SRC = MOD_SRC_CLRR;
					I2C_PROTO_SRC = MOD_SRC_CLRR;
					i2c_state.code = ERR_SUCCESS;
				} else {
					I2C_WHBSYSCON |= I2C_WHBSYSCON_CLRBUM;
					I2C_WHBSYSCON |= I2C_WHBSYSCON_CLRIRQD;
				}
			}
		} else {
			if (i2c_state.addr_sent) {
				I2C_SYSCON |= I2C_SYSCON_INT;
				i2c_hw_read_fifo();
				I2C_SYSCON &= ~I2C_SYSCON_INT;
			} else {
				printf("addr is sent!\n");
				i2c_state.addr_sent = true;
				I2C_WHBSYSCON |= I2C_WHBSYSCON_CLRTRX;
			}

			if (i2c_state.size > 0) {
				i2c_update_buffer_size(MIN(i2c_state.size, 4));
				if (i2c_state.size <= 4) {
					I2C_WHBSYSCON |= I2C_WHBSYSCON_SETACKDIS | I2C_WHBSYSCON_SETSTP;
				}
			}

			(void) I2C_RTB; // clear IRQD?
		}
		printf("! IRQD %d\n", I2C_SYSCON & I2C_SYSCON_IRQD ? 1 : 0);
	} else if (irqn == NVIC_I2C_PROTO_IRQ) {
		I2C_WHBSYSCON |= I2C_WHBSYSCON_CLRIRQP;
		I2C_PROTO_SRC |= MOD_SRC_CLRR;
		// TODO: ????
		printf("UNHANDLED NVIC_I2C_PROTO_IRQ\n");
		while (true);
	} else if (irqn == NVIC_I2C_END_IRQ) {
		I2C_END_SRC |= MOD_SRC_CLRR;
		printf("NVIC_I2C_END_IRQ\n");

		printf("! BUSY %d\n", I2C_SYSCON & I2C_SYSCON_BB ? 1 : 0);
		printf("! BUM %d\n", I2C_SYSCON & I2C_SYSCON_BUM ? 1 : 0);
		printf("! TRX %d\n", I2C_SYSCON & I2C_SYSCON_TRX ? 1 : 0);
		printf("! RSC %d\n", I2C_SYSCON & I2C_SYSCON_RSC ? 1 : 0);
		printf("! IRQD %d\n", I2C_SYSCON & I2C_SYSCON_IRQD ? 1 : 0);
		printf("! IRQE %d\n", I2C_SYSCON & I2C_SYSCON_IRQE ? 1 : 0);
		printf("! STP %d\n", I2C_SYSCON & I2C_SYSCON_STP ? 1 : 0);

		I2C_WHBSYSCON |= I2C_WHBSYSCON_CLRIRQE;

		i2c_state.code = ERR_SUCCESS;

		i2c_hw_stop();
	} else {
		printf("UNHANDLED irq_handler=%d\n", irqn);
		while (true);
	}

	NVIC_IRQ_ACK = 1;
}

static int hw_i2c_transfer(uint8_t addr, uint8_t *buffer, int size, bool is_write, bool restart) {
	i2c_state.buffer = buffer;
	i2c_state.size = size;
	i2c_state.code = ERR_BUSY;
	i2c_state.is_write = is_write;
	i2c_state.restart = restart;

	if (i2c_state.is_write) {
		I2C_SYSCON = I2C_SYSCON_MOD_MASTER;

		printf("i2c_write: addr: %02X, data[%d]:", addr, size);
		for (int i = 0; i < size; i++)
			printf(" %02X", buffer[i]);
		printf("\n");

		i2c_state.addr_sent = true;
		i2c_hw_write_fifo((addr << 1), 1);
		i2c_hw_start();
	} else {
		printf("i2c_read: addr: %02X, size: %d\n", addr, size);

		i2c_state.addr_sent = false;
		i2c_update_buffer_size(1);
		I2C_RTB = (addr << 1) | 1;
		i2c_hw_start();
	}

	while (i2c_state.code == -1)
		wdt_serve();

	if (!i2c_state.is_write) {
		printf("-> read:");
		for (int i = 0; i < size; i++)
			printf(" %02X", buffer[i]);
		printf("\n");
	}

	printf("-> done: %d\n", i2c_state.code);

	return i2c_state.code;
}

static int hw_i2c_write(uint8_t addr, uint8_t *buffer, int size) {
	return hw_i2c_transfer(addr, buffer, size, true, false);
}

static int hw_i2c_read(uint8_t addr, uint8_t *buffer, int size) {
	return hw_i2c_transfer(addr, buffer, size, false, false);
}

static int hw_i2c_read_smbus(uint8_t addr, uint8_t reg, uint8_t *buffer, int size) {
	int ret = hw_i2c_transfer(addr, &reg, 1, true, true);
	if (ret != 0)
		return ret;
	return hw_i2c_transfer(addr, buffer, size, false, false);
}

static void poweroff(void) {
	uint8_t data[2];
	
	data[0] = 0x0E;
	data[1] = 0x14;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 2);
}

static void pickoff(void) {
	uint8_t data[2];
	
	data[0] = 0x44;
	data[1] = 0x24;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 2);
	
	data[0] = 0x46;
	data[1] = 0x5F;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 2);
	
	data[0] = 0x42;
	data[1] = 0x5F;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 2);
}

int main(void) {
	uint8_t data[0x100] = { };
	wdt_init();
	wdt_set_max_execution_time(2000);
	
	cpu_enable_irq(true);
	NVIC_CON(NVIC_I2C_DATA_IRQ) = 3;
	NVIC_CON(NVIC_I2C_PROTO_IRQ) = 2;
	NVIC_CON(NVIC_I2C_END_IRQ) = 1;

	i2c_hw_init();

	data[0] = 0x00;
	data[1] = 0x01;
	hw_i2c_read(D1601AA_I2C_ADDR, data, 2);

	pickoff();

	data[0] = 0;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 1);
	hw_i2c_read(D1601AA_I2C_ADDR, data, 1);

	printf("--------------------------------------------\n");
	
	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 1);
	printf("--------------------------------------------\n");

	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 2);
	printf("--------------------------------------------\n");

	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 3);
	printf("--------------------------------------------\n");

	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 4);
	printf("--------------------------------------------\n");

	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 5);
	printf("--------------------------------------------\n");

	hw_i2c_read_smbus(D1601AA_I2C_ADDR, 0, data, 6);
	printf("--------------------------------------------\n");

	printf("Done!\n");
	
	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
