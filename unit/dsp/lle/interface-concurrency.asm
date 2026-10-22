// Keep the DSP core busy while the MCU accesses the asynchronous interface.
segment p 0006
br 0x0000$0200 always

segment p 0100
dint
mov 0x$01FF a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x000e icr
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_EINTA0]
set 0x$000C st0
set 0x$0040 st2
eint
mov 0x$C101 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]

segment p 0180
inc a1 always
br 0x0000$0180 always

// Acknowledge MCU0 and copy the payload written before the interrupt assertion.
segment p 0200
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
retic true
