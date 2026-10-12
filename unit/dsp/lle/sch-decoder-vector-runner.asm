// Direct Mask ROM SCH decoder harness.
//
// The ARM runner supplies the Mask ROM entry point, caller registers, and
// workspace bounds in Shared RAM and initializes every writable input.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x$0001 st0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0005)] a0
mov a0l st1
mov 0x$0000 st2
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0007)] a0
mov a0l r0
mov r0 sp
mov 0x$0000 sv
mov 0x$0000 cfgi
mov 0x$0000 cfgj
mov 0x$0000 lc
clr a0 always
clr a1 always
clr b0 always
clr b1 always
mov 0x$0000 r0
mov a0l x0
mov a0l y0
mpy y0 r0 a0
nop

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
mov a0l r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)] a0
mov a0l r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0003)] a0
mov a0l r2
clr a0 always
mov a0l [0x$TEAK_DSP_PAGE]
nop
nop
nop
nop
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)] a0
calla a0l

// Save the caller-visible result before using registers for the state copy.
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100)]
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0102)]

// Capture the complete TeakLite architectural state modified by the decoder.
// This core has X0/Y0, not X0/X1/Y0/Y1. A0L is already safe, so it is the
// scratch destination while the original A0H and all of A1 remain intact.
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0109)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010A)]
mov r2 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010B)]
mov r3 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010C)]
mov r4 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010D)]
mov r5 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010E)]
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x010F)]
mov y0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0110)]
mov sv r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0111)]
mov sp r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0112)]
mov st0 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0113)]
mov st1 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0114)]
mov st2 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0115)]
mov cfgi r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0116)]
mov cfgj r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0117)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0118)]
mov mixp r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0119)]

mov a0h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0101)]
mov a1h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0103)]
mov x0 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011A)]
mov a0h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011B)]
mov p* a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011C)]
mov a0h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011D)]
mov b0 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011E)]
mov a0h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x011F)]
mov b1 a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0120)]
mov a0h r7
mov r7 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0121)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)] a0
mov a0l r0
mov [r0++] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0104)]
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0105)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0004)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0106)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0006)] a0
mov a0l r0
mov [r0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0107)]

br 0x0000$0200 always

segment p 0200
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)] a0
mov a0l r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0180) r1
mov 0x$00BB r5
bkrep r5 0x0000$0209
mov [r0++] a0
mov a0l [r1++]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0108)]
br 0x0000$0300 always

segment p 0300
br 0x0000$0300 always
