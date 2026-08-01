// Parameterized single-run channel-decoder algorithm probe.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0800)]
br 0x0000$0120 always

segment p 0120
// Reset the decoder and program all eight butterfly-reference registers.
mov 0x$0100 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x080A)]
mov 0x$0003 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
nop
nop
nop
nop
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0810) r0
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY0]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY1]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY2]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY3]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY4]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY5]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY6]
mov [r0++] a0
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY7]

// Load old and canary/new metric banks using the Mask ROM packing mode.
mov 0x$1001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0820) r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0804)] a0
dec a0 always
mov a0l r1
call 0x0000$0600 always
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0860) r0
call 0x0000$0600 always

// Snapshot both banks before decoding to distinguish RAM transport from ACS writes.
mov 0x$1801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0B00) r0
call 0x0000$0610 always
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0B40) r0
call 0x0000$0610 always

// Load two packed bytes to each region as Mask ROM does: SIN0/SIN1 and SIN2/zero.
mov 0x$2001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x08A0) r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0806)] a0
shl a0 always
dec a0 always
mov a0l r1
call 0x0000$0600 always
mov 0x$0004 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x08E0) r0
call 0x0000$0600 always

// Snapshot packed soft inputs before decoding to verify byte order and FIFO length.
mov 0x$2801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0B80) r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0806)] a0
shl a0 always
dec a0 always
mov a0l r1
call 0x0000$0610 always
mov 0x$0004 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0BC0) r0
call 0x0000$0610 always

// Fill the requested traceback range so incomplete pipeline output stays visible.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0805)] a0
dec a0 always
mov a0l r1
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0807)] a0
call 0x0000$0620 always

// Start with caller-selected DEC_64/OFLOW flags and wait for completion.
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0802)] a0
mov a0l [0x$TEAK_CHDEC_CONF_CNT]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0803)] a0
or 0x$0005 a0
mov a0l [0x$TEAK_CHDEC_CONF2]
mov [0x$TEAK_CHDEC_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A40)]
mov [0x$TEAK_CHDEC_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A41)]
call 0x0000$0800 always
mov [0x$TEAK_CHDEC_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A42)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A43)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x080B)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x080B)]

// Capture both complete metric banks.
mov 0x$1801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0940) r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0804)] a0
dec a0 always
mov a0l r1
call 0x0000$0610 always
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0980) r0
call 0x0000$0610 always

// Capture the requested number of traceback words.
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x09C0) r0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0805)] a0
dec a0 always
mov a0l r1
call 0x0000$0610 always

br 0x0000$0700 always

segment p 0700
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0809)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0809)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0808)] a0
cmp 0x0000u8 a0
br 0x0000$0720 eq
dec a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0808)]
br 0x0000$0120 always

segment p 0720
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0801)]
br 0x0000$0900 always

segment p 0600
bkrep r1 0x0000$0603
mov [r0++] a0
data 5A9A // mov a0l,ext0
ret always

segment p 0610
bkrep r1 0x0000$0613
data 5B54 // mov ext0,a0l
mov a0l [r0++]
ret always

segment p 0620
bkrep r1 0x0000$0624
data 5A9A // mov a0l,ext0
nop
nop
ret always

segment p 0800
mov [0x$TEAK_CHDEC_STATUS] a0
and 0x$4000 a0
br 0x0000$0800 neq
ret always

segment p 0900
br 0x0000$0900 always
