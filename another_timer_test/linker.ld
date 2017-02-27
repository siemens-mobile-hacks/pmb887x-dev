OUTPUT_FORMAT("elf32-littlearm")
ENTRY(__enter_reset)


SECTIONS
{
    . = 0xA8000000;
    .text :
    {
            *( .text.init )
            _ftext = .;
            *(.text .stub .text.* .gnu.linkonce.t.*)
            _etext = .;
    }

    .rodata :
    {
            . = ALIGN(4);
            _frodata = .;
             *(.rodata .rodata.* .gnu.linkonce.r.*)
            *(.rodata1)
            _erodata = .;
    }

    .data :
    {
            . = ALIGN(4);
            _fdata = .;
            *(.data .data.* .gnu.linkonce.d.*)
            *(.data1)
            _gp = ALIGN(16);
            *(.sdata .sdata.* .gnu.linkonce.s.*)
            _edata = .;
    }
}
