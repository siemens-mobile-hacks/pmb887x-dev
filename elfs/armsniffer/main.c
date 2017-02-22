#define NU_NO_ERROR_CHECKING

#define REG(addr)								(*((volatile unsigned int *) (addr)))
#define PMB8876_IRQ(n)							(REG(0xF2800030 + ((n) * 4)))

// USART
#define PMB8876_USART0_BASE   0xf1000000
#define PMB8876_USART0_CLC    PMB8876_USART0_BASE
#define PMB8876_USART0_BG     (PMB8876_USART0_BASE + 0x14)
#define PMB8876_USART0_FDV    (PMB8876_USART0_BASE + 0x18)
#define PMB8876_USART0_TXB    (PMB8876_USART0_BASE + 0x20)
#define PMB8876_USART0_RXB    (PMB8876_USART0_BASE + 0x24)
#define PMB8876_USART0_FCSTAT (PMB8876_USART0_BASE + 0x68)
#define PMB8876_USART0_ICR    (PMB8876_USART0_BASE + 0x70)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <swilib.h>
#include <nucleus/nucleus.h>

#include "sniffer/system.h"
#include "sniffer/sniffer.h"

enum {
	UART_SPEED_57600 = 0x001901d8, 
	UART_SPEED_115200 = 0x000c01d8, 
	UART_SPEED_230400 = 0x000501b4, 
	UART_SPEED_460800 = 0x00000092, 
	UART_SPEED_614400 = 0x000000c3, 
	UART_SPEED_921600 = 0x00000127, 
	UART_SPEED_1228800 = 0x0000018a, 
	UART_SPEED_1600000 = 0x00000000, 
	UART_SPEED_1500000 = 0x000001d0
};

/*
VOID TCC_Task_Sleep(UNSIGNED ticks)
__def(0x0C1C, VOID, ticks)

STATUS TCC_Create_Task(NU_TASK *task, CHAR *name, 
                        VOID (*task_entry)(UNSIGNED, VOID *), UNSIGNED argc,
                        VOID *argv, VOID *stack_address, UNSIGNED stack_size,
                        OPTION priority, UNSIGNED time_slice, 
                        OPTION preempt, OPTION auto_start)
__def(0x0C00, STATUS, task, name, task_entry, argc, argv, stack_address, stack_size, priority, time_slice, preempt, auto_start)
*/

void pmb8876_serial_set_speed(unsigned int speed) {
	REG(PMB8876_USART0_BG) = ((speed >> 16));
	REG(PMB8876_USART0_FDV) = ((speed << 16) >> 16);
}

void pmb8876_serial_putc(volatile char c) {
	REG(PMB8876_USART0_TXB) = c;
	while (!(REG(PMB8876_USART0_FCSTAT) & 2));
	REG(PMB8876_USART0_ICR) |= 2;
}

void pmb8876_serial_print(const char *data) {
	while (*data)
		pmb8876_serial_putc(*data++);
}

#pragma pack(push, 1)
struct LogEntry {
	unsigned int addr;
	unsigned int value;
	unsigned int pc;
	char type;
};
#pragma pack(pop)

struct LogEntry *buffer;
unsigned int buffer_size;
unsigned int buffer_free_pos;
unsigned int buffer_tx_pos;
unsigned int lost_frames = 0;

GBSTMR transmit_tmr;
GBSTMR start_delayed_tmr;

unsigned int get_array_idx(unsigned int offset) {
	return offset % buffer_size;
}

int allocate_buffer() {
	for (buffer_size =  300000; buffer_size > 100000; buffer_size -= 50000) {
		buffer = malloc(sizeof(struct LogEntry) * buffer_size);
		if (buffer) {
			buffer_free_pos = 0;
			buffer_tx_pos = 0;
			return 1;
		}
	}
	return 0;
}

void transmit_buffer(int i) {
	char tmp[256];
	
	if (lost_frames > 0) {
		sprintf(tmp, "LOST FRAMES: %d\n", lost_frames);
		lost_frames = 0;
	}
	
	while (buffer_tx_pos < buffer_free_pos && i) {
		struct LogEntry *log = &buffer[get_array_idx(buffer_tx_pos)];
		sprintf(tmp, "%c %08X: %08X (from %08X)\r\n", log->type ? 'R' : 'W', log->addr, log->value, log->pc);
		pmb8876_serial_print(tmp);
		++buffer_tx_pos;
		--i;
	}
}

void my_sniff_proc(unsigned int addr, unsigned int value, unsigned int pc, char is_ldr) {
	if (get_array_idx(buffer_free_pos + 1) != get_array_idx(buffer_tx_pos)) {
		struct LogEntry *log = &buffer[get_array_idx(buffer_free_pos++)];
		log->addr = addr;
		log->value = value;
		log->pc = pc;
		log->type = is_ldr;
	} else {
		++lost_frames;
	}
}

void check_buffer() {
	transmit_buffer(100);
	GBS_StartTimerProc(&transmit_tmr, 216 / 100, check_buffer);
}

void start_delayed() {
	// 1MB блок, который нужно снифать
	io_sniffer_add(0xF7100000);
}

// TODO: гуй
int main() {
	if (!allocate_buffer()) {
		ShowMSG(0, (int) "Can't allocate log buffer!");
		return 0;
	}
	
	// Убиваем все IRQ UART'а
	for (int i = 4; i <= 11; ++i)
		PMB8876_IRQ(i) = 0;
	pmb8876_serial_set_speed(UART_SPEED_1600000);
	
	check_buffer();
	
	io_sniffer_init(my_sniff_proc);
	pmb8876_serial_print("init - OK\r\n");
	
	GBS_StartTimerProc(&start_delayed_tmr, 216 / 2, start_delayed);
	
	return 0;
}
