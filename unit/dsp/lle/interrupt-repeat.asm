// MCU0 interrupt preempting a single-instruction repeat.
//
// The mainline repeats INC A0 65535 times; the MCU raises MCU0 while it runs.
// The handler uses only A1 and records the A0 it interrupted.
//
// Results in Shared RAM:
//   0x0400 ready marker, written just before the repeat
//   0x0401 A0L seen by the handler
//   0x0402 handler entries
//   0x0403 A0L after the repeat
//   0x0404 A0H after the repeat
//   0x0405 complete marker
segment p 0006
br 0x0000$0600 always

segment p 0100
mov 0x$0001 a1l
mov a1l [0x$TEAK_MCS_CFR]
dint
mov 0x0000 icr // keep one register bank: mainline uses a0/r0, the handler a1.
set 0x$000C st0
set 0x$0040 st2
clr a1 always
mov a1l [0x$TEAK_INT_EINTB0]
mov a1l [0x$TEAK_INT_EINT1]
mov a1l [0x$TEAK_INT_EINT2]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
mov 0x$FFFF a1l
mov a1l [0x$TEAK_INT_RINTA0]
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_EINTA0]
clr a0 always
mov 0x$FFFE r0 // REP Rn runs Rn + 1 iterations.
eint
mov 0x$A55A a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
rep r0
inc a0 always
dint
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0h a1l // only an accumulator low half stores to a 16-bit direct address.
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
mov 0x$600D a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
br 0x0000$0200 always

segment p 0200
br 0x0000$0200 always

// MCU0 handler.
segment p 0600
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a1
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_RINTA0]
reti true
