// I2S receive interrupt position while the core runs and while it idles.
//
// Each RX interrupt reads RWADDR first, then moves RXINTADDR eight positions
// ahead, like the EL71 audio player does.
//
// Parameters in Shared RAM: see i2s-functional.c load_receive_parameters().
// Results in Shared RAM:
//   0x0026 interrupt count
//   0x0040.. RWADDR and the RXINTADDR it interrupted at, one pair per interrupt
//   0x0000 complete marker
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
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Power the interface before configuring it.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov 0x$0001 a0l
mov a0l [r0]

// Master mode on divider 0: 104 MHz * 32 / (104 * 32 clocks * 4) = 250 kHz frames.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)] a0
mov a0l r0
mov 0x$1020 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0014)] a0
mov a0l r0
mov 0x$0068 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0015)] a0
mov a0l r0
mov 0x$0010 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0017)] a0
mov a0l r0
mov 0x$0010 a0l
mov a0l [r0]

// First RX interrupt at position 8; results start at 0x0040.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov 0x$0008 a0l
mov a0l [r0]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003E)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0040) a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003F)]

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x001A)] a0
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0007 a0l
mov a0l [r0]
eint
br 0x0000$0300 always

// Sixteen interrupts while the core polls.
segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
cmp 0x$0010 a0
br 0x0000$0300 neq
br 0x0000$0320 always

// Sixteen more while the core idles in DSP_CTRL between interrupts.
segment p 0320
clr a0 always
mov a0l [0x$TEAK_DSP_CTRL]
nop
nop
nop
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
cmp 0x$0020 a0
br 0x0000$0320 neq

dint
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0001 a0l
mov a0l [r0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r0
mov [r0] a1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003F)] a0
mov a0l r1
mov a1l [r1++]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003E)] a0
mov a0l [r1++]
mov r1 a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003F)]

add 0x$0008 a0
and 0x$003F a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x003E)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a1
mov a1l r0
mov a0l [r0]

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x001A)] a0
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
retic true
