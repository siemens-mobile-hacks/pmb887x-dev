#include <pmb887x.h>
#include <sdram.h>
#include "usb.h"
#include "crc.h"
#include "wdt.h"

enum command_t {
	CMD_PING = 0x41,
	CMD_GOTO = 0x47,
	CMD_SET_SPEED = 0x48,
	CMD_WRITE_RAM_LZ4 = 0x4C,
	CMD_RAM_WRITE = 0x57,
};

enum status_t {
	STATUS_FAILURE = 0x1C,
	STATUS_PONG = 0x52,
	STATUS_READY = 0xA5,
	STATUS_SUCCESS = 0xC1,
};

typedef void (*payload_entry_t)(void);

static void __attribute__((noreturn)) cpu_reset(void) {
	for (volatile uint32_t i = 0; i < 1000000; i++)
		__asm__ volatile ("nop");
	cpu_enable_irq(false);
	cpu_enable_fiq(false);
	wdt_set_endinit(false);
	CGU_CON1 = 0;
	CGU_CON2 = 0;
	CGU_CON3 = 0;
	while (true);
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

static void cmd_ping(void) {
	usb_send_byte(STATUS_PONG);
}

static void cmd_ram_write(void) {
	uint32_t address = usb_recv_uint32_le();
	uint32_t size = usb_recv_uint32_le();
	uint8_t *data = (uint8_t *) address;

	if (size > 0x400) {
		usb_send_byte(STATUS_FAILURE);
		cpu_reset();
		return;
	}

	usb_recv_buffer(data, size);

	uint32_t received_crc = usb_recv_uint32_le();
	uint32_t crc = crc32_update(CRC32_INITIAL_VALUE, data, size);

	if (received_crc != ~crc) {
		usb_send_byte(STATUS_FAILURE);
		cpu_reset();
		return;
	}

	usb_send_byte(STATUS_SUCCESS);
}

static void __attribute__((noreturn)) cmd_goto(void) {
	uint32_t address = usb_recv_uint32_le();
	usb_send_byte(STATUS_SUCCESS);

	for (volatile uint32_t i = 0; i < 1000000; i++)
		__asm__ volatile ("nop");

	cpu_enable_irq(false);
	cpu_enable_fiq(false);

	GPTU_CLC(GPTU0) = 0;
	GPTU_CLC(GPTU1) = 0;

	/*
	 *	SCU_RST_REQ |= SCU_RST_REQ_I2C;
	 *	GPTU_CLC(GPTU0) = 0;
	 *	GPTU_CLC(GPTU1) = 0;
	 *	I2C_CLC = 0;
	 *
	 *	uint32_t chip_id = (SCU_CHIPID & SCU_CHIPID_CHIPD) >> SCU_CHIPID_CHIPD_SHIFT;
	 *	switch (chip_id) {
	 *		case 0x1A: // PMB8875
	 *			for (int i = 0; i < 128; i++)
	 *				VIC_CON(i) = 0;
	 *			break;
	 *		case 0x1B: // PMB8876
	 *			for (int i = 0; i < 128; i++)
	 *				VIC_CON(i) = 0;
	 *			break;
}
*/
	cache_sync();
	((payload_entry_t) address)();
	while (true);
}

__attribute__((section(".startup"), noreturn))
void _start(void) {
	sdram_init();

	usb_recv_reset();
	usb_send_byte(STATUS_READY);

	while (usb_recv() != CMD_PING);
	cmd_ping();

	while (true) {
		uint8_t cmd = usb_recv();
		if (cmd == CMD_PING) {
			cmd_ping();
		} else if (cmd == CMD_GOTO) {
			cmd_goto();
		} else if (cmd == CMD_RAM_WRITE) {
			cmd_ram_write();
		} else {
			usb_send_byte(STATUS_FAILURE);
			cpu_reset();
		}
		usb_recv_reset();
	}
}
