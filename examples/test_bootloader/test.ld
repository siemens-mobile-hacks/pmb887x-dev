ENTRY(_start)
SECTIONS {
	.xuj : {
		*(.text)
		*(.rodata)
	}
}
MEMORY {
    RAM (wx) : ORIGIN = 0x00082000, LENGTH = 8k
}