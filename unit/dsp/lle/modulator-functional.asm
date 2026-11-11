// Digital GMSK modulator control, start sources, RAM consumption, wrap, status, and interrupt test.
segment p 0006
br 0x0000$0800 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x000e icr // context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]

// MOD_CTRL after reset, then every bit except MSWACT written as one.
mov [0x$TEAK_MOD_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov 0x$FEFF a0l
mov a0l [0x$TEAK_MOD_CTRL]
mov [0x$TEAK_MOD_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
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

// MSWACT without CODON: status, MODU at address 0x030 within a bounded poll, and stop.
mov 0x$0030 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$0100 a0l
mov a0l [0x$TEAK_MOD_CTRL]
clr a1 always
br 0x0000$0160 always

segment p 0160
inc a1 always
cmp 0x$00FF a1
br 0x0000$0170 eq
mov [0x$TEAK_MOD_STAT] a0
and 0x$0001 a0
br 0x0000$0160 eq
br 0x0000$0170 always

segment p 0170
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)]
clr a1 always
br 0x0000$0180 always

segment p 0180
inc a1 always
cmp 0x$7FFF a1
br 0x0000$01C0 eq
mov [0x$TEAK_INT_FINTA0] a0
and 0x$0080 a0
br 0x0000$0180 eq
br 0x0000$01C0 always

segment p 01C0
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)]
clr a0 always
mov a0l [0x$TEAK_MOD_CTRL]
clr a1 always
br 0x0000$01E0 always

segment p 01E0
inc a1 always
cmp 0x$00FF a1
br 0x0000$0200 eq
mov [0x$TEAK_MOD_STAT] a0
and 0x$0001 a0
br 0x0000$01E0 neq
br 0x0000$0200 always

// CODON from the TPU starts and stops the modulator.
segment p 0200
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$0004 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_EINTA0]
eint
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
br 0x0000$0280 always

segment p 0280
mov [0x$TEAK_MOD_STAT] a0
and 0x$0001 a0
br 0x0000$0280 eq
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
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
retic true

segment p 0840
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
retic true
