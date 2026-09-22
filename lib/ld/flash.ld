ENTRY(start)

MEMORY {
	flash (rx) : ORIGIN = 0xA0000000, LENGTH = 1M
	sram (rwx) : ORIGIN = 0x00080000, LENGTH = 96K
	tcm (rwx) : ORIGIN = 0x01000000, LENGTH = 8K
}

SECTIONS {
	/* The flash header starts at 0xA0000000. */
	.text ORIGIN(flash) : {
		KEEP(*(.boot.header))
		*(.startup)
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >flash

	/* C++ Static constructors/destructors, also used for
	 * __attribute__((constructor)) and the likes.
	 */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >flash
	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >flash
	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >flash

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >flash
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >flash

	. = ALIGN(4);
	_etext = .;

	.sram ORIGIN(sram) + 0x4000 : {
		_sram_start = .;
		*(.sram)
		*(.sram.*)
		. = ALIGN(4);
		_sram_end = .;
	} >sram AT>flash
	_sram_loadaddr = LOADADDR(.sram);

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		*(.ramtext*)	/* "text" functions to run in ram */
		. = ALIGN(4);
		_edata = .;
	} >sram AT>flash
	_data_loadaddr = LOADADDR(.data);

	.tcm : {
		_tcm_start = .;
		*(.tcm)
		*(.tcm.*)
		. = ALIGN(4);
		_tcm_end = .;
	} >tcm AT>flash
	_tcm_loadaddr = LOADADDR(.tcm);

	.signature : {
		KEEP(*(.signature))
	} >flash

	.bss (NOLOAD) : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >sram

	/* ram, but not cleared on reset, eg boot/app comms */
	.noinit (NOLOAD) : {
		*(.noinit*)
	} >sram

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack_irq = ORIGIN(sram) + 0x4000);
PROVIDE(_stack_fiq = _stack_irq - 0x1000);
PROVIDE(_stack_abt = _stack_fiq - 0x1000);
PROVIDE(_stack_und = _stack_abt - 0x400);
PROVIDE(_stack_svc = _stack_und - 0x400);
PROVIDE(_stack_sys = _stack_svc - 0x400);
PROVIDE(start = _start);
PROVIDE(boot_entry = _start);
