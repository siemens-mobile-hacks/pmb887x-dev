// Digital modulator RAM read rate. Shared words: 0x0020 counts poll iterations
// from MSWACT to MODU at address 0x030, 0x0021 from there to MODU at 0x080;
// 0x0010 counts MODU flags at the MCU-supplied addresses 0x0040.. for MCU
// timestamps.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_MOD_CTRL]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]

// TXON remains inactive, so only the digital modulator runs.
mov 0x$TEAK_MODULATOR_RAM_BASE r0
mov 0x$0001 a0l
mov 0x$01FF r1
rep r1
mov a0l [r0++]

mov 0x$0030 a0l
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$0100 a0l
mov a0l [0x$TEAK_MOD_CTRL]
clr a1 always
br 0x0000$0180 always

segment p 0180
inc a1 always
mov [0x$TEAK_INT_FINTA0] a0
and 0x$0080 a0
br 0x0000$0180 eq
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_MOD_INT_ADDR]
clr a1 always
br 0x0000$01C0 always

segment p 01C0
inc a1 always
mov [0x$TEAK_INT_FINTA0] a0
and 0x$0080 a0
br 0x0000$01C0 eq
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0040) r2
mov [r2++] a0
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_INT_FINTA0] a0
and 0x$0080 a0
br 0x0000$0200 eq
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [r2++] a0
mov a0l [0x$TEAK_MOD_INT_ADDR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
cmp 0x$0003 a0
br 0x0000$0200 neq

clr a0 always
mov a0l [0x$TEAK_MOD_CTRL]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0280 always

segment p 0280
br 0x0000$0280 always
