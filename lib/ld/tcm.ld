ENTRY(start)

MEMORY {
	sram (rwx) : ORIGIN = 0x00080000, LENGTH = 96K
	tcm (rwx) : ORIGIN = 0x01000000, LENGTH = 8K
}

SECTIONS {
	/* BootROM enters the relocation stub at 0x00082000. */
	.bootstrap ORIGIN(sram) + 0x2000 : {
		KEEP(*(.boot.header))
		KEEP(*(.boot.relocate))
	} >sram

	.image ORIGIN(tcm) : {
		_image_start = .;
		*(.startup)
		*(.text*)
		. = ALIGN(4);
		*(.rodata*)
		. = ALIGN(4);

		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array))
		__init_array_end = .;
		__fini_array_start = .;
		KEEP (*(.fini_array))
		KEEP (*(SORT(.fini_array.*)))
		__fini_array_end = .;

		*(.ARM.extab*)
		__exidx_start = .;
		*(.ARM.exidx*)
		__exidx_end = .;
		. = ALIGN(4);
		_etext = .;

		_data = .;
		*(.data*)
		*(.ramtext*)
		. = ALIGN(4);
		_edata = .;
		_data_loadaddr = _data;
		_image_end = .;
	} >tcm AT>sram
	_image_load_start = LOADADDR(.image);

	.signature : {
		KEEP(*(.signature))
	} >sram

	.bss (NOLOAD) : {
		*(.bss*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
	} >tcm

	/* TCM, but not cleared on reset, eg boot/app comms */
	.noinit (NOLOAD) : {
		*(.noinit*)
	} >tcm

	. = ALIGN(4);
	end = .;

	/DISCARD/ : { *(.eh_frame) }
}

PROVIDE(_stack_irq = ORIGIN(tcm) + LENGTH(tcm));
PROVIDE(_stack_fiq = _stack_irq - 1K);
PROVIDE(_stack_abt = _stack_fiq - 1K);
PROVIDE(_stack_und = _stack_abt - 0x100);
PROVIDE(_stack_svc = _stack_und - 0x100);
PROVIDE(_stack_sys = _stack_svc - 0x100);
PROVIDE(_stack_limit = _stack_sys - 1K);
PROVIDE(start = tcm_relocate);
PROVIDE(boot_entry = tcm_relocate);

ASSERT(start == ORIGIN(sram) + 0x2000, "TCM bootstrap must start at 0x00082000")
ASSERT(end <= _stack_limit, "TCM image overlaps stacks")
