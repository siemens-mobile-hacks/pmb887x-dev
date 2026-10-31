// Program-counter writes: executed following words, target, and stack pointer.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
br 0x0000$0200 always

// 0: ret.
segment p 0200
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
clr a1 always
mov 0x$0700 r1
push r1
ret always
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
br 0x0000$0240 always

segment p 0700
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)] a0
mov a0l sp
br 0x0000$0240 always

segment p 0A00
data 0700 // movp target

// 1: mov r0 pc.
segment p 0240
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0417)]
clr a1 always
mov 0x$0720 r0
data 5980 // mov r0, pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0416)]
br 0x0000$0280 always

segment p 0720
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0417)] a0
mov a0l sp
br 0x0000$0280 always

segment p 0A01
data 0720 // movp target

// 2: mov a0l pc.
segment p 0280
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
clr a1 always
clr a0 always
mov 0x$0740 a0l
data 599A // mov a0l, pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
br 0x0000$02C0 always

segment p 0740
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)] a0
mov a0l sp
br 0x0000$02C0 always

segment p 0A02
data 0740 // movp target

// 3: mov long immediate pc.
segment p 02C0
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041F)]
clr a1 always
data 5E0C // mov ##long immediate, pc; PDF Table 4-4
data 0760 // target
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041E)]
br 0x0000$0300 always

segment p 0760
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041D)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041F)] a0
mov a0l sp
br 0x0000$0300 always

segment p 0A03
data 0760 // movp target

// 4: pop pc.
segment p 0300
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
clr a1 always
mov 0x$0780 r1
push r1
data 5E6C // pop pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
br 0x0000$0340 always

segment p 0780
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)] a0
mov a0l sp
br 0x0000$0340 always

segment p 0A04
data 0780 // movp target

// 5: mov mixp pc.
segment p 0340
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0427)]
clr a1 always
mov 0x$07A0 r0
mov r0 mixp
data 47CC // mov mixp, pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0426)]
br 0x0000$0380 always

segment p 07A0
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0425)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0427)] a0
mov a0l sp
br 0x0000$0380 always

segment p 0A05
data 07A0 // movp target

// 6: mov indirect r0 pc.
segment p 0380
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
clr a1 always
mov 0x$07C0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402) r0
data 1D80 // mov (r0), pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
br 0x0000$03C0 always

segment p 07C0
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)] a0
mov a0l sp
br 0x0000$03C0 always

segment p 0A06
data 07C0 // movp target

// 7: movp indirect a0l pc.
segment p 03C0
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042F)]
clr a1 always
clr a0 always
mov 0x$0A07 a0l
data 004C // movp (a0l), pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042E)]
br 0x0000$0400 always

segment p 07E0
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042D)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042F)] a0
mov a0l sp
br 0x0000$0400 always

segment p 0A07
data 07E0 // movp target

// 8: mov stack top pc.
segment p 0400
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
clr a1 always
mov 0x$0800 r1
push r1
data 47EC // mov (sp), pc; PDF Table 4-4
inc a1 always
inc a1 always
inc a1 always
inc a1 always
mov 0x$FA11 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
br 0x0000$0A80 always

segment p 0800
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)] a0
mov a0l sp
br 0x0000$0A80 always

segment p 0A08
data 0800 // movp target

segment p 0A80
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0A84 always

segment p 0A84
br 0x0000$0A84 always
