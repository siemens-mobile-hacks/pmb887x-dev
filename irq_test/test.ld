ENTRY(_start)
SECTIONS {
	.xuj : {
		*(.xuj0)
		*(.text)
		*(.rodata)
		*(.xuj2)
	}
}
MEMORY {
    RAM (wx) : ORIGIN = 0x00082000, LENGTH = 8k
}