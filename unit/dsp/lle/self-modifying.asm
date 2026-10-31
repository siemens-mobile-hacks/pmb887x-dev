// Self-modifying program code written by movd: pipeline distance, executed code, and block repeats.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
br 0x0000$0200 always

// 0: movd rewrites the word 1 after itself.
segment p 0200
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$020A r4
clr a0 always
clr a1 always
data 5F80 // movd (r0),(r4); PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov 0x$020A a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
br 0x0000$0280 always

// 1: movd rewrites the word 2 after itself.
segment p 0280
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$028B r4
clr a0 always
clr a1 always
data 5F80 // movd (r0),(r4); PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
mov 0x$028B a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0416)]
br 0x0000$0300 always

// 2: movd rewrites the word 3 after itself.
segment p 0300
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$030C r4
clr a0 always
clr a1 always
data 5F80 // movd (r0),(r4); PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov 0x$030C a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
br 0x0000$0380 always

// 3: movd rewrites the word 4 after itself.
segment p 0380
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$038D r4
clr a0 always
clr a1 always
data 5F80 // movd (r0),(r4); PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041D)]
mov 0x$038D a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041E)]
br 0x0000$0400 always

// 4: movd rewrites an executed subroutine twice.
segment p 0400
clr a0 always
clr a1 always
call 0x0000$0420 always
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$0420 r4
data 5F80 // movd (r0),(r4); PDF Table 4-4
nop
nop
nop
call 0x0000$0420 always
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$77E0 r3
mov r3 [r0]
mov 0x$0420 r4
data 5F80 // movd (r0),(r4); PDF Table 4-4
nop
nop
nop
call 0x0000$0420 always
br 0x0000$0422 always
inc a1 always
ret always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov 0x$0420 a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
br 0x0000$0480 always

// 5: movd rewrites the active block repeat body.
segment p 0480
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$048B r4
clr a0 always
clr a1 always
bkrep 0x0003u8 0x0000$0490
inc a1 always
data 5F80 // movd (r0),(r4); PDF Table 4-4
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0425)]
mov 0x$048B a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0426)]
br 0x0000$0500 always

// 6: movd rewrites a calla target between calls.
segment p 0500
clr a1 always
mov 0x$0515 r2
mov r2 a0l
data D480 // calla a0l; PDF Table 4-4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$67D0 r3
mov r3 [r0]
mov 0x$0515 r4
data 5F80 // movd (r0),(r4); PDF Table 4-4
nop
nop
nop
clr a0 always
mov r2 a0l
data D480 // calla a0l; PDF Table 4-4
br 0x0000$0517 always
inc a1 always
ret always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov 0x$0515 a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
br 0x0000$0F00 always

segment p 0F00
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0F04 always

segment p 0F04
br 0x0000$0F04 always
