// Parameterized I2S clock-select rate measurement.
//
// The MCU writes a configuration and sets the command word to 1. The runner
// powers the interface down, programs it, powers it up, starts it, and then
// accumulates how far both ring pointers move over a fixed number of samples.
// It writes 2 to the command word when the measurement is complete.
//
// Command 3 writes one register, waits 256 cycles, and records RWADDR, FINTB0,
// and the register at 0x0048.
//
// Shared RAM:
//   0x0000 ready marker
//   0x0001 command word
//   0x0010..0x0018 CTRL, CSEL, RWADDR, NUM0, DEN0, NUM1, DEN1, TXCONF, RXCONF addresses
//   0x0020 CTRL start value
//   0x0021, 0x0023..0x0028 CSEL, NUM0, DEN0, NUM1, DEN1, TXCONF, RXCONF values
//   0x0029 sample count, below 0x8000
//   0x002A extra 256-cycle delays per sample
//   0x0030 transmit positions moved
//   0x0031 receive positions moved
//   0x0032 RWADDR after I2SON
//   0x0033 RWADDR at the end of the measurement
//   0x0034 remaining samples
//   0x0040 command 3 register address
//   0x0041 command 3 register value
//   0x0042 RWADDR after command 3
//   0x0043 FINTB0 after command 3
//   0x0048 register command 3 reads back
//   0x0049 value command 3 read back
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
cmp 0x$0003 a0
br 0x0000$0400 eq
cmp 0x$0001 a0
br 0x0000$0200 neq

// Program the interface while it is powered down.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
clr a0 always
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0011)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0013)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0014)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0015)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0016)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0017)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0027)] a0
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0018)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0028)] a0
mov a0l [r0]

// Power up, record the reset pointers, and start the selected directions.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010)] a0
mov a0l r0
mov 0x$0001 a0l
mov a0l [r0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r1
mov [r1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)]
and 0x$003F a0
mov a0l r4
mov [r1] a0
shfi a0 a0 -0x0008
and 0x$003F a0
mov a0l r3
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0029)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)] a0
mov a0l [r0]
br 0x0000$0300 always

// Each sample waits without I/O, then adds the pointer movement modulo the 64-word ring.
segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)] a0
mov a0l r7
bkrep r7 0x0000$0307
rep 0x00ffu8
nop
nop
mov [r1] a0
mov a0l r5
and 0x$003F a0
mov a0l r2
sub r4 a0
and 0x$003F a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a1
add a1 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov r2 r4
mov r5 a0l
shfi a0 a0 -0x0008
and 0x$003F a0
mov a0l r2
sub r3 a0
and 0x$003F a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)] a1
add a1 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
mov r2 r3
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)] a0
dec a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0034)]
br 0x0000$0300 neq

mov [r1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)]
clr a0 always
mov a0l [r0]
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
br 0x0000$0200 always

segment p 0400
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0040)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0041)] a0
mov a0l [r0]
rep 0x00ffu8
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0012)] a0
mov a0l r1
mov [r1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0042)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0043)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0048)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0049)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
br 0x0000$0200 always
