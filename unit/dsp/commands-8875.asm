// Command pipe interrupt vectors in PMB8875 Program Mask ROM.
segment p 0006
br 0x0000$14B0 always

segment p 000E
br 0x0000$14B0 always

segment p 0016
br 0x0000$13C3 always

// Minimal startup: acknowledge BRANCH, enable interrupts, and wait.
segment p 0100
set 0x$0040 st2
set 0x$000E st0
mov 0x$0007 a0l
mov a0l [0x$E601]
mov 0x$0001 a0l
mov a0l [0x$E692]
eint
br 0x0000$010D always

// First user command copies its first parameter to a test result in Shared RAM.
segment p 0180
mov [r0] a0
mov a0l [0x$DB00]
ret always

// First user command slot (ID 35).
segment p 0FE0
br 0x0000$0180 always
