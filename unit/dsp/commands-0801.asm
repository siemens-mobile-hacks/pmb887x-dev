// Command pipe interrupt vectors in PMB8876 Program Mask ROM 0801.
segment p 0006
br 0x0000$24B0 always

segment p 000E
br 0x0000$24B0 always

segment p 0016
br 0x0000$23C3 always

// Minimal startup: acknowledge BRANCH, enable interrupts, and wait.
segment p 0100
set 0x$0040 st2
set 0x$000E st0
mov 0x$0007 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
eint
br 0x0000$010D always

// USER_0 copies its first parameter to a test result in Shared RAM.
segment p 0180
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
ret always

// USER_0 command slot (ID 64).
segment p 1FFA
br 0x0000$0180 always
