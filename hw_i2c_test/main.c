#include <pmb887x.h>
#include <d1601aa.h>
#include <printf.h>

struct i2c_state_t {
	bool is_write;
	uint8_t addr;
	uint8_t *buffer;
	int size;
	bool addr_sent;
	int code;
};

enum {
	ERR_SUCCESS		= 0,
	ERR_BUSY		= -1,
	ERR_OVERFLOW	= -2,
	ERR_UNDERFLOW	= -3,
	ERR_NACK		= -4
};

static volatile struct i2c_state_t i2c_state = {0};

static void dump_all_regs(void) {
	printf("Dump all Dialog registers...\n");
	for (int i = 0; i <= 0xFF; ++i) {
		uint32_t v = i2c_smbus_read_byte(D1601AA_I2C_ADDR, i);
		printf("%02X: %02X\n", i, v);
		wdt_serve();
	}
}
/*

 READ[4] F4300090: 00008200 (GPIO_PIN28_I2C_SCL): IS(NONE) | OS(NONE) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(PUSHPULL) | PDPU(NONE) | ENAQ(ON) (PC: A04D1A44, LR: A04D1A44)
WRITE[4] F4300090: 00008200 (GPIO_PIN28_I2C_SCL): IS(NONE) | OS(NONE) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(PUSHPULL) | PDPU(NONE) | ENAQ(ON) (PC: A04D1A44, LR: A04D1A44)
WRITE[4] F4300090: 00001211 (GPIO_PIN28_I2C_SCL): IS(ALT0) | OS(ALT0) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(OPENDRAIN) | PDPU(NONE) | ENAQ(OFF) (PC: A04D1A44, LR: A04D1A44)
 READ[4] F4300094: 00008200 (GPIO_PIN29_I2C_SDA): IS(NONE) | OS(NONE) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(PUSHPULL) | PDPU(NONE) | ENAQ(ON) (PC: A04D1A44, LR: A04D1A44)
WRITE[4] F4300094: 00008200 (GPIO_PIN29_I2C_SDA): IS(NONE) | OS(NONE) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(PUSHPULL) | PDPU(NONE) | ENAQ(ON) (PC: A04D1A44, LR: A04D1A44)
WRITE[4] F4300094: 00001211 (GPIO_PIN29_I2C_SDA): IS(ALT0) | OS(ALT0) | PS(ALT) | DATA(HIGH) | DIR(IN) | PPEN(OPENDRAIN) | PDPU(NONE) | ENAQ(OFF) (PC: A04D1A44, LR: A04D1A44)

WRITE[4] F7600000: 00000400 (I2C_CLC): RMC(0x04) (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600010: 00000000 (I2C_RUNCTRL) (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600020: 00080000 (I2C_ADDRCFG): MnS (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600028: 00030022 (I2C_FIFOCFG): RXBS(4_WORD) | TXBS(4_WORD) | RXFA(BYTE) | TXFA(BYTE) | RXFC | TXFC (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600018: 0004003D (I2C_FDIVCFG): DEC(0x3D) | INC(0x04) (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600010: 00000001 (I2C_RUNCTRL): RUN (PC: A04F9584, LR: A04D6BA0)
* 
WRITE[4] F760008C: 0000003F (I2C_ICR): LSREQ_INT | SREQ_INT | LBREQ_INT | BREQ_INT | UNK_4 | UNK_5 (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600078: 0000007F (I2C_PIRQSC): AM | GC | MC | AL | NACK | TX_END | RX (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600068: 0000000F (I2C_ERRIRQSC): RXF_UFL | RXF_OFL | TXF_UFL | TXF_OFL (PC: A04F9584, LR: A04D6BA0)
WRITE[4] F7600084: 0000003F (I2C_IMSC): LSREQ_INT | SREQ_INT | LBREQ_INT | BREQ_INT | I2C_ERR_INT | I2C_P_INT (PC: A057A9F0, LR: A057AEE0)
WRITE[4] F7600034: 00000002 (I2C_TPSCTRL): TPS(0x02) (PC: A057AF9C, LR: A057AEE0)

 READ[4] F280001C: 0000009B (NVIC_CURRENT_IRQ): NUM(0x9B)=I2C_SINGLE_REQ (PC: 000001C8, LR: A00A2B8C)
 READ[4] F7600080: 00000001 (I2C_RIS): LSREQ_INT (PC: A057AB18, LR: 00000930)
WRITE[4] F7608000: 00000062 (I2C_TXD) (PC: A057A800, LR: A8E35D74)
WRITE[4] F760008C: 00000001 (I2C_ICR): LSREQ_INT (PC: A057AB58, LR: A8E35D74)
WRITE[4] F2800014: 00000001 (NVIC_IRQ_ACK) (PC: 00000944, LR: 00000914)

 READ[4] F280001C: 0000009E (NVIC_CURRENT_IRQ): NUM(0x9E)=I2C_PROTOCOL (PC: 000001C8, LR: A00954D4)
 READ[4] F7600074: 00000020 (I2C_PIRQSS): TX_END (PC: A057AA0C, LR: 00000930)
 READ[4] F7600074: 00000020 (I2C_PIRQSS): TX_END (PC: A057AA70, LR: 00000930)
 READ[4] F7600074: 00000020 (I2C_PIRQSS): TX_END (PC: A057AABC, LR: 00000930)
WRITE[4] F7600078: 00000020 (I2C_PIRQSC): TX_END (PC: A057A45C, LR: A057AAC8)
WRITE[4] F760008C: 00000020 (I2C_ICR): UNK_5 (PC: A057A45C, LR: A057AAC8)
WRITE[4] F760002C: 00000001 (I2C_MRPSCTRL): MRPS(0x01) (PC: A057A58C, LR: 00000001)
WRITE[4] F7600034: 00000001 (I2C_TPSCTRL): TPS(0x01) (PC: A057A58C, LR: 00000001)
 READ[4] F7600074: 00000000 (I2C_PIRQSS) (PC: A057AAC8, LR: 00000001)
*/
static void i2c_hw_init(void) {
	GPIO_PIN(GPIO_I2C_SCL) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	GPIO_PIN(GPIO_I2C_SDA) = GPIO_IS_ALT0 | GPIO_OS_ALT0 | GPIO_PPEN_OPENDRAIN | GPIO_PS_ALT | GPIO_DIR_IN;
	
	I2C_CLC = 0xFF << MOD_CLC_RMC_SHIFT;
	I2C_RUNCTRL = 0;
	I2C_ADDRCFG = I2C_ADDRCFG_MnS;
	I2C_FIFOCFG = I2C_FIFOCFG_RXBS_4_WORD | I2C_FIFOCFG_TXBS_4_WORD | I2C_FIFOCFG_RXFA_BYTE | I2C_FIFOCFG_TXFA_BYTE | I2C_FIFOCFG_RXFC | I2C_FIFOCFG_TXFC;
	I2C_FDIVCFG = (0x3D << I2C_FDIVCFG_DEC_SHIFT) | (0x04 << I2C_FDIVCFG_INC_SHIFT);
	I2C_RUNCTRL = I2C_RUNCTRL_RUN;
}	

static void i2c_hw_read_fifo(void) {
	while (I2C_FFSSTAT > 0) {
		if (!i2c_state.size) {
			printf("WTF, internal buffer underflow\n");
			while (true);
		}
		
		printf("I2C_FFSSTAT=%d\n", I2C_FFSSTAT);
		
		uint32_t value = I2C_RXD;
		int size = MIN(4, i2c_state.size);
		for (int i = 0; i < size; i++) {
			*i2c_state.buffer++ = (value >> (8 * i));
			i2c_state.size--;
		}
	}
}

__IRQ void irq_handler(void) {
	int irqn = NVIC_CURRENT_IRQ;
	
	if (irqn == NVIC_I2C_ERROR_IRQ) {
		printf("-> NVIC_I2C_ERROR_IRQ\n");
		if ((I2C_ERRIRQSS & I2C_ERRIRQSS_RXF_UFL)) {
			printf("I2C_ERRIRQSS_RXF_UFL\n");
			i2c_state.code = ERR_UNDERFLOW;
			I2C_ERRIRQSC = I2C_ERRIRQSC_RXF_UFL;
		} else if ((I2C_ERRIRQSS & I2C_ERRIRQSS_RXF_OFL)) {
			printf("I2C_ERRIRQSS_RXF_OFL\n");
			i2c_state.code = ERR_OVERFLOW;
			I2C_ERRIRQSC = I2C_ERRIRQSC_RXF_OFL;
		} else if ((I2C_ERRIRQSS & I2C_ERRIRQSS_TXF_UFL)) {
			printf("I2C_ERRIRQSS_TXF_UFL\n");
			i2c_state.code = ERR_UNDERFLOW;
			I2C_ERRIRQSC = I2C_ERRIRQSC_TXF_UFL;
		} else if ((I2C_ERRIRQSS & I2C_ERRIRQSS_TXF_OFL)) {
			printf("I2C_ERRIRQSS_TXF_OFL\n");
			i2c_state.code = ERR_OVERFLOW;
			I2C_ERRIRQSC = I2C_ERRIRQSC_TXF_OFL;
		} else {
			printf("Unknown I2C_ERRIRQSS: %08X\n", I2C_ERRIRQSS);
			while (true);
		}
	} else if (irqn == NVIC_I2C_BURST_REQ_IRQ) {
		printf("-> NVIC_I2C_BURST_REQ_IRQ\n");
		if ((I2C_RIS & I2C_RIS_BREQ_INT)) {
			i2c_hw_read_fifo();
			I2C_ICR = I2C_ICR_BREQ_INT;
		} else {
			printf("Unknown I2C_RIS: %08X\n", I2C_RIS);
			while (true);
		}
	} else if (irqn == NVIC_I2C_SINGLE_REQ_IRQ) {
		printf("-> NVIC_I2C_SINGLE_REQ_IRQ\n");
		if ((I2C_RIS & I2C_RIS_LSREQ_INT)) {
			if ((i2c_state.addr & 1)) {
				if (!i2c_state.addr_sent) {
					I2C_TXD = i2c_state.addr;
					i2c_state.addr_sent = true;
				} else {
					i2c_hw_read_fifo();
				}
			} else {
				int offset = 0;
				uint32_t value = 0;
				if (!i2c_state.addr_sent) {
					value |= i2c_state.addr;
					offset = 1;
					i2c_state.addr_sent = true;
				}
				
				int size = MIN(4 - offset, i2c_state.size);
				for (int i = 0; i < size; i++) {
					value |= *i2c_state.buffer++ << (8 * (offset + i));
					i2c_state.size--;
				}
				
				I2C_TXD = value;
			}
			I2C_ICR = I2C_ICR_LSREQ_INT;
		} else if ((I2C_RIS & I2C_RIS_LBREQ_INT)) {
			i2c_hw_read_fifo();
			I2C_ICR = I2C_ICR_LBREQ_INT;
		} else {
			printf("Unknown I2C_RIS: %08X\n", I2C_RIS);
			while (true);
		}
	} else if (irqn == NVIC_I2C_PROTOCOL_IRQ) {
		printf("-> NVIC_I2C_PROTOCOL_IRQ\n");
		if ((I2C_PIRQSS & I2C_PIRQSS_TX_END)) {
			printf("-> I2C_PIRQSS_TX_END\n");
			i2c_state.code = ERR_SUCCESS;
			I2C_PIRQSC = I2C_PIRQSC_TX_END;
		} else if ((I2C_PIRQSS & I2C_PIRQSS_RX)) {
			printf("-> I2C_PIRQSS_RX\n");
			I2C_PIRQSC = I2C_PIRQSC_RX;
		} else if ((I2C_PIRQSS & I2C_PIRQSS_NACK)) {
			printf("-> I2C_PIRQSS_NACK\n");
			I2C_PIRQSC = I2C_PIRQSC_NACK;
			i2c_state.code = ERR_NACK;
		} else {
			printf("Unknown I2C_PIRQSS: %08X\n", I2C_PIRQSS);
			while (true);
		}
	} else {
		printf("Unknown irq: %d\n", irqn);
		while (true);
	}
	
	NVIC_IRQ_ACK = 1;
}

static int hw_i2c_transfer(uint8_t addr, uint8_t *buffer, int size, bool is_write) {
	i2c_state.addr = (addr << 1) | (is_write ? 0 : 1);
	i2c_state.buffer = buffer;
	i2c_state.size = size;
	i2c_state.code = ERR_BUSY;
	i2c_state.addr_sent = false;
	
	if ((i2c_state.addr & 1)) {
		printf("i2c_read: addr: %02X, size: %d\n", addr, size);
	} else {
		printf("i2c_write: addr: %02X, data[%d]:", addr, size);
		for (int i = 0; i < size; i++)
			printf(" %02X", buffer[i]);
		printf("\n");
	}
	
	I2C_ICR = I2C_ICR_LSREQ_INT | I2C_ICR_SREQ_INT | I2C_ICR_LBREQ_INT | I2C_ICR_BREQ_INT;
	I2C_PIRQSC = I2C_PIRQSC_AM | I2C_PIRQSC_GC | I2C_PIRQSC_MC | I2C_PIRQSC_AL | I2C_PIRQSC_NACK | I2C_PIRQSC_TX_END | I2C_PIRQSC_RX;
	I2C_ERRIRQSC = I2C_ERRIRQSC_RXF_UFL | I2C_ERRIRQSC_RXF_OFL | I2C_ERRIRQSC_TXF_UFL | I2C_ERRIRQSC_TXF_OFL;
	I2C_IMSC = I2C_IMSC_LSREQ_INT | I2C_IMSC_SREQ_INT | I2C_IMSC_LBREQ_INT | I2C_IMSC_BREQ_INT | I2C_IMSC_I2C_ERR_INT | I2C_IMSC_I2C_P_INT;
	
	if ((i2c_state.addr & 1)) {
		I2C_MRPSCTRL = i2c_state.size;
		I2C_TPSCTRL = 1;
	} else {
		I2C_TPSCTRL = i2c_state.size + 1;
	}
	
	while (i2c_state.code == -1)
		wdt_serve();
	
	if ((i2c_state.addr & 1)) {
		printf("-> read:");
		for (int i = 0; i < size; i++)
			printf(" %02X", buffer[i]);
		printf("\n");
	}
	
	printf("-> done: %d\n", i2c_state.code);
	
	return i2c_state.code;
}

static int hw_i2c_write(uint8_t addr, uint8_t *buffer, int size) {
	return hw_i2c_transfer(addr, buffer, size, true);
}

static int hw_i2c_read(uint8_t addr, uint8_t *buffer, int size) {
	return hw_i2c_transfer(addr, buffer, size, false);
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
	wdt_init();
	
	cpu_enable_irq(true);
	for (int i = 0; i < 0x200; i++)
		NVIC_CON(i) = 1;
	
	i2c_hw_init();
	
	uint8_t data[64] = {0xFF};
	data[0] = 0x44;
	data[1] = 0x24;
	hw_i2c_write(D1601AA_I2C_ADDR, data, 2);
	
	hw_i2c_read(D1601AA_I2C_ADDR, data, 64);
	
	// pickoff();
	
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
