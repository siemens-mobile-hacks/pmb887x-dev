ENTRY(__enter_reset)

MEMORY
{
	RAM (rwx) : ORIGIN = 0x00082000, LENGTH = 8k
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
