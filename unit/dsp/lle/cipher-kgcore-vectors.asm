// DSP-side server for the physical A5/3 16-to-32-bit wrapper.
segment p 000E
br 0x0000$0900 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
set 0x$000C st0
set 0x$0040 st2
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0900)]
br 0x0000$0A00 always

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

mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0B00) r0
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY0]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY1]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY2]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY3]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY4]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY5]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY6]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KEY7]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KDATA1]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KDATA2]
mov [r0++] a0l
mov a0l [0x$TEAK_CIPH_KDATA3]
mov [r0] a0l
mov a0l [0x$TEAK_CIPH_KDATA4]

mov 0x$0019 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
call 0x0000$0B80 always
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0B20) r1
call 0x0000$0BA0 always
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0904)]
br 0x0000$0A00 always

// Wait until CACT clears.
segment p 0B80
mov [0x$TEAK_CIPH_CSTAT] a0
and 0x$0001 a0
br 0x0000$0B80 neq
ret always

// Copy both 114-bit streams, omitting the unused Cipher RAM gap. The A5/3
// wrapper reverses each 114-bit stream as a whole relative to 3GPP bit order.
segment p 0BA0
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
