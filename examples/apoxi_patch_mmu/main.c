#include <pmb887x.h>
#include <printf.h>
#include <stdint.h>

#ifndef BOOT_EXTRAM
#undef printf
#define printf(...)
#endif

extern void _start();
extern void clean_all_caches();

enum {
	RESP_SUCCESS					= 0,
	RESP_BOOT_ALREADY_OPEN			= -1,
	RESP_UNKNOWN_FLASH				= -2,
	RESP_FLASH_BUSY					= -3,
	RESP_ERASE_ERROR				= -4,
	RESP_PROGRAM_ERROR				= -5,
	RESP_ADDR_NOT_ALIGED			= -6,
	RESP_FLASH_REGION_NOT_FOUND		= -7,
	RESP_FLASH_REGION_TOO_BIG		= -8,
	RESP_INVALID_FLASH_REGIONS		= -9,
	RESP_INVALID_FLASH_REGION_COUNT	= -10,
	RESP_UNSUPPORTED_FLASH			= -11,
	RESP_FLASH_NOT_FOUND			= -12,
	RESP_UNKNOWN					= -13,
};

typedef struct FlashEraseRegion FlashEraseRegion;

struct FlashEraseRegion {
	uint32_t addr;
	uint32_t size;
	uint32_t blocks_count;
	uint32_t erase_size;
};

static volatile uint32_t *get_mmu_table(void);
static uint32_t get_domain_access(void);
static uint32_t set_domain_access(volatile uint32_t domains);

#define PATCHER_ADDR				((uint32_t) &_start)
#define TCM_START					(0xFFFF0000)
#define TCM_END						(0xFFFF0000 + 16 * 1024)
#define PRAM_IRQ_HANDLER			(TCM_START + 0x38)

#define PARAM_OLD_IRQ_HANDLER		(PATCHER_ADDR + 4)
#define PARAM_RESPONSE_CODE			(PATCHER_ADDR + 8)

static uint32_t flash_vid;
static uint32_t flash_pid;
static uint32_t flash_unlock_addr1;
static uint32_t flash_unlock_addr2;
static uint32_t old_domain_access;
static uint32_t old_flash_mmu;
static uint32_t old_buscon;
static uint32_t max_erase_size;
static uint32_t erase_regions_cnt;
static FlashEraseRegion erase_regions[8];
static uint8_t write_buffer[128 * 1024];

typedef void (*funcp_t) (void);

__attribute__((interrupt("irq"), used))
void main(void) {
	int ret;

	clean_all_caches();

	MMIO32(PRAM_IRQ_HANDLER) = MMIO32(PARAM_OLD_IRQ_HANDLER);
	MMIO32(PARAM_RESPONSE_CODE) = RESP_UNKNOWN;

	volatile uint32_t *mmu_table = get_mmu_table();
	uint32_t old_domain_access = get_domain_access();
	set_domain_access(0xFFFFFFFF);
	mmu_table[0x00400000 >> 20] = 0x00400000 | 0xC12;

	USART_CLC(USART0) = 0x100;
	USART_FDV(USART0) = 0x000001D8;
	USART_BG(USART0) = 0x0000000C;
	USART_CON(USART0) = 0x00008811;
	USART_WHBCON(USART0) = 0x00000020;

	GPIO_PIN(GPIO_USART0_RXD) = 1;
	GPIO_PIN(GPIO_USART0_TXD) = 0x10;

	while (true) {
		USART_TXB(USART0) = 'A';
		while (!(USART_RIS(USART0) & USART_RIS_TX));
		USART_ICR(USART0) |= USART_ICR_TX;
	}

	MMIO32(PARAM_RESPONSE_CODE) = RESP_SUCCESS;

	clean_all_caches();
}

static volatile uint32_t *get_mmu_table(void) {
	volatile uint32_t *addr;
	__asm__ volatile("mrc p15, 0, %0, c2, c0, 0" : "=r" (addr));
	return addr;
}

static uint32_t get_domain_access(void) {
	volatile uint32_t domains;
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r" (domains));
	return domains;
}

static uint32_t set_domain_access(volatile uint32_t domains) {
	volatile uint32_t old_domains;
	__asm__ volatile("mrc p15, 0, %0, c3, c0, 0" : "=r" (old_domains));
	__asm__ volatile("mcr p15, 0, %0, c3, c0, 0" : : "r" (domains));
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	__asm__ volatile("NOP");
	return old_domains;
}
