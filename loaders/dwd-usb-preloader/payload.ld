ENTRY(_start)

MEMORY {
	payload (rwx) : ORIGIN = 0x1AE0, LENGTH = 1636
}

SECTIONS {
	.text ORIGIN(payload) : {
		KEEP(*(.boot.header))
		*(.startup)
		*(.text*)	/* Program code */
		. = ALIGN(4);
		*(.rodata*)	/* Read-only data */
		. = ALIGN(4);
	} >payload

	/* C++ Static constructors/destructors, also used for
	 * __attribute__((constructor)) and the likes.
	 */
	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
	} >payload
	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
	} >payload
	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >payload

	/*
	 * Another section used by C++ stuff, appears when using newlib with
	 * 64bit (long long) printf support
	 */
	.ARM.extab : {
		*(.ARM.extab*)
	} >payload
	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >payload

	. = ALIGN(4);
	_etext = .;

	.data : {
		_data = .;
		*(.data*)	/* Read-write initialized data */
		*(.ramtext*)	/* "text" functions to run in ram */
		. = ALIGN(4);
		_edata = .;
	} >payload
	_data_loadaddr = LOADADDR(.data);

	.signature : {
		KEEP(*(.signature))
	} >payload

	.bss (NOLOAD) : {
		*(.bss*)	/* Read-write zero initialized data */
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >payload

	/* ram, but not cleared on reset, eg boot/app comms */
	.noinit (NOLOAD) : {
		*(.noinit*)
	} >payload

	. = ALIGN(4);
	end = .;

	/*
	 * The .eh_frame section appears to be used for C++ exception handling.
	 * You may need to fix this if you're using C++.
	 */
	/DISCARD/ : { *(.eh_frame) }
}
