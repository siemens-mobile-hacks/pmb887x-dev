// Parameterized runner for equalizer hard/soft output combining and packing.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0120 always

segment p 0120
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
cmp 0x0000u8 a0
br 0x0000$0120 eq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)]

// Reset the accelerator and all output pointers.
mov 0x$0101 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
mov 0x$0003 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop

// Load 64 unpacked hard-output words.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r0
mov 0x$003F r1
call 0x0000$0800 always

// Load 192 soft-output bytes, packed by the RAM2 write interface.
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0140) r0
mov 0x$00BF r1
call 0x0000$0800 always

// Read the original sign-extended bytes through the normal RAM2 unpacker.
mov 0x$2801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300) r0
mov 0x$00BF r1
call 0x0000$0820 always

// Combined output is already packed, so PC_EQ_1 must remain clear.
mov 0x$0C01 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400) r0
mov 0x$005F r1
call 0x0000$0820 always

// Repeat with the packing layout used next to a training sequence.
mov 0x$0E01 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0500) r0
mov 0x$005F r1
call 0x0000$0820 always

clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
br 0x0000$0120 always

// Write r1+1 shared-RAM halfwords to ext1.
segment p 0800
bkrep r1 0x0000$0803
mov [r0++] a0
data 5ABA // mov a0l,ext1
ret always

// Read r1+1 ext1 halfwords to shared RAM.
segment p 0820
bkrep r1 0x0000$0823
data 5B55 // mov ext1,a0l
mov a0l [r0++]
ret always
