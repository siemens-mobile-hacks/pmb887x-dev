#include "reset_handler.h"

#include <pmb887x.h>

extern uint32_t _data_loadaddr, _data, _edata, _ebss;
#if defined(BOOT_EXTRAM) || defined(BOOT_FLASH)
extern uint32_t _sram_loadaddr, _sram_start, _sram_end;
#endif
#if defined(BOOT_INTRAM) || defined(BOOT_EXTRAM) || defined(BOOT_FLASH)
extern uint32_t _tcm_loadaddr, _tcm_start, _tcm_end;
#endif
extern uint32_t _vectors_table_start, _vectors_table_end;
extern funcp_t __preinit_array_start, __preinit_array_end;
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;

void __attribute__((weak)) reset_handler(void) {
	// Unmount BootROM from 0x00000000
	SCU_ROMAMCR &= ~SCU_ROMAMCR_MOUNT_BROM;

	// Enable FIFO for USART0
	USART_RXFCON(USART0) = (USART_RXFCON_RXFEN | USART_RXFCON_RXFFLU);
	USART_TXFCON(USART0) = (USART_TXFCON_TXFEN | USART_TXFCON_TXFFLU);

	volatile uint32_t *src = &_data_loadaddr;
	volatile uint32_t *dest = &_data;
	for (; dest < &_edata; src++, dest++)
		*dest = *src;

#if defined(BOOT_EXTRAM) || defined(BOOT_FLASH)
	for (src = &_sram_loadaddr, dest = &_sram_start; dest < &_sram_end; src++, dest++)
		*dest = *src;
#endif

#if defined(BOOT_INTRAM) || defined(BOOT_EXTRAM) || defined(BOOT_FLASH)
	for (src = &_tcm_loadaddr, dest = &_tcm_start; dest < &_tcm_end; src++, dest++)
		*dest = *src;

	uint32_t value = 0;
	__asm__ volatile("mcr p15, 0, %0, c7, c10, 4" : : "r" (value) : "memory");
	__asm__ volatile("mcr p15, 0, %0, c7, c5, 0" : : "r" (value) : "memory");
#endif

	dest = &_edata;
	while (dest < &_ebss)
		*dest++ = 0;

	// Copy vectors to 0x00000000
	for (src = &_vectors_table_start, dest = (volatile uint32_t *) 0; src < &_vectors_table_end; src++, dest++)
		*dest = *src;

	// Constructors
	for (volatile funcp_t *fp = &__preinit_array_start; fp < &__preinit_array_end; fp++)
		(*fp)();
	for (volatile funcp_t *fp = &__init_array_start; fp < &__init_array_end; fp++)
		(*fp)();

	// Call main
	main();

	// Destructors
	for (volatile funcp_t *fp = &__fini_array_start; fp < &__fini_array_end; fp++)
		(*fp)();

	blocking_handler();
}

__IRQ void blocking_handler(void) {
	while (1);
}

__IRQ void undef_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void swi_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void prefetch_abort_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void data_abort_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void reserved_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void irq_handler(void) __attribute__((weak, alias("blocking_handler")));
__IRQ void fiq_handler(void) __attribute__((weak, alias("blocking_handler")));
