// PMB8875 Program ROM bank execution probes for Mask ROM 0602 and 0604.

// Mask ROM 0602 runner. P:E9F4 returns immediately on page 0 and executes
// cmp [r0++],a0 before returning on page 1.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0200) r3

mov 0x$0000 r5
call 0x0000$0180 always
mov 0x$0002 r5
call 0x0000$0180 always
mov 0x$0000 r5
call 0x0000$0180 always
mov 0x$0002 r5
call 0x0000$0180 always
mov 0x$0002 r5
call 0x0000$0180 always
mov 0x$0000 r5
call 0x0000$0180 always

mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01FF)]
br 0x0000$0300 always

segment p 0180
mov r5 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r0
clr a0 always
call 0x0000$E9F4 always
mov r0 a0l
mov a0l [r3++]
ret always

// Mask ROM 0604 runner. P:C037 returns immediately on page 0 and clears A0
// before returning on page 1.
segment p 0200
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0200) r3

mov 0x$0000 r5
call 0x0000$0280 always
mov 0x$0002 r5
call 0x0000$0280 always
mov 0x$0000 r5
call 0x0000$0280 always
mov 0x$0002 r5
call 0x0000$0280 always
mov 0x$0002 r5
call 0x0000$0280 always
mov 0x$0000 r5
call 0x0000$0280 always

mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01FF)]
br 0x0000$0300 always

segment p 0280
mov r5 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov 0x$EEEE a0l
call 0x0000$C037 always
mov a0l [r3++]
ret always

segment p 0300
br 0x0000$0300 always
