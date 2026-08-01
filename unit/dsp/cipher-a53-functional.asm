// DSP-side A5/3 server using the CIPH_KEY path from the MCU firmware and MASK ROM.
segment p 000E
br 0x0000$0900 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: enable context switching for INT0/INT1/INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0902)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0903)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0904)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_EINT1]
eint
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0900)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0901)]
br 0x0000$0A00 always

// INT1 handler records cipher completion and acknowledges only CIPH.
segment p 0900
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0903)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0902)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0902)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
data 45D0 // reti always,context.

// Execute requests supplied through shared RAM by the MCU side.
segment p 0A00
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0904)] a0
cmp 0x0000u8 a0
br 0x0000$0A00 eq
clr a0 always
mov a0l [0x$TEAK_CIPH_CSTAT]
mov 0x$0010 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
nop
nop
nop
nop
mov 0x$0011 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
nop
nop
nop
nop

// Transform the eight CIPH_KEY words exactly like MASK ROM P:32E6..P:3305.
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A00) r0
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY1]
mov [r0++] a0l
shfi a0 a0 +0x0008
or a0h a0
mov a0l [0x$TEAK_CIPH_KEY0]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY3]
mov [r0++] a0l
shfi a0 a0 +0x0008
or a0h a0
mov a0l [0x$TEAK_CIPH_KEY2]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY5]
mov [r0++] a0l
shfi a0 a0 +0x0008
or a0h a0
mov a0l [0x$TEAK_CIPH_KEY4]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY7]
mov [r0] a0l
shfi a0 a0 +0x0008
or a0h a0
mov a0l [0x$TEAK_CIPH_KEY6]

// Build the physical KDATA words from T2, T3, and T1 like MASK ROM P:3306..P:3311.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A08)] a0
mov a0l [page:0x0074u8]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A09)] a0
mov a0l [page:0x0073u8]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A0A)] a0
mov a0l [page:0x0076u8]
mov 0x$0F00 a0
mov a0l [0x$TEAK_CIPH_KDATA1]
clr a0 always
mov a0l [0x$TEAK_CIPH_KDATA2]
mov [page:0x0074u8] a0h
shfi a0 a0 -0x0005
addh [page:0x0073u8] a0
shfi a0 a0 -0x0006
addh [page:0x0076u8] a0
shfi a0 a0 -0x0005
mov a0l [0x$TEAK_CIPH_KDATA4]
mov a0h a0l
shfi a0 a0 +0x0008
mov a0l [0x$TEAK_CIPH_KDATA3]

mov 0x$0019 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0910)]
call 0x0000$0B00 always
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0911)]
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A20) r1
call 0x0000$0B20 always
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0904)]
br 0x0000$0A00 always

// Wait until CACT clears.
segment p 0B00
mov [0x$TEAK_CIPH_CSTAT] a0
and 0x$0001 a0
br 0x0000$0B00 neq
ret always

// Copy both 114-bit streams, omitting the unused Cipher RAM gap. The A5/3
// wrapper reverses each 114-bit stream as a whole relative to 3GPP bit order.
segment p 0B20
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
addv 0x$0018 r0
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
mov [r0++] a0l
mov a0l [r1++]
ret always
