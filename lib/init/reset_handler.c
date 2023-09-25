#include "reset_handler.h"

#include <pmb887x.h>

extern uint32_t _data_loadaddr, _data, _edata, _ebss, _stack;
extern uint32_t _vectors_table_start, _vectors_table_end, _vectors_table_handlers;
extern funcp_t __preinit_array_start, __preinit_array_end;
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;

void __attribute__ ((weak)) reset_handler(void) {
	volatile uint32_t *src, *dest;
	volatile funcp_t *fp;
	
	// Unmount BootROM from 0x00000000
	REG(0xf440007C) &= ~1;
	
	for (src = &_data_loadaddr, dest = &_data; dest < &_edata; src++, dest++)
		*dest = *src;
	
	while (dest < &_ebss)
		*dest++ = 0;
	
	// Copy vectors to 0x00000000
	for (src = &_vectors_table_start, dest = 0; src < &_vectors_table_end; src++, dest++)
		*dest = *src;
	
	// Setup handlers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
	funcp_t *cpu_vector = (funcp_t *) 0x20;
	*cpu_vector++ = reset_handler;
	*cpu_vector++ = undef_handler;
	*cpu_vector++ = swi_handler;
	*cpu_vector++ = prefetch_abort_handler;
	*cpu_vector++ = data_abort_handler;
	*cpu_vector++ = reserved_handler;
	*cpu_vector++ = irq_handler;
	*cpu_vector++ = fiq_handler;
#pragma GCC diagnostic pop 
	
	// Constructors
	for (fp = &__preinit_array_start; fp < &__preinit_array_end; fp++)
		(*fp)();
	for (fp = &__init_array_start; fp < &__init_array_end; fp++)
		(*fp)();
	
	// Call main
	main();

	// Destructors
	for (fp = &__fini_array_start; fp < &__fini_array_end; fp++)
		(*fp)();
	
	blocking_handler();
}

__IRQ void blocking_handler(void) {
	while (1);
}

#pragma weak undef_handler = blocking_handler
#pragma weak swi_handler = blocking_handler
#pragma weak prefetch_abort_handler = blocking_handler
#pragma weak data_abort_handler = blocking_handler
#pragma weak reserved_handler = blocking_handler
#pragma weak irq_handler = blocking_handler
#pragma weak fiq_handler = blocking_handler
