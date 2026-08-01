// AFE digital ring-buffer, wrap, stop/reset, and interrupt test.
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
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Keep every analog output, input, and microphone supply disabled.
clr a0 always
mov a0l [0x$TEAK_AFE_BCON]
mov a0l [0x$TEAK_AFE_VRXCTRL1]
mov a0l [0x$TEAK_AFE_VRXCTRL2]
mov a0l [0x$TEAK_AFE_VTXCTRL]
mov a0l [0x$TEAK_AFE_RINGCTRL]

// Fill the DSP-writable receive ring with a known playback sample.
mov 0x$TEAK_ADDR(TEAK_AFE_RAM_BASE, 0x0040) r0
clr a0 always
mov 0x$A55A a0l
rep 0x003fu8
mov a0l [r0++]

// The receive/interpolation path consumes the DSP-written RX ring.
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov 0x$0004 a0l
mov a0l [0x$TEAK_AFE_INTPTR]
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]
mov 0x$0020 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_EINTB0]
eint
mov 0x$0003 a0l
mov a0l [0x$TEAK_AFE_BCON]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
cmp 0x$0002 a0
br 0x0000$0200 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)]
mov [0x$TEAK_AFE_BCON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]

// The transmit/decimation path produces samples in the DSP-readable TX ring.
clr a0 always
mov a0l [0x$TEAK_AFE_BCON]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0028)]
mov 0x$0400 a0l
mov a0l [0x$TEAK_AFE_INTPTR]
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)]
mov 0x$0040 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0021 a0l
mov a0l [0x$TEAK_AFE_BCON]
br 0x0000$0300 always

segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
cmp 0x$0002 a0
br 0x0000$0300 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002C)]
mov [0x$TEAK_AFE_BCON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002F)]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
clr a0 always
mov a0l [0x$TEAK_AFE_BCON]
mov 0x$0010 a0l
mov a0l [0x$TEAK_AFE_INTPTR]
mov 0x$0003 a0l
mov a0l [0x$TEAK_AFE_BCON]
call 0x0000$0700 always
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002B)]
clr a0 always
mov a0l [0x$TEAK_AFE_BCON]
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002E)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Snapshot the complete DSP-readable transmit ring after both paths have stopped.
mov 0x$TEAK_AFE_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r2
call 0x0000$0707 always
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 0700
mov [0x$TEAK_AFE_RWADDR] a0
and 0x$3F00 a0
br 0x0000$0700 neq
ret always
clr a1 always
mov 0x0040u8 a1l
mov [r0++] a0l
mov a0l [r2++]
dec a1 always
br 0x0000$0709 neq
ret always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
cmp 0x$0001 a0
br 0x0000$08C0 neq

// Receive/interpolation path: VBRX source, low pointer field.
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]
mov 0x$0020 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l r4
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
cmp 0x0000u8 a0
br 0x0000$0890 eq
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
clr a0 always
mov a0l [0x$TEAK_AFE_INTPTR]
mov 0x$0001 a0l
mov a0l [0x$TEAK_AFE_BCON]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
mov 0x$0020 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
data 45D0 // reti always,context.

segment p 0890
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_AFE_INTPTR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
data 45D0 // reti always,context.

// Transmit/decimation path: VBTX source, high pointer field.
segment p 08C0
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)]
mov 0x$0040 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l r4
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
cmp 0x0000u8 a0
br 0x0000$0910 eq
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_AFE_BCON]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
data 45D0 // reti always,context.

segment p 0910
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0029)]
mov 0x$0200 a0l
mov a0l [0x$TEAK_AFE_INTPTR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
data 45D0 // reti always,context.
