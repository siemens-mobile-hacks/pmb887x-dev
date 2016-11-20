ENTRY(_start)
SECTIONS {
	.xuj : {
		KEEP(*(.xuj0))
		*(.text)
		*(.text*)
		*(.rodata)
		KEEP(*(.xuj2))
	}
}
MEMORY {
    RAM (wx) : ORIGIN = 0x00082000, LENGTH = 8k
}
