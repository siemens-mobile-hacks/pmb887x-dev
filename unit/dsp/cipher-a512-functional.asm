// DSP-side A5/1 and A5/2 known-answer, repeatability, sensitivity, EDGE, and interrupt scenarios.
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
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0703)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
eint
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0700)]

// A5/1 GSM known-answer vector: Kc 0123456789ABCDEF, frame 123456.
clr a0 always
mov a0l [0x$TEAK_CIPH_CSTAT]
mov 0x$CDEF a0l
mov a0l [0x$TEAK_CIPH_KEY0]
mov 0x$89AB a0l
mov a0l [0x$TEAK_CIPH_KEY1]
mov 0x$4567 a0l
mov a0l [0x$TEAK_CIPH_KEY2]
mov 0x$0123 a0l
mov a0l [0x$TEAK_CIPH_KEY3]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CIPH_TMOD26]
mov 0x$0024 a0l
mov a0l [0x$TEAK_CIPH_TMOD51]
mov 0x$005D a0l
mov a0l [0x$TEAK_CIPH_SFNUM]
mov 0x$0001 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0710)]
call 0x0000$0800 always
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0712)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0713)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINT1]
nop
nop
nop
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0716)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0717)]
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0711)]
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0740) r1
call 0x0000$0820 always

// Repeat the identical vector without reset.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
call 0x0000$0800 always
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0718)]
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0760) r1
call 0x0000$0820 always

// A5/2 GSM with the same published input.
mov 0x$0002 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
mov 0x$0003 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
call 0x0000$0800 always
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0780) r1
call 0x0000$0820 always

// Change one key bit and one frame-number bit from the A5/1 vector.
clr a0 always
mov a0l [0x$TEAK_CIPH_CSTAT]
mov 0x$CDEE a0l
mov a0l [0x$TEAK_CIPH_KEY0]
mov 0x$0009 a0l
mov a0l [0x$TEAK_CIPH_TMOD26]
mov 0x$0001 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
call 0x0000$0800 always
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x07A0) r1
call 0x0000$0820 always

// A5/1 EDGE produces two complete 348-bit streams.
mov 0x$CDEF a0l
mov a0l [0x$TEAK_CIPH_KEY0]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CIPH_TMOD26]
mov 0x$0004 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
mov 0x$0005 a0l
mov a0l [0x$TEAK_CIPH_CSTAT]
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0714)]
call 0x0000$0800 always
mov [0x$TEAK_CIPH_CSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0715)]
mov 0x$TEAK_CIPHER_RAM_BASE r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0800) r1
call 0x0000$0860 always

mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0701)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

// Wait until CACT clears.
segment p 0800
mov [0x$TEAK_CIPH_CSTAT] a0
and 0x$0001 a0
br 0x0000$0800 neq
ret always

// Copy both 114-bit GSM streams, omitting the unused gap in cipher RAM.
segment p 0820
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

// Copy both 348-bit EDGE streams, omitting the unused gap in cipher RAM.
segment p 0860
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
addv 0x$000A r0
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

// INT1 handler records cipher completion and acknowledges only CIPH.
segment p 0900
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0703)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
data 45D0 // reti always,context.
