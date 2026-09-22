#include <pmb887x.h>

#include "crc.h"
#include "lz4.h"

#define DTCM_ADDRESS		0x01002000
#define DTCM_REGION_SIZE_8K	(4U << 2)
#define DTCM_REGION_ENABLE	BIT(0)
#define LZ4_BLOCK_SIZE		0x00001000

enum command {
	CMD_PING = 0x41,
	CMD_INIT_EXTRAM = 0x45,
	CMD_GOTO = 0x47,
	CMD_SET_SPEED = 0x48,
	CMD_WRITE_RAM_LZ4 = 0x4C,
	CMD_RAM_WRITE = 0x57,
};

enum status {
	STATUS_FAILURE = 0x1C,
	STATUS_PONG = 0x52,
	STATUS_READY = 0xA5,
	STATUS_SUCCESS = 0xC1,
};

typedef void (*payload_entry_t)(void);

static void dtcm_enable(void) {
	uint32_t region = DTCM_ADDRESS | DTCM_REGION_SIZE_8K | DTCM_REGION_ENABLE;
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 0" : : "r" (region) : "memory");
}

static void dtcm_disable(void) {
	uint32_t region;
	__asm__ volatile("mrc p15, 0, %0, c9, c1, 0" : "=r" (region));
	region &= ~DTCM_REGION_ENABLE;
	__asm__ volatile("mcr p15, 0, %0, c9, c1, 0" : : "r" (region) : "memory");
}

static void cmd_ping(void) {
	wdt_serve();
	usart_putc(USART0, STATUS_PONG);
}

static void cmd_init_extram(void) {
	uint32_t address;
	usart_read(USART0, &address, sizeof(address));
	uint32_t size;
	usart_read(USART0, &size, sizeof(size));

	if (size < 0x00001000) {
		usart_putc(USART0, STATUS_FAILURE);
		return;
	}

	if (size > 0x08000000) {
		usart_putc(USART0, STATUS_FAILURE);
		return;
	}

	if ((size & (size - 1)) != 0) {
		usart_putc(USART0, STATUS_FAILURE);
		return;
	}

	if ((address & (size - 1)) != 0) {
		usart_putc(USART0, STATUS_FAILURE);
		return;
	}

	uint32_t mask = 27 - __builtin_ctz(size);
	EBU_ADDRSEL(1) = address | (mask << EBU_ADDRSEL_MASK_SHIFT) | EBU_ADDRSEL_REGENAB;
	EBU_BUSCON(1) = (
		EBU_BUSCON_AGEN_SDRAM_0 |
		(1U << EBU_BUSCON_PORTW_SHIFT) |
		(3U << EBU_BUSCON_BCGEN_SHIFT) |
		EBU_BUSCON_DLOAD |
		EBU_BUSCON_AALIGN
	);
	EBU_SDRMREF(0) = (6U << EBU_SDRMREF_REFRESHC_SHIFT);
	EBU_SDRMCON(0) = (
		(2U << EBU_SDRMCON_BANKM_SHIFT) |
		(1U << EBU_SDRMCON_PAGEM_SHIFT) |
		(1U << EBU_SDRMCON_CRC_SHIFT) |
		(1U << EBU_SDRMCON_AWIDTH_SHIFT) |
		(3U << EBU_SDRMCON_CRP_SHIFT) |
		(7U << EBU_SDRMCON_CRFSH_SHIFT)
	);
	EBU_SDRMOD(0) = (2U << EBU_SDRMOD_CASLAT_SHIFT) | (3U << EBU_SDRMOD_BURSTL_SHIFT);
	usart_putc(USART0, STATUS_SUCCESS);
}

static void cmd_set_speed(void) {
	uint32_t baud_rate;
	usart_read(USART0, &baud_rate, sizeof(baud_rate));
	usart_set_speed(USART0, baud_rate);
	while (usart_getc(USART0) != CMD_PING);
	cmd_ping();
}

static void cache_sync(void) {
	uint32_t value = 0;
	__asm__ volatile("1: mrc p15, 0, r15, c7, c10, 3\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"bne 1b" : : : "cc", "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
}

static void cmd_ram_write(void) {
	uint32_t address;
	usart_read(USART0, &address, sizeof(address));
	uint32_t size;
	usart_read(USART0, &size, sizeof(size));
	uint8_t *data = (uint8_t *) address;
	uint32_t offset = 0;

	while (offset < size) {
		uint32_t batch_size = MIN(size - offset, 256);
		usart_read(USART0, data + offset, batch_size);
		offset += batch_size;
		wdt_serve();
	}

	uint32_t received_crc;
	usart_read(USART0, &received_crc, sizeof(received_crc));

	uint32_t crc = CRC32_INITIAL_VALUE;
	offset = 0;
	while (offset < size) {
		uint32_t batch_size = MIN(size - offset, 256);
		crc = crc32_update(crc, data + offset, batch_size);
		offset += batch_size;
		wdt_serve();
	}

	if (received_crc != ~crc) {
		usart_putc(USART0, STATUS_FAILURE);
		return;
	}

	usart_putc(USART0, STATUS_SUCCESS);
}

static void discard_data(uint32_t size) {
	uint8_t data[256];
	uint32_t offset = 0;

	while (offset < size) {
		uint32_t batch_size = MIN(size - offset, sizeof(data));
		usart_read(USART0, data, batch_size);
		offset += batch_size;
		wdt_serve();
	}
}

static void cmd_write_ram_lz4(void) {
	uint32_t address;
	usart_read(USART0, &address, sizeof(address));
	uint32_t compressed_size;
	usart_read(USART0, &compressed_size, sizeof(compressed_size));
	uint32_t output_size;
	usart_read(USART0, &output_size, sizeof(output_size));

	uint8_t *compressed = (uint8_t *) DTCM_ADDRESS;
	bool valid;
	if (compressed_size > LZ4_BLOCK_SIZE) {
		discard_data(compressed_size);
		valid = false;
	} else if (output_size > LZ4_BLOCK_SIZE) {
		discard_data(compressed_size);
		valid = false;
	} else {
		usart_read(USART0, compressed, compressed_size);
		valid = lz4_decompress(compressed, compressed_size, (void *) address, output_size);
	}

	uint32_t received_crc;
	usart_read(USART0, &received_crc, sizeof(received_crc));
	if (valid) {
		uint32_t crc = crc32_update(CRC32_INITIAL_VALUE, (const void *) address, output_size);
		valid = received_crc == ~crc;
	}

	usart_putc(USART0, valid ? STATUS_SUCCESS : STATUS_FAILURE);
}

static void cmd_goto(void) {
	uint32_t address;
	usart_read(USART0, &address, sizeof(address));
	usart_putc(USART0, STATUS_SUCCESS);
	usart_flush(USART0);
	cache_sync();
	dtcm_disable();
	((payload_entry_t) address)();
}

int main(void) {
	dtcm_enable();
	wdt_init();
	usart_putc(USART0, STATUS_READY);

	while (1) {
		uint8_t command = usart_getc(USART0);
		wdt_serve();

		if (command == CMD_PING) {
			cmd_ping();
		} else if (command == CMD_INIT_EXTRAM) {
			cmd_init_extram();
		} else if (command == CMD_GOTO) {
			cmd_goto();
		} else if (command == CMD_SET_SPEED) {
			cmd_set_speed();
		} else if (command == CMD_WRITE_RAM_LZ4) {
			cmd_write_ram_lz4();
		} else if (command == CMD_RAM_WRITE) {
			cmd_ram_write();
		}
	}
}
