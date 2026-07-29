// Forward Shared RAM requests to the PMB8876 Mask ROM 0801 DSP-to-MCU register.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$DE92]
mov 0x$A55A a0l
mov a0l [0x$D301]
mov [0x$D300] a0
cmp 0x$0000 a0
br 0x0000$0108 eq
mov a0l [0x$DE10]
clr a0 always
mov a0l [0x$D300]
br 0x0000$0108 always
