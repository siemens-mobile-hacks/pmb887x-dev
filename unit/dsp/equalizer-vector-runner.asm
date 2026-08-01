// Parameterized equalizer runner for exact RAM, ACS, and output characterization.
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
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0305)]

// Reset all equalizer state and acknowledge a stale completion source.
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
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
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]

// Load all RAMW1 regions using packed 32-bit transfers.
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0800 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0110) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$2000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0120) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0130) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$8000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0140) r0
mov 0x$003F r1
call 0x0000$0800 always

// Load all RAMW2 regions.
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0180) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0190) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01A0) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$4001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01B0) r0
mov 0x$000F r1
call 0x0000$0800 always
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$8001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x01C0) r0
mov 0x$003F r1
call 0x0000$0800 always

// Load 64 complex branch partial sums and 32 complex received values.
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0200) r0
mov 0x$007F r1
call 0x0000$0800 always
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0280) r0
mov 0x$003F r1
call 0x0000$0800 always

// Clear all output storage so unwritten pipeline slots remain distinguishable.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$007F r1
call 0x0000$0880 always
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$003F r1
call 0x0000$0880 always
mov 0x$0080 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$003F r1
call 0x0000$0880 always

// Prove that later soft-output changes came from processing, not stale RAM.
mov 0x$2801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0580) r0
mov 0x$001F r1
call 0x0000$0820 always

// Start one caller-selected equalization run and snapshot its state transition.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l [0x$TEAK_EQ_CONF_CNT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0005)] a0
mov a0l [0x$TEAK_EQ_SC_SOUT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
or 0x$0005 a0
mov a0l [0x$TEAK_EQ_CONF2]
mov [0x$TEAK_EQ_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
mov [0x$TEAK_EQ_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
call 0x0000$0840 always
mov [0x$TEAK_EQ_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov [0x$TEAK_EQ_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0304)]
mov [0x$TEAK_EQ_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)]

// Capture every RAMW1 region.
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0800 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0310) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0320) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$2000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0330) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0340) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$8000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0350) r0
mov 0x$003F r1
call 0x0000$0820 always

// Capture every RAMW2 region.
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0390) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x03A0) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x03B0) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$4001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x03C0) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$8001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x03D0) r0
mov 0x$003F r1
call 0x0000$0820 always

// Capture 64 hard-output words, 192 packed soft bytes, and 64 latency words.
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410) r0
mov 0x$003F r1
call 0x0000$0820 always
call 0x0000$08A0 always
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0450) r0
mov 0x$00BF r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0510) r0
mov 0x$003F r1
call 0x0000$0820 always

// SQUAL is a 16-entry descending FIFO with one required wait state per read.
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0550) r0
mov 0x$000F r1
call 0x0000$0860 always

// Optionally continue to a second cumulative timestamp target without RES_EQ.
call 0x0000$08C0 always

mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0305)] a0
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

// Wait for EQ_BUSY to clear.
segment p 0840
mov [0x$TEAK_EQ_STATUS] a0
and 0x$8000 a0
br 0x0000$0840 neq
ret always

// Read r1+1 SQUAL FIFO entries with the required wait state.
segment p 0860
bkrep r1 0x0000$0865
mov [0x$TEAK_EQ_SQUAL] a0
mov a0l [r0++]
nop
ret always

// Write r1+1 zero words to the selected ext1 RAM region.
segment p 0880
bkrep r1 0x0000$0883
clr a0 always
data 5ABA // mov a0l,ext1
ret always

// Combined SOUT is already byte-packed and must bypass the normal RAM2 byte unpacker.
segment p 08A0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
and 0x$0400 a0
br 0x0000$08B0 eq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
and 0x$0600 a0
or 0x$0801 a0
mov a0l [0x$TEAK_EQ_CONF2]
ret always

segment p 08B0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
and 0x$0600 a0
or 0x$2801 a0
mov a0l [0x$TEAK_EQ_CONF2]
ret always

segment p 08C0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0006)] a0
cmp 0x0000u8 a0
br 0x0000$08C8 neq
ret always
nop
nop
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]

// Only RX and BPAR are refreshed between successive segments.
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0600) r0
mov 0x$007F r1
call 0x0000$0800 always
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0680) r0
mov 0x$003F r1
call 0x0000$0800 always

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0007)] a0
mov a0l [0x$TEAK_EQ_CONF_CNT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0009)] a0
mov a0l [0x$TEAK_EQ_SC_SOUT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0008)] a0
or 0x$0005 a0
mov a0l [0x$TEAK_EQ_CONF2]
call 0x0000$0840 always
mov [0x$TEAK_EQ_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0700)]
mov [0x$TEAK_EQ_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0701)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0702)]

// Capture both left metric/path banks to expose the internal ping-pong state.
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0710) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0720) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0730) r0
mov 0x$000F r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$4001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0740) r0
mov 0x$000F r1
call 0x0000$0820 always

mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0750) r0
mov 0x$001F r1
call 0x0000$0820 always
mov 0x$2801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0770) r0
mov 0x$001F r1
call 0x0000$0820 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0790) r0
mov 0x$000F r1
call 0x0000$0820 always
ret always
