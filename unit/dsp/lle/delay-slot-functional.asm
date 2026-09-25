// Delayed return with the arithmetic delay slots used by the EL71 DSP firmware.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
clr a0 always
mov 0x$1234 r0
addh r0 a0
call 0x0000$0200 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0h a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov st0 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0180 always

segment p 0180
br 0x0000$0180 always

segment p 0200
retd
sqr a0h a0
pacr a0 always
