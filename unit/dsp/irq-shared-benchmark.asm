// DSP interrupt latency and Shared RAM throughput benchmark.
segment p 0006
mov a1l [0x$7000]
br 0x0000$0800 always

segment p 000E
mov a1l [0x$7000]
br 0x0000$0800 always

segment p 0016
mov a1l [0x$7000]
br 0x0000$0800 always

// Initialize Shared RAM, synthetic interrupt sources, and interrupt delivery.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F80 // mov #0,icr: keep the interrupted register bank visible at the vector.
clr a0 always
mov a0l [0x$7001]
mov a0l [0x$7002]
mov a0l [0x$7003]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
mov 0x$001F a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
set 0x$000C st0
set 0x$0040 st2
eint
mov 0x$B101 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0180 always

// Run once for every new nonzero command sequence.
segment p 0180
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
mov [0x$7001] a1
cmp a1 a0
br 0x0000$0180 eq
cmp 0x$0000 a0
br 0x0000$0180 eq
mov a0l [0x$7001]
br 0x0000$0200 always

// Count one-cycle increments retired after SINT and before the interrupt vector.
segment p 0200
mov 0x$FFFF a0l
mov a0l [0x$7000]
clr a0 always
mov a0l [0x$7002]
clr a1 always
mov 0x$0010 a0l
mov a0l [0x$TEAK_INT_SINTA0]
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov [0x$7000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov [0x$7002] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]

mov 0x$FFFF a0l
mov a0l [0x$7000]
clr a0 always
mov a0l [0x$7002]
clr a1 always
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINTB0]
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov [0x$7000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)]
mov [0x$7002] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)]

mov 0x$FFFF a0l
mov a0l [0x$7000]
clr a0 always
mov a0l [0x$7002]
clr a1 always
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINT1]
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov [0x$7000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0014)]
mov [0x$7002] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0015)]

mov 0x$FFFF a0l
mov a0l [0x$7000]
clr a0 always
mov a0l [0x$7002]
clr a1 always
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINT2]
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov [0x$7000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)]
mov [0x$7002] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0017)]
dint
data 4F8E // mov #0x0E,icr: preserve mainline state for sustained and external IRQs.
mov 0x$0001 a0l
mov a0l [0x$7003]
eint
br 0x0000$0300 always

// Measure 4096 complete SINTA0 -> vector -> acknowledge -> RETI round trips.
segment p 0300
clr a0 always
mov a0l [0x$7002]
mov a0l [0x$TEAK_TMR2_CTRL]
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$1000 a1l
mov 0x$0010 a0l
mov a0l [0x$TEAK_INT_SINTA0]
dec a1 always
br 0x0000$0311 neq
mov [0x$TEAK_TMR2_CNT] a1
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)]
mov [0x$7002] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0019)]
br 0x0000$0400 always

// Run identical 65536-word read/write loops against XRAM and Shared RAM.
segment p 0400
mov 0x$6000 r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020) r5
call 0x0000$0500 always
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022) r5
call 0x0000$0500 always
mov 0x$6000 r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024) r5
call 0x0000$0522 always
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026) r5
call 0x0000$0522 always
mov [0x$7001] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
br 0x0000$0180 always

// Read 64 passes over 1024 words. Return Timer2 ticks and the final value.
segment p 0500
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$0040 a1l
mov r4 a0l
mov a0l r2
mov 0x$03FF r1
rep r1
mov [r2++] a0l
dec a1 always
br 0x0000$050F neq
mov a0l r3
mov [0x$TEAK_TMR2_CNT] a1
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov a1l [r5++]
mov r3 a0l
mov a0l [r5++]
ret always

// Write 64 passes over 1024 words. Return Timer2 ticks and the final value.
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$0040 a1l
mov r4 a0l
mov a0l r2
mov 0x$03FF r1
mov 0x$5A5A a0l
rep r1
mov a0l [r2++]
dec a1 always
br 0x0000$0531 neq
mov [0x$TEAK_TMR2_CNT] a1
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov a1l [r5++]
mov r4 a0l
add 0x$03FF a0
mov a0l r2
mov [r2] a0
mov a0l [r5++]
ret always

// All three vectors share an intentionally uniform acknowledgement handler.
segment p 0800
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_INT_RINT1]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_INT_RINT2]
mov [0x$7002] a0
inc a0 always
mov a0l [0x$7002]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)]
mov [0x$7003] a0
cmp 0x$0000 a0
br 0x0000$0820 neq
data 45C0 // reti always: entry probe runs without context switching.
data 45D0 // reti always,context.
