// Direct PMB8876 Mask ROM monitoring vectors with synthetic zero I/Q input.
segment p 0006
br 0x0000$0100 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x$0001 st0
mov 0x$007D st1
mov 0x$0000 st2
mov 0x$7C14 sp
clr a0 always
mov a0l [0x$77DC]
mov a0l [0x$77D6]
mov a0l [0x$7727]
mov a0l [0x$7728]
mov a0l [0x$TEAK_MOD_CTRL]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0180 always

segment p 0180
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
cmp 0x0000u8 a0
br 0x0000$0180 eq

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l [0x$6000]
sub 0x0001u8 a0
mov a0l r1
mov 0x$TEAK_DEMODULATOR_RAM_BASE r0
clr a0 always
rep r1
mov a0l [r0++]

mov 0x$6000 r0
call 0x0000$20BF always

mov [0x$TEAK_MOD_STAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
mov [0x$TEAK_MOD_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
br 0x0000$0180 always
