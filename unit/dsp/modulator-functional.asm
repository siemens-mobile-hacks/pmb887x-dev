// Digital GMSK modulator RAM-consumption, wrap, status, and interrupt test.
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
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
clr a0 always
mov a0l [0x$TEAK_MOD_CTRL]

// Every RAM entry contains a valid data bit. TXON remains inactive, so only the
// digital modulator runs and the analog transmitter stays disabled.
mov 0x$TEAK_MODULATOR_RAM_BASE r0
mov 0x$0001 a0l
mov 0x$01FF r1
rep r1
mov a0l [r0++]
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov 0x$0004 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_EINTA0]
eint
mov 0x$0100 a0l
mov a0l [0x$TEAK_MOD_CTRL]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_MOD_STAT] a0
and 0x$0001 a0
br 0x0000$0200 eq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
br 0x0000$0300 always

segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
cmp 0x$0002 a0
br 0x0000$0300 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov [0x$TEAK_MOD_STAT] a0
and 0x$0001 a0
br 0x0000$0300 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)]
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov [0x$TEAK_MOD_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)]
mov [0x$TEAK_MODULATOR_RAM_BASE] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0028)]
mov [0x$TEAK_ADDR(TEAK_MODULATOR_RAM_BASE, 0x01FF)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0029)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
cmp 0x0000u8 a0
br 0x0000$0840 eq
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
clr a0 always
mov a0l [0x$TEAK_MOD_CTRL]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
data 45D0 // reti always,context.

segment p 0840
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
data 45D0 // reti always,context.
