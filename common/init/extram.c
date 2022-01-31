char ___stack_top[512 * 1024];
char ___irq_stack_top[512 * 1024];
extern unsigned int _cpu_vectors;

typedef void (*funcp_t) (void);
extern funcp_t __preinit_array_start, __preinit_array_end;
extern funcp_t __init_array_start, __init_array_end;
extern funcp_t __fini_array_start, __fini_array_end;

/* Common symbols exported by the linker script(s): */
extern unsigned _data_loadaddr, _data, _edata, _ebss, _stack;

extern void main();

void __reset_handler() {
	// Очищаем зачем-то первый бит у SCU_ROMAMCR
	// Иначе не работает запись векторов o_O
	(*(volatile unsigned int *) (0xf440007C)) &= ~1;
	
	volatile unsigned *src, *dest;
	funcp_t *fp;

	for (src = &_data_loadaddr, dest = &_data;
		dest < &_edata;
		src++, dest++) {
		*dest = *src;
	}

	while (dest < &_ebss) {
		*dest++ = 0;
	}

	void **vectors = (void **) 0;
	for (int i = 0; i < 16; ++i)
		vectors[i] = (void *) (&_cpu_vectors)[i];
	
	/* Constructors. */
	for (fp = &__preinit_array_start; fp < &__preinit_array_end; fp++) {
		(*fp)();
	}
	for (fp = &__init_array_start; fp < &__init_array_end; fp++) {
		(*fp)();
	}

	/* Call the application's entry point. */
	(void)main();

	/* Destructors. */
	for (fp = &__fini_array_start; fp < &__fini_array_end; fp++) {
		(*fp)();
	}
}
