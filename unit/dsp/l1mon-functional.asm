// Minimal test harness for the autonomous PMB8876 Mask ROM monitoring path.
segment p 0006
br 0x0000$25A4 always

segment p 000E
br 0x0000$25A4 always

segment p 0016
br 0x0000$23C3 always

segment p 0100
mov 0x$0001 st0
mov 0x$007D st1
mov 0x$0000 st2
data 4F90 // mov #0x10,icr: standard Mask ROM startup interrupt context.
mov 0x$7C14 sp
call 0x0000$2783 always // Initialize fixed Mask ROM scheduler and Baseband state.

mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400) a0l
mov a0l [0x$7000]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x00BF)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
mov a0l [0x$TEAK_TMR2_CTRL]
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]

// Fixed Mask ROM extension hooks used only as execution-path observers.
mov 0x$0376 a0l
mov a0l [0x$7776]
mov 0x$0300 a0l
mov a0l [0x$77D7]
mov 0x$0320 a0l
mov a0l [0x$77D8]
mov 0x$0340 a0l
mov a0l [0x$77DC]
mov 0x$035A a0l
mov a0l [0x$77BF]
set 0x$0040 st2
set 0x$000E st0
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$0C07 a0l
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
eint
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x000F)]
br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x00BF)] a0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)] a1
cmp a1 a0
br 0x0000$021D eq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
mov [0x$7000] a0
mov a0l r1
mov 0x$0005 a0l
mov a0l [r1++]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)] a0
mov a0l [r1++]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [r1++]
mov r1 a0l
mov a0l [0x$7000]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0028)]
mov [0x$TEAK_BB_CTRL] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0029)]
br 0x0000$0200 always

segment p 0300
// BBHI observer.
data 5E41 // push r1
mov [0x$7000] a0
mov a0l r1
mov 0x$0001 a0l
mov a0l [r1++]
mov a1l [r1++]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [r1++]
mov r1 a0l
mov a0l [0x$7000]
data 5E61 // pop r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov [0x$TEAK_BB_WR_POINTER] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002A)]
ret always

// BBLO observer.
data 5E41 // push r1
mov [0x$7000] a0
mov a0l r1
mov 0x$0002 a0l
mov a0l [r1++]
mov a1l [r1++]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [r1++]
mov r1 a0l
mov a0l [0x$7000]
data 5E61 // pop r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]
mov [0x$TEAK_BB_WR_POINTER] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002B)]
ret always

// Monitoring algorithm observer.
data 5E41 // push r1
mov [0x$7000] a0
mov a0l r1
mov 0x$0003 a0l
mov a0l [r1++]
mov a1l [r1++]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [r1++]
mov r1 a0l
mov a0l [0x$7000]
data 5E61 // pop r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0024)]
ret always

// Scheduler insertion observer.
data 5E41 // push r1
mov [0x$7000] a0
mov a0l r1
mov 0x$0004 a0l
mov a0l [r1++]
mov a1l [r1++]
mov [0x$TEAK_TMR2_CNT] a0
mov a0l [r1++]
mov r1 a0l
mov a0l [0x$7000]
data 5E61 // pop r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0025)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0026)]
ret always

// Runtime command handler observer.
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002C)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x002D)]
ret always

segment d 7772
data 0000
data 0000
data 0000
data 0000
data 0000

segment d 77BF
data 0000
data 0000
