// Modulo addressing after immediate ST2, CFGI, and CFGJ writes.
//
// Every case starts at the last address of a four-word ring and post-increments
// the register once. MODI or MODJ = 3 selects the ring 0x0100..0x0103.
//
// Results in Shared RAM:
//   0x0401 R5 after SET enables R5 modulo
//   0x0402 R5 after MOV #0 to ST2 disables it
//   0x0403 R5 after MOV #imm to ST2 enables it again
//   0x0404 R5 after MOV #imm to CFGJ widens MODJ to 7
//   0x0405 R0 after MOV #imm to CFGI widens MODI to 7
//   0x0400 complete marker
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
mov 0x$0180 cfgj
mov 0x$0180 cfgi

set 0x$0020 st2
mov 0x$0103 r5
modr [r5++]
mov r5 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]

mov 0x$0000 st2
mov 0x$0103 r5
modr [r5++]
mov r5 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]

rst 0x$0020 st2
mov 0x$0020 st2
mov 0x$0103 r5
modr [r5++]
mov r5 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]

set 0x$0020 st2
mov 0x$0380 cfgj
mov 0x$0103 r5
modr [r5++]
mov r5 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0404)]

set 0x$0001 st2
mov 0x$0380 cfgi
mov 0x$0103 r0
modr [r0++]
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]

mov 0x$0000 st2
mov 0x$600D a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0180 always

segment p 0180
br 0x0000$0180 always
