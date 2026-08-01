// PMB8876 DSP pad output, input-latch, and input-edge interrupt runner.
segment p 000E
br 0x0000$0800 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
mov 0x$0030 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_EINT1]
eint
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
cmp 0x0001u8 a0
br 0x0000$0240 eq
cmp 0x0002u8 a0
br 0x0000$0260 eq
cmp 0x$FFFF a0
br 0x0000$0280 eq
br 0x0000$0200 always

segment p 0240
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
mov a0l [0x$TEAK_DSP_DSPOUT]
mov [0x$TEAK_DSP_DSPOUT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
br 0x0000$0200 always

segment p 0260
mov [0x$TEAK_DSP_DSPOUT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
br 0x0000$0200 always

segment p 0280
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$0030 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov [0x$TEAK_INT_FINT1] a0
and 0x$0030 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
mov a0l [0x$TEAK_INT_RINT1]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
data 45D0 // reti always,context.
