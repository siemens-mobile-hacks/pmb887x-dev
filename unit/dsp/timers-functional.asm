// TIMER1/TIMER2 behavior and real INT1 delivery.
segment p 000E
br 0x0000$0700 always

segment p 0100
// Shared RAM and interrupt setup.
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$000E a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_EINT1]
data 4F84 // mov #4,icr: enable context switching for INT1.
set 0x$0008 st0
eint
mov 0x$FFFF r0

// TIMER1: standby, ordered compares, stop, terminal count, restart.
mov 0x$0002 a0l
mov a0l [0x$TEAK_TMR1_INT0]
mov 0x$0020 a0l
mov a0l [0x$TEAK_TMR1_INT1]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0304)]
mov 0x$0003 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0305)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0307)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0308)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0309)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030A)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030B)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030C)]
clr a0 always
mov a0l [0x$TEAK_TMR1_CTRL]
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030D)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030E)]

// Run TIMER1 to 0xFFF with IRQ delivery disabled.
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$000E a0l
mov a0l [0x$TEAK_INT_RINT1]
mov 0x$0FFF a0l
mov a0l [0x$TEAK_TMR1_INT0]
mov a0l [0x$TEAK_TMR1_INT1]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
mov 0x$0003 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030F)]
mov [0x$TEAK_TMR1_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0310)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0311)]
mov 0x$0003 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0312)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0313)]
clr a0 always
mov a0l [0x$TEAK_TMR1_CTRL]

br 0x0000$0400 always

segment p 0400
// TIMER2: small wrap sequence, write-ignore, IRQ, and stop.
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$000E a0l
mov a0l [0x$TEAK_INT_RINT1]
clr a0 always
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$0004 a0l
mov a0l [0x$TEAK_TMR2_MAX]
clr a0 always
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0314)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0315)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0316)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0317)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0318)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0319)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031A)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031B)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031C)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031D)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031E)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x031F)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0320)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0321)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0322)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0323)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0324)]
call 0x0000$0740 always
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033D)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033E)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033F)]
mov 0x$0008 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_EINT1]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov 0x$7000 a0l
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$7001 a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov [0x$TEAK_TMR2_MAX] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033B)]
mov [0x$TEAK_TMR2_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033C)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0325)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0326)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0327)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0328)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0329)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032A)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032B)]
rep 0x0020u8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032C)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032D)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032E)]
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x032F)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0330)]

// TIMER2 start above MAX: first 0xFFFF wrap is not a compare event.
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$000E a0l
mov a0l [0x$TEAK_INT_RINT1]
mov 0x$FFFC a0l
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$0003 a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
rep 0x00ffu8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0331)]
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0332)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0333)]
rep 0x00ffu8
nop
rep 0x00ffu8
nop
rep 0x00ffu8
nop
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0334)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0335)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0336)]
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]

// TIMER2:TIMER1 rate over the same instruction interval.
mov a0l [0x$TEAK_INT_EINT1]
mov 0x$0FFF a0l
mov a0l [0x$TEAK_TMR1_INT0]
mov a0l [0x$TEAK_TMR1_INT1]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
mov 0x$0003 a0l
mov a0l [0x$TEAK_TMR1_CTRL]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
clr a0 always
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0337)]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0338)]
rep r0
nop
rep r0
nop
rep r0
nop
rep r0
nop
mov [0x$TEAK_TMR1_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0339)]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x033A)]
clr a0 always
mov a0l [0x$TEAK_TMR1_CTRL]
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0720 always

// INT1 handler: record entry count and active source, acknowledge, restore context.
segment p 0700
mov [0x$TEAK_INT_FINT1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov a0l [0x$TEAK_INT_RINT1]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
data 45D0 // reti always,context.

segment p 0720
br 0x0000$0720 always

// Capture the first non-zero value after TIMER2 wraps to zero.
segment p 0740
mov [0x$TEAK_TMR2_CNT] a0
cmp 0x0000u8 a0
br 0x0000$0740 eq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0340)]
ret always
