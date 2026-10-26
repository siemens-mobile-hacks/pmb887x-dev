// MCU0 interrupts preempting single-instruction repeats.
//
// Every mainline repeat runs INC A0 65535 times while the MCU raises MCU0.
// The handler uses only A1 and R1; the MODE word selects its behavior:
//   0 reads REPC, runs a nested repeat, reads REPC again and returns
//   1 replaces the stacked return address with 0x0300 and returns there
//   2 records the interrupted A0 at [R3] and returns
//   3 replaces the stacked return address with the abandoned repeated instruction
// The abandoned repeated instruction at 0x0201 stores A0 at [R2] and continues at
// 0x0400, which dispatches on the STAGE word.
//
// Results in Shared RAM:
//   0x0400 ready marker, written just before each interrupt window
//   0x0401 handler mode
//   0x0402 handler entries
//   0x0403 mode 0: A0L seen by the handler
//   0x0404 mode 0: REPC before the nested repeat
//   0x0405 mode 0: A1 after the nested repeat
//   0x0406 mode 0: REPC after the nested repeat
//   0x0407 A0L after the first repeat
//   0x0408 A0H after the first repeat
//   0x0409 mode 1: A0L seen by the handler
//   0x040A mode 1: stacked return address
//   0x040B A1 after INC A1 at the replaced return address
//   0x040C A0L seen by the handler during the third repeat
//   0x040D A0L after the third repeat
//   0x040E A0H after the third repeat
//   0x040F A0 after branching directly to the abandoned repeated instruction
//   0x0410 A0 after RETI to the abandoned repeated instruction
//   0x0411 A0L seen by the handler during the last repeat
//   0x0412 A0L after the last repeat
//   0x0413 A0H after the last repeat
//   0x0414 stage
//   0x0415 complete marker
segment p 0006
br 0x0000$0600 always

segment p 0100
mov 0x$0001 a1l
mov a1l [0x$TEAK_MCS_CFR]
dint
mov 0x0000 icr // keep one register bank: mainline uses a0/r0/r2/r3, the handler a1/r1.
set 0x$000C st0
set 0x$0040 st2
clr a1 always
mov a1l [0x$TEAK_INT_EINTB0]
mov a1l [0x$TEAK_INT_EINT1]
mov a1l [0x$TEAK_INT_EINT2]
mov 0x$FFFF a1l
mov a1l [0x$TEAK_INT_RINTA0]
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_EINTA0]
mov 0x$FFFE r0 // REP Rn runs Rn + 1 iterations.
eint

// First repeat: the handler runs a nested repeat and returns.
clr a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a0 always
mov 0x$A551 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
rep r0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov a0h a1l // only an accumulator low half stores to a 16-bit direct address.
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0408)]

// Second repeat: the handler abandons it and returns to 0x0300.
mov 0x$0001 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a0 always
mov 0x$A552 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0200 always

segment p 0200
rep r0
inc a0 always
mov a0l [r2]
br 0x0000$0400 always

segment p 0300
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040B)]

// Third repeat after the abandoned one.
mov 0x$0002 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040C) r3
clr a0 always
mov 0x$A553 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
rep r0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040D)]
mov a0h a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040E)]

// Branch straight to the abandoned repeated instruction without its REP.
mov 0x$0004 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040F) r2
clr a0 always
br 0x0000$0201 always

segment p 0400
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)] a1
cmp 0x0004u8 a1
br 0x0000$0420 eq
cmp 0x0005u8 a1
br 0x0000$0460 eq
br 0x0000$0500 always

// Return from an interrupt to the abandoned repeated instruction.
segment p 0420
mov 0x$0005 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov 0x$0003 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410) r2
clr a0 always
mov 0x$A554 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0440 always

segment p 0440
br 0x0000$0440 always

// Last repeat after the interrupt return.
segment p 0460
mov 0x$0002 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411) r3
clr a0 always
mov 0x$A555 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
rep r0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov a0h a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
br 0x0000$0500 always

segment p 0500
dint
mov 0x$600D a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
br 0x0000$0520 always

segment p 0520
br 0x0000$0520 always

// MCU0 handler.
segment p 0600
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a1
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)] a1
cmp 0x0000u8 a1
br 0x0000$0640 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov repc a1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
clr a1 always
rep 0x0003u8
inc a1 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov repc a1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
br 0x0000$06E0 always

segment p 0640
cmp 0x0001u8 a1
br 0x0000$0680 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0409)]
mov sp r1
mov [r1] a1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x040A)]
mov 0x$0300 a1l
mov a1l [r1]
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_RINTA0]
clr a1 always
reti true

segment p 0680
cmp 0x0002u8 a1
br 0x0000$06A0 neq
mov a0l [r3]
br 0x0000$06E0 always

segment p 06A0
mov sp r1
mov 0x$0201 a1l
mov a1l [r1]
br 0x0000$06E0 always

segment p 06E0
mov 0x$0001 a1l
mov a1l [0x$TEAK_INT_RINTA0]
reti true
