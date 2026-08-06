// TPU Timer RAM decoder observations through DSP-visible status and interrupt flags.
segment p 0006
br 0x0000$0100 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x000F)]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
br 0x0000$0200 eq
mov a0l r4
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov [0x$TEAK_BB_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
br 0x0000$0200 always
