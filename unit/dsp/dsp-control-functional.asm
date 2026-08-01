// PMB8876 ROM-page selection, PRAM execution, and core idle/resume test.
segment p 0006
br 0x0000$0800 always

segment p 000E
br 0x0000$0820 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: context switching for INT0, INT1, and INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0041)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0042)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0043)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0044)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0045)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0046)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0047)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0048)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0049)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x004A)]

// Read one signature word from every data page while executing from PRAM.
clr a0 always
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov [0x$9000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0020)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov [0x$9000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0021)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov [0x$9000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0022)]
mov 0x$0003 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov [0x$9000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0023)]

// Program-page switches must not redirect execution outside PRAM.
mov 0x$0004 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov 0x$1001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0030)]
mov 0x$0008 a0l
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov 0x$1002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0031)]
clr a0 always
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov 0x$1000 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0032)]
mov [0x$9000] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0033)]

// MCU0 drives core INT0 after BRANCH. It must be enabled before entering idle.
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_EINTA0]

// DSP_CTRL stops only the core. INT0 must resume at the interrupted PRAM point.
mov 0x$5AA5 a0l
mov a0l r3
mov 0x$1111 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0040)]
eint
clr a0 always
mov a0l [0x$TEAK_DSP_CTRL]
nop
nop
nop
nop
nop
nop
nop
nop
mov r3 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0042)]
mov 0x$3333 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0043)]

// Timer 2 keeps running while only the core clock is disabled and wakes INT1.
mov 0x$0008 a0l
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_EINT1]
clr a0 always
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$0400 a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$4444 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0044)]
mov 0x$A55A a0l
mov a0l r3
mov a0l [0x$TEAK_DSP_CTRL]
nop
nop
nop
nop
mov r3 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0046)]

// A long-running Timer 2 event distinguishes core idle from whole-subsystem clock off.
mov 0x$0008 a0l
mov a0l [0x$TEAK_INT_RINT1]
clr a0 always
mov a0l [0x$TEAK_TMR2_CNT]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_TMR2_MAX]
mov 0x$0001 a0l
mov a0l [0x$TEAK_TMR2_CTRL]
mov 0x$7777 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0047)]
mov 0x$C33C a0l
mov a0l r3
clr a0 always
mov a0l [0x$TEAK_DSP_CTRL]
nop
nop
nop
nop
mov r3 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0048)]
mov 0x$9999 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0049)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$07F0 always

segment p 07F0
br 0x0000$07F0 always

segment p 0800
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov 0x$2222 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0041)]
data 45D0 // reti always,context.

segment p 0820
mov 0x$0008 a0l
mov a0l [0x$TEAK_INT_RINT1]
clr a0 always
mov a0l [0x$TEAK_TMR2_CTRL]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x004A)] a0
add 0x$0001 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x004A)]
mov 0x$5555 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0045)]
data 45D0 // reti always,context.
