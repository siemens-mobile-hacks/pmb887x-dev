// Execute the same Mask ROM PC through every PMB8876 Program ROM page.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0200) r3

// Revisit pages after executing both other mappings to expose stale fetch state.
mov 0x$0000 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0004 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0008 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0000 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0008 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0004 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0000 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0004 r0
call 0x0000$0200 always
call 0x0000$0280 always
mov 0x$0008 r0
call 0x0000$0200 always
call 0x0000$0280 always

mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01FF)]
br 0x0000$0180 always

segment p 0180
br 0x0000$0180 always

// P:D99C is a short RAM-only leaf in all three banks:
// bank 0 returns, bank 1 stores A0L through R7, and bank 2 copies two words.
segment p 0200
mov 0x$1111 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100)]
mov 0x$2222 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0101)]
mov 0x$EEEE a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0138)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0140)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0141)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r1
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0101) r2
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0140) r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0141) r5
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r7
mov r0 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
call 0x0000$D99C always
ret always

// Publish the three bank effects as one record.
segment p 0280
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0138)] a0
mov a0l [r3++]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0140)] a0
mov a0l [r3++]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0141)] a0
mov a0l [r3++]
ret always
