ENTRY(pll_loader)

MEMORY {
	extram (rwx) : ORIGIN = 0xA8000000, LENGTH = 8M
	intram (rwx) : ORIGIN = 0x00082000, LENGTH = 96k-0x2000
}

SECTIONS {
	.pll_loader : {
		KEEP(*(.pll_loader))
		. = ALIGN(4);
	} >extram

	.text : {
		_intram_start = .;
		*(.startup)
		*(.text*)
		. = ALIGN(4);
		*(.rodata*)
		. = ALIGN(4);
	} >intram AT>extram

	.preinit_array : {
		. = ALIGN(4);
		__preinit_array_start = .;
		KEEP(*(.preinit_array))
		__preinit_array_end = .;
	} >intram AT>extram

	.init_array : {
		. = ALIGN(4);
		__init_array_start = .;
		KEEP(*(SORT(.init_array.*)))
		KEEP(*(.init_array))
		__init_array_end = .;
	} >intram AT>extram

	.fini_array : {
		. = ALIGN(4);
		__fini_array_start = .;
		KEEP(*(.fini_array))
		KEEP(*(SORT(.fini_array.*)))
		__fini_array_end = .;
	} >intram AT>extram

	.ARM.extab : {
		*(.ARM.extab*)
	} >intram AT>extram

	.ARM.exidx : {
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
	} >intram AT>extram

	. = ALIGN(4);
	_etext = .;

	.data : {
		_data = .;
		*(.data*)
		*(.ramtext*)
		. = ALIGN(4);
		_edata = .;
	} >intram AT>extram
	_data_loadaddr = _data;
	_intram_loadaddr = LOADADDR(.text);
	_intram_loadend = LOADADDR(.data) + SIZEOF(.data);

	.noinit (NOLOAD) : {
		*(.noinit*)
	} >intram

	.bss (NOLOAD) : {
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >intram

	/DISCARD/ : { *(.eh_frame) }

	. = ALIGN(4);
	end = .;
}

PROVIDE(_stack_sys = ORIGIN(intram) + LENGTH(intram) - 0x4000);
PROVIDE(_stack_irq = ORIGIN(intram) + LENGTH(intram));
