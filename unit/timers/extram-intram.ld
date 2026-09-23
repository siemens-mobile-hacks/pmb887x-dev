ENTRY(start)

MEMORY {
	extram (rwx) : ORIGIN = 0xB0000000, LENGTH = 32M
	intram (rwx) : ORIGIN = 0x00082000, LENGTH = 96K - 0x2000
}

SECTIONS {
	.timers_loader : {
		KEEP(*(.boot.header))
		KEEP(*(.timers_loader))
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

/* The suite has no separately linked SRAM or TCM sections. */
_sram_start = 0x00084000;
_sram_end = _sram_start;
_sram_loadaddr = _sram_start;
_tcm_start = 0x01000000;
_tcm_end = _tcm_start;
_tcm_loadaddr = _tcm_start;
boot_entry = timers_loader;

PROVIDE(_stack_irq = ORIGIN(intram) + LENGTH(intram));
PROVIDE(_stack_fiq = _stack_irq - 0x1000);
PROVIDE(_stack_abt = _stack_fiq - 0x1000);
PROVIDE(_stack_und = _stack_abt - 0x400);
PROVIDE(_stack_svc = _stack_und - 0x400);
PROVIDE(_stack_sys = _stack_svc - 0x400);

ASSERT(_ebss <= _stack_sys - 0x1000, "image leaves less than 4 KiB for SYS stack")
