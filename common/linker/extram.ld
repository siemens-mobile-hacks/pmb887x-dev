ENTRY(__enter_reset)

MEMORY
{
	RAM (rwx) : ORIGIN = 0xA8000000, LENGTH = 8M
}

SECTIONS {
	.rom : {
		*(.text*)
		*(.rodata*)
		*(.sdata*)
		*(.data*)
		_sbss = .;
		*(.bss*)
		_ebss = .;
	} > RAM
}
