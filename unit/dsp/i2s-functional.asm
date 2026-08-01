// Parameterized I2S transmit-ring, stop/restart, wrap, and interrupt test.
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

// Fill the complete 64-word transmit ring with a deterministic nonzero sample.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0017)] a0
mov a0l r0
mov 0x$A55A a0l
rep 0x003fu8
mov a0l [r0++]

// Configure I2S and use I2SON to reset the internal read pointer.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov 0x$0001 a0l
mov a0l [r0]
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
clr a0 always
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]

// Enable the selected real I2S source in interrupt bank B0.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_EINTB0]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0019)]
eint

// First transfer stops in the handler at interrupt position 4.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0004 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0003 a0l
mov a0l [r0]
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]

// Restart from the preserved position and stop at position 8.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0008 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0003 a0l
mov a0l [r0]
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]

// Continue through the end of the ring and stop just after the pointer wraps.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0002 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0003 a0l
mov a0l [r0]
call 0x0000$0840 always
call 0x0000$0840 always
call 0x0000$0840 always
call 0x0000$0840 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)]

// PCM mode stops automatically when the programmed interrupt position is reached.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0029)]
clr a0 always
mov a0l [r0]
mov 0x$0001 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0019)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002E)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002B)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002C)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0006 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0023 a0l
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

// Natural I2S TX handler: capture, acknowledge, and stop after the current frame.
segment p 0800
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0019)] a0
cmp 0x0000u8 a0
br 0x0000$0820 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0001 a0l
mov a0l [r0]
br 0x0000$0820 always

segment p 0820
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
data 45D0 // reti always,context.

// Delay long enough for the programmed I2S transfer and handler entry.
segment p 0840
mov 0x$FFFF r0
rep r0
nop
ret always
