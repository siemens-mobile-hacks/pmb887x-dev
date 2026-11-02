// AFE transmit pacing: every VBTX handler re-arms TXINTPTR one ring position after the previous pointer.
segment p 0006
br 0x0000$0800 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x000e icr // context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Keep every analog output, input, and microphone supply disabled.
clr a0 always
mov a0l [0x$TEAK_AFE_BCON]
mov a0l [0x$TEAK_AFE_VRXCTRL1]
mov a0l [0x$TEAK_AFE_VRXCTRL2]
mov a0l [0x$TEAK_AFE_VTXCTRL]
mov a0l [0x$TEAK_AFE_RINGCTRL]

// 0x10 VBTX interrupts, 0x11 VBTX handlers that found the write pointer past the armed one,
// 0x12 VBRX interrupts, 0x13 armed TX pointer, 0x20 requested BCON, 0x21 applied BCON.
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0060 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_EINTB0]
eint
mov 0x$1111 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0200 always

// Poll for the first BCON request, then idle between ring interrupts until the next request.
segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)] a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)] a1
cmp a1 a0
br 0x0000$0240 neq
cmp 0x0000u8 a1
br 0x0000$0200 eq
clr a0 always
mov a0l [0x$TEAK_DSP_CTRL]
nop
nop
nop
nop
br 0x0000$0200 always

// Stopping both paths resets the ring pointers; restart with TXINTPTR one ring position ahead.
segment p 0240
dint
clr a1 always
mov a1l [0x$TEAK_AFE_BCON]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)]
mov 0x$0100 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)]
mov 0x$0120 a1l
mov a1l [0x$TEAK_AFE_INTPTR]
mov 0x$0060 a1l
mov a1l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov a0 a1
and 0x$7FFF a0
mov a0l [0x$TEAK_AFE_BCON]
and 0x$8000 a1
br 0x0000$0280 neq
eint
br 0x0000$0200 always

// Bit 15 of the request counts RDADDR and WRADDR steps with interrupts disabled: shared 0x30 holds the RDADDR
// steps, 0x31 the largest RDADDR step between consecutive polls and 0x33 the WRADDR steps.
segment p 0280
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)]
mov [0x$TEAK_AFE_RWADDR] a0
mov a0 a1
and 0x$003F a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)]
mov a1 a0
and 0x$3F00 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)]
br 0x0000$02A0 always

segment p 02A0
mov [0x$TEAK_AFE_RWADDR] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0035)]
and 0x$003F a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)] a1
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)]
sub a1 a0
clr a1 always
mov 0x$0040 a1l
add a1 a0
and 0x$003F a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a1
add a0 a1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)] a1
cmp a1 a0
br 0x0000$02E0 le
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
br 0x0000$02E0 always

// WRADDR moves at most one position between polls, so every change is one step.
segment p 02E0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0035)] a0
and 0x$3F00 a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)] a1
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)]
cmp a1 a0
br 0x0000$0300 eq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)]
br 0x0000$0300 always

segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)] a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)] a1
cmp a1 a0
br 0x0000$02A0 eq
br 0x0000$0240 always

segment p 0800
mov [0x$TEAK_INT_FINTB0] a0
and 0x$0040 a0
br 0x0000$0880 eq
mov 0x$0040 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_AFE_RWADDR] a0
and 0x$3F00 a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)] a1
cmp a1 a0
br 0x0000$0840 eq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)]
br 0x0000$0840 always

segment p 0840
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)] a0
clr a1 always
mov 0x$0100 a1l
add a1 a0
and 0x$3F00 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)]
or 0x$0020 a0
mov a0l [0x$TEAK_AFE_INTPTR]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)]
retic true

segment p 0880
mov 0x$0020 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)]
retic true
