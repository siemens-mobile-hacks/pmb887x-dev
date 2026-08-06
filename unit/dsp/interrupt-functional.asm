// Interrupt-unit masking, acknowledgement, priority, and repeated delivery.
segment p 0006
br 0x0000$0900 always

segment p 000E
br 0x0000$0920 always

segment p 0016
br 0x0000$0960 always

// Initialize all interrupt banks and enable context switching for INT0/INT1/INT2.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]

// A0 source: pending while disabled, then delivered after enable.
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTA0]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
cmp 0x$0001 a0
br 0x0000$0145 neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]

// B0 source shares INT0 but has independent flag/enable/reset state.
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_SINTB0]
rep 0x00ffu8
nop
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_EINTB0]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
cmp 0x$0002 a0
br 0x0000$016D neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0416)]

// INT1 source: pending while disabled, then delivered after enable.
mov 0x$0003 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINT1]
rep 0x00ffu8
nop
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0417)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINT1]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)] a0
cmp 0x$0001 a0
br 0x0000$0195 neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]

// INT2 firmware source: pending while disabled, then delivered after enable.
mov 0x$0004 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINT2]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
cmp 0x$0001 a0
br 0x0000$01BD neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]

// Two simultaneous INT0 sources: acknowledge A0 first and leave B0 pending.
mov 0x$0005 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINTA0]
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_SINTB0]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
cmp 0x$0004 a0
br 0x0000$01E0 neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]

// Set INT2, INT1, then INT0; core must service priority order INT0, INT1, INT2.
mov 0x$0006 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
mov a0l [0x$TEAK_INT_SINT1]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0434)]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)] a0
cmp 0x$0003 a0
br 0x0000$021A neq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]

// The same INT2 source must deliver again after acknowledgement.
mov 0x$0007 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
cmp 0x$0001 a0
br 0x0000$0236 neq
dint
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0426)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_INT_SINT2]
eint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
cmp 0x$0002 a0
br 0x0000$0248 neq
dint
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0427)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042D)]
mov [0x$TEAK_INT_EINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042E)]
mov [0x$TEAK_INT_EINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042F)]
mov [0x$TEAK_INT_EINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov [0x$TEAK_INT_EINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
br 0x0000$0500 always

// INT1 enables nesting with simultaneous INT0 and INT2 pending. INT0 must win the nested arbitration.
segment p 0500
dint
mov 0x$0008 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0437)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0438)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0439)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043A)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043B)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT1]
eint
rep 0x00ffu8
nop
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043B)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043C)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043D)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043E)]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0435)]
br 0x0000$0580 always

segment p 0580
br 0x0000$0580 always

// INT0 handler; phase 5 intentionally acknowledges only one source per entry.
segment p 0800
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0005 a0
br 0x0000$0855 eq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0006 a0
br 0x0000$0816 eq
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_INT_RINTB0]
br 0x0000$0821 always

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_INT_RINTA0]

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
data 45D0 // reti always,context.

// INT1 handler.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0006 a0
br 0x0000$0834 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_INT_RINT1]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]
data 45D0 // reti always,context.

// INT2 handler.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0006 a0
br 0x0000$084B neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_INT_RINT2]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
data 45D0 // reti always,context.

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
cmp 0x$0002 a0
br 0x0000$0861 eq
mov 0x$0080 a0l
mov a0l [0x$TEAK_INT_RINTB0]
br 0x0000$0821 always

mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041D)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041E)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041F)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
br 0x0000$0821 always

// INT0 vector dispatcher and nested-priority observer.
segment p 0900
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0008 a0
br 0x0000$0800 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0438)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
data 45D0 // reti always,context.

// INT1 vector dispatcher and nesting initiator.
segment p 0920
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0008 a0
br 0x0000$0827 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0437)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_SINT2]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x000fu8
nop
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043F)]
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0440)]
eint
rep 0x000fu8
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043A)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)]
data 45D0 // reti always,context.

// INT2 vector dispatcher and nested-priority observer.
segment p 0960
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
cmp 0x$0008 a0
br 0x0000$083E neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0439)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0436)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT2]
data 45D0 // reti always,context.
