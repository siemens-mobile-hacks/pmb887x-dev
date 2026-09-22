ENTRY(start)

MEMORY {
	sram (rwx) : ORIGIN = 0x00080000, LENGTH = 96K
	tcm (rwx) : ORIGIN = 0x01000000, LENGTH = 8K
}

SECTIONS {
	/* BootROM enters the image at 0x00082000. */
	.text ORIGIN(sram) + 0x2000 : {
		KEEP(*(.boot.header))
		*(.startup)
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >sram

	/* C++ Static constructors/destructors, also used for
	 * __attribute__((constructor)) and the likes.
	 */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >sram
	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >sram
	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >sram

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >sram
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >sram

	. = ALIGN(4);
	_etext = .;

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		*(.ramtext*)	/* "text" functions to run in ram */
		. = ALIGN(4);
		_edata = .;
	} >sram
	_data_loadaddr = LOADADDR(.data);

	.tcm : {
		_tcm_start = .;
		*(.tcm)
		*(.tcm.*)
		. = ALIGN(4);
		_tcm_end = .;
	} >tcm AT>sram
	_tcm_loadaddr = LOADADDR(.tcm);

	.signature : {
		KEEP(*(.signature))
	} >sram

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

	. = ALIGN(4);
	end = .;

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }
}

/* Stacks below the image */
PROVIDE(_stack_fiq = ORIGIN(sram) + 4K);
PROVIDE(_stack_irq = ORIGIN(sram) + 8K);

/* Stacks above the image */
PROVIDE(_stack_sys = ORIGIN(sram) + LENGTH(sram));
PROVIDE(_stack_svc = _stack_sys - 4K);
PROVIDE(_stack_und = _stack_svc - 1K);
PROVIDE(_stack_abt = _stack_und - 1K);
PROVIDE(_stack_limit = _stack_abt - 1K);

PROVIDE(start = _start);
PROVIDE(boot_entry = _start);

ASSERT(end <= _stack_limit, "image overlaps SRAM stacks")
