// Single-instruction repeat of instructions that are allowed to repeat.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
br 0x0000$0200 always

// 0: rep push stores four words that pop returns.
segment p 0200
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
clr a0 always
mov 0x$1234 r1
rep 0x0003u8
push r1
pop r1
pop r1
pop r1
mov 0x$0000 r1
pop r1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)] a0
mov a0l sp
br 0x0000$0280 always
data 1111
data 2222
data 3333
data 4444

// 1: rep pop loads the first pushed word last.
segment p 0280
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
clr a0 always
push 0x$1111
push 0x$2222
push 0x$3333
push 0x$4444
rep 0x0003u8
pop r1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)] a0
mov a0l sp
br 0x0000$0300 always
data 1111
data 2222
data 3333
data 4444

// 2: rep movp rereads the same program word.
segment p 0300
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
clr a0 always
mov 0x$031F a0l
rep 0x0003u8
data 0041 // movp (a0l),r1; PDF Table 4-4
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)] a0
mov a0l sp
br 0x0000$0380 always
data 1111
data 2222
data 3333
data 4444

// 3: rep movd copies four data words to program memory.
segment p 0380
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)]
clr a0 always
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
mov 0x$03B4 r4
mov 0x$5A01 r3
mov r3 [r0++]
mov 0x$5A02 r3
mov r3 [r0++]
mov 0x$5A03 r3
mov r3 [r0++]
mov 0x$5A04 r3
mov r3 [r0]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r0
rep 0x0003u8
data 5FA8 // movd (r0++),(r4++); PDF Table 4-4
nop
nop
mov 0x$03B7 a0l
data 0041 // movp (a0l),r1; PDF Table 4-4
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)] a0
mov a0l sp
br 0x0000$0400 always
data 1111
data 2222
data 3333
data 4444

// 4: rep mov register to icr.
segment p 0400
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0434)]
clr a0 always
mov 0x$000E r1
rep 0x0003u8
mov r1 icr
mov icr a0
mov a0l r1
mov 0x0000 icr
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0434)] a0
mov a0l sp
br 0x0000$0480 always
data 1111
data 2222
data 3333
data 4444

// 5: rep dint keeps interrupts disabled.
segment p 0480
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043C)]
clr a0 always
eint
rep 0x0003u8
dint
mov st0 r1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0438)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0439)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043A)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043C)] a0
mov a0l sp
br 0x0000$0500 always
data 1111
data 2222
data 3333
data 4444

// 6: rep break leaves the block repeat after one iteration.
segment p 0500
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0444)]
clr a0 always
bkrep 0x0003u8 0x0000$0514
inc a1 always
rep 0x0000u8
break
nop
nop
nop
inc a0 always
mov a1l r1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0440)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0441)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0442)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0443)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0444)] a0
mov a0l sp
br 0x0000$0F00 always
data 1111
data 2222
data 3333
data 4444

segment p 0F00
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0F04 always

segment p 0F04
br 0x0000$0F04 always
