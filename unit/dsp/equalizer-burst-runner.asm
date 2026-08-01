// Stateful EDGE burst runner. Successive requests preserve both half-slot contexts.
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

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0006)] a0
cmp 0x0000u8 a0
br 0x0000$01A0 eq

// Initialize both half slots once per burst.
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
nop
nop
nop
nop
mov 0x$2000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0120) r0
mov 0x$000F r1
call 0x0000$0700 always
nop
nop
nop
nop
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0130) r0
mov 0x$000F r1
call 0x0000$0700 always
nop
nop
nop
nop
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0180) r0
mov 0x$000F r1
call 0x0000$0700 always
nop
nop
nop
nop
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0190) r0
mov 0x$000F r1
call 0x0000$0700 always
nop
nop
nop
nop
br 0x0000$01A0 always

segment p 01A0
nop
nop
nop
nop

// Acknowledge an old completion source and load this segment's branch data.
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0200) r0
mov 0x$007F r1
call 0x0000$0700 always
nop
nop
nop
nop
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0280) r0
mov 0x$003F r1
call 0x0000$0700 always
nop
nop
nop
nop

// Clear all output RAM before each independently captured segment.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$007F r1
call 0x0000$0760 always
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$003F r1
call 0x0000$0760 always
mov 0x$0080 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$003F r1
call 0x0000$0760 always

// Start one segment and capture its completion state.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l [0x$TEAK_EQ_CONF_CNT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0005)] a0
mov a0l [0x$TEAK_EQ_SC_SOUT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
or 0x$0005 a0
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
call 0x0000$0740 always
mov [0x$TEAK_EQ_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
mov [0x$TEAK_EQ_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]

// Capture both working banks for both half slots.
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
call 0x0000$0720 always
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0320) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$2000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0330) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0340) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0350) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0360) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0370) r0
mov 0x$000F r1
call 0x0000$0720 always
mov 0x$4001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0380) r0
mov 0x$000F r1
call 0x0000$0720 always

// Capture the complete hard, soft, and latency memories.
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
call 0x0000$0720 always
mov 0x$2801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0450) r0
mov 0x$00FF r1
call 0x0000$0720 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0560) r0
mov 0x$003F r1
call 0x0000$0720 always

mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
br 0x0000$0120 always

// Write r1+1 shared-RAM halfwords to ext1.
segment p 0700
bkrep r1 0x0000$0703
mov [r0++] a0
data 5ABA // mov a0l,ext1
ret always

// Read r1+1 ext1 halfwords to shared RAM.
segment p 0720
bkrep r1 0x0000$0723
data 5B55 // mov ext1,a0l
mov a0l [r0++]
ret always

// Wait for EQ_BUSY to clear.
segment p 0740
mov [0x$TEAK_EQ_STATUS] a0
and 0x$8000 a0
br 0x0000$0740 neq
ret always

// Write r1+1 zero words to the selected ext1 RAM region.
segment p 0760
bkrep r1 0x0000$0763
clr a0 always
data 5ABA // mov a0l,ext1
ret always
