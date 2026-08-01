// Parameterized I2S receive PCM auto-stop, pointer stability, and interrupt test.
segment p 0006
br 0x0000$0800 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Power the interface before accessing its dual-port RAM.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov 0x$0001 a0l
mov a0l [r0]

// Transmit zeros while receiving.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0019)] a0
mov a0l r0
clr a0 always
rep 0x003fu8
mov a0l [r0++]

// Configure divider 0 and opposite normal-mode edges for the 4-pin clock path.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)] a0
mov a0l r0
mov 0x$1004 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0014)] a0
mov a0l r0
mov 0x$0068 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0015)] a0
mov a0l r0
mov 0x$0001 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0017)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]

// Ignore companion TX interrupts and stop both PCM directions at position 6.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x001A)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0006 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)] a0
mov a0l r0
mov 0x$0006 a0l
mov a0l [r0]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0067 a0l
mov a0l [r0]
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002C)]
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)]

dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0028)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x001A)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
data 45D0 // reti always,context.

segment p 0840
mov 0x$FFFF r0
rep r0
nop
ret always
