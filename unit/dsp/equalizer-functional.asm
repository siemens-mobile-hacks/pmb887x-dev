// Equalizer ext1 memory transport, processing, and natural completion interrupt.
segment p 0006
br 0x0000$0A00 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0700)]

// RAM1 RX, unpacked 16-bit transfers.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$0000 a0l
data 5ABA // mov a0l,ext1
mov 0x$0001 a0l
data 5ABA // mov a0l,ext1
mov 0x$7FFF a0l
data 5ABA // mov a0l,ext1
mov 0x$8000 a0l
data 5ABA // mov a0l,ext1
mov 0x$A55A a0l
data 5ABA // mov a0l,ext1
mov 0x$5AA5 a0l
data 5ABA // mov a0l,ext1
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0710) r0
call 0x0000$0800 always

// RAM1 BPAR has an independent pointer.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$1357 a0l
data 5ABA // mov a0l,ext1
mov 0x$2468 a0l
data 5ABA // mov a0l,ext1
mov 0x$369C a0l
data 5ABA // mov a0l,ext1
mov 0x$47AD a0l
data 5ABA // mov a0l,ext1
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0720) r0
call 0x0000$0820 always

// RAM2 SOUT, packed 8-bit transfers.
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$0000 a0l
data 5ABA // mov a0l,ext1
mov 0x$007F a0l
data 5ABA // mov a0l,ext1
mov 0x$0080 a0l
data 5ABA // mov a0l,ext1
mov 0x$00FF a0l
data 5ABA // mov a0l,ext1
mov 0x$0055 a0l
data 5ABA // mov a0l,ext1
mov 0x$00AA a0l
data 5ABA // mov a0l,ext1
mov 0x$2801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0730) r0
call 0x0000$0800 always

// RAM2 HOUT, unpacked transfers.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$1122 a0l
data 5ABA // mov a0l,ext1
mov 0x$3344 a0l
data 5ABA // mov a0l,ext1
mov 0x$5566 a0l
data 5ABA // mov a0l,ext1
mov 0x$7788 a0l
data 5ABA // mov a0l,ext1
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0740) r0
call 0x0000$0820 always

// Packed working-RAM transfers and bank/base isolation.
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$1122 a0l
data 5ABA // mov a0l,ext1
mov 0x$3344 a0l
data 5ABA // mov a0l,ext1
mov 0x$5566 a0l
data 5ABA // mov a0l,ext1
mov 0x$7788 a0l
data 5ABA // mov a0l,ext1
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
mov 0x$89AB a0l
data 5ABA // mov a0l,ext1
mov 0x$CDEF a0l
data 5ABA // mov a0l,ext1
mov 0x$0F1E a0l
data 5ABA // mov a0l,ext1
mov 0x$2D3C a0l
data 5ABA // mov a0l,ext1
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0750) r0
call 0x0000$0820 always
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0760) r0
call 0x0000$0820 always
mov 0x$1801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0770) r0
call 0x0000$0820 always

// Enable the real EQ completion interrupt on INT0.
dint
data 4F8E // mov #0x0E,icr: enable context switching for INT0/INT1/INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0784)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0785)]
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_EINTA0]
eint

// Reset and initialize every path, metric, and branch-metric working area in both banks.
mov 0x$0003 a0l
mov a0l [0x$TEAK_EQ_CONF2]
nop
nop
nop
nop
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$4000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$4001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$2000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$2001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$1000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$0800 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$8000 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x003fu8
data 2D00 // mov +0x00,ext1
mov 0x$8001 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x003fu8
data 2D00 // mov +0x00,ext1

// Sanitize the output regions so short-operation unwritten slots are deterministic.
mov 0x$0001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D00 // mov +0x00,ext1

// Initialize all partial sums and received complex values.
mov 0x$1001 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D03 // mov +0x03,ext1
rep 0x000fu8
data 2DF9 // mov -0x07,ext1
rep 0x000fu8
data 2D0B // mov +0x0B,ext1
rep 0x000fu8
data 2DED // mov -0x13,ext1
rep 0x000fu8
data 2D1D // mov +0x1D,ext1
rep 0x000fu8
data 2DDB // mov -0x25,ext1
rep 0x000fu8
data 2D35 // mov +0x35,ext1
rep 0x000fu8
data 2DB9 // mov -0x47,ext1
mov 0x$0004 a0l
mov a0l [0x$TEAK_EQ_CONF1]
rep 0x000fu8
data 2D11 // mov +0x11,ext1
rep 0x000fu8
data 2D23 // mov +0x23,ext1
rep 0x000fu8
data 2DE3 // mov -0x1D,ext1
rep 0x000fu8
data 2D49 // mov +0x49,ext1

mov 0x$0000 a0l
mov a0l [0x$TEAK_EQ_SC_SOUT]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF_CNT]
mov 0x$0005 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov [0x$TEAK_EQ_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0780)]
mov [0x$TEAK_EQ_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0781)]
call 0x0000$0840 always
mov [0x$TEAK_EQ_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0782)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0783)]

// Capture sixteen hard and soft output words for hardware-golden discovery.
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0020 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0790) r0
call 0x0000$0860 always
mov 0x$0801 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov 0x$0010 a0l
mov a0l [0x$TEAK_EQ_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x07A0) r0
call 0x0000$0860 always

mov 0x$0101 a0l
mov a0l [0x$TEAK_EQ_CONF2]
mov [0x$TEAK_EQ_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0786)]
nop
nop
nop
nop
mov [0x$TEAK_EQ_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0787)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0701)]
br 0x0000$0900 always

// Copy six ext1 values to shared RAM.
segment p 0800
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
ret always

// Copy four ext1 values to shared RAM.
segment p 0820
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
ret always

// Wait for EQ_BUSY to clear after EQ_ON has been accepted.
segment p 0840
mov [0x$TEAK_EQ_STATUS] a0
and 0x$8000 a0
br 0x0000$0840 neq
ret always

// Copy sixteen ext1 values to shared RAM.
segment p 0860
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
data 5B55 // mov ext1,a0l
mov a0l [r0++]
ret always

segment p 0900
br 0x0000$0900 always

// INT0 handler records and acknowledges a natural EQ completion.
segment p 0A00
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0785)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0784)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0784)]
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTA0]
data 45D0 // reti always,context.
