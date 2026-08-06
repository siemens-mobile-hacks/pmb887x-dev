// Exhaustive software-generated interrupt-source runner for bank 2.
segment p 0016
br 0x0000$0600 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0501)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINT2]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0500)]

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0501)] a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)] a1
cmp a1 a0
br 0x0000$0119 eq
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0504)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0505)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0506)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0507)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0508)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINT2]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0503)] a0
mov a0l [0x$TEAK_INT_SINT2]
rep 0x000fu8
nop
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0504)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0508)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0505)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0503)] a0
mov a0l [0x$TEAK_INT_EINT2]
eint
rep 0x00ffu8
nop
dint
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0501)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
br 0x0000$0119 always

segment p 0600
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0506)]
mov a0l [0x$TEAK_INT_RINT2]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0508)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0508)]
rep 0x000fu8
nop
mov [0x$TEAK_INT_FINT2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0507)]
data 45D0 // reti always,context.
