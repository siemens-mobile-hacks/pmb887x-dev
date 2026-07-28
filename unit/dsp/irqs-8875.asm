// Acknowledge BRANCH, then forward Shared RAM requests to the PMB8875
// DSP-to-MCU interrupt register. Bits 0..3 map to SCU DSP_SRC0..3.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$E692]
mov 0x$A55A a0l
mov a0l [0x$DB01]
mov [0x$DB00] a0
cmp 0x$0000 a0
br 0x0000$0108 eq
mov a0l [0x$E610]
clr a0 always
mov a0l [0x$DB00]
br 0x0000$0108 always
