// Block repeat inside an interrupt that preempts a mainline block repeat.
//
// PMB8876 Mask ROM 0801 runs BKREP R7 at P:6692 from interrupt context while
// the interrupted code may be inside its own block repeat. The handler loop
// lies outside the interrupted loop body and ends after it.
//
// Results in Shared RAM:
//   0x0400 ready marker
//   0x0401 mainline loop iterations
//   0x0402 handler loop iterations
//   0x0403 handler entries
//   0x0404 complete marker
//   0x0405 subroutine loop iterations
//   0x0406 outer call-loop iterations
//   0x0407 subroutine complete marker
segment p 0006
br 0x0000$0600 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x0000 icr // keep one register bank: mainline uses a0/r5, the handler a1/r7.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
mov 0x$000F r5 // 16 mainline iterations.
eint
br 0x0000$0200 always

// Each mainline iteration raises INT0 and leaves the handler room to preempt the loop body.
segment p 0200
bkrep r5 0x0000$020D
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINTA0]
nop
nop
nop
dint
mov 0x$600D a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
br 0x0000$0300 always

// A called subroutine may use another block-repeat level while its caller's block repeat is active.
segment p 0300
mov 0x$0003 r5 // 4 outer iterations.
bkrep r5 0x0000$030C
call 0x0000$0700 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
nop
nop
mov 0x$CA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
br 0x0000$0311 always

// INT0 handler: a second block-repeat level whose end lies after the interrupted loop.
segment p 0600
mov 0x$0004 r7 // 5 handler iterations.
bkrep r7 0x0000$0609
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a1
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a1
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_RINTA0]
reti true

segment p 0700
mov 0x$0002 r7 // 3 subroutine iterations.
bkrep r7 0x0000$0708
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a1
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
ret true
