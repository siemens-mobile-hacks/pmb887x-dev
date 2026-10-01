// Busy loop that publishes core progress and the independently clocked Timer 2 count.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$FFFF a1l
mov a1l [0x$TEAK_TMR2_MAX]
clr a1 always
mov a1l [0x$TEAK_TMR2_CNT]
mov 0x$0001 a1l
mov a1l [0x$TEAK_TMR2_CTRL]
mov 0x$A55A a1l
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
mov [0x$TEAK_TMR2_CNT] a1
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
br 0x0000$0117 always
