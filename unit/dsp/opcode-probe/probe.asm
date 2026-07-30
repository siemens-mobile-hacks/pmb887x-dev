// Generated probe fixture for one patched first-word opcode.

// A taken absolute branch or call with a zero expansion reaches this trampoline.
segment p 0000
br 0x0000$0162 always

// Explicit TRAP and any implementation-defined vectoring record entry here.
segment p 0002
mov 0x0054u8 a0l
mov a0l [0x$D301]
reti always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$DE92]
mov 0x$0000 st0
mov 0x$0000 st1
mov 0x$0000 st2
mov 0x$D6A0 r0
mov 0x$D6B0 r1
mov 0x$D6C0 r2
mov 0x$D6D0 r3
mov 0x$D6E0 r4
mov 0x$D6F0 r5
mov 0x$D6C0 r7
mov 0x$D700 sp
mov 0x$0000 cfgi
mov 0x$0000 cfgj
mov 0x$0000 lc
mov 0x$0000 sv
load 0x0000 modi
load 0x0000 modj
load +0x0000 stepi
load +0x0000 stepj
load 0x0000u8 page
load 0x0000 ps
// Reset does not clear the regular or alternative register banks.
data 4B8F // banke r0,r1,r4,cfgi
mov 0x$D6A0 r0
mov 0x$D6B0 r1
mov 0x$D6E0 r4
mov 0x$0000 cfgi
data 4B8F // banke r0,r1,r4,cfgi
// Clear block-repeat state, context switching, and the non-reset core registers.
data 4F90 // mov #0x10,icr; clear LP and all block-repeat levels
nop
data 4F80 // mov #0,icr
nop
mov 0x$0000 a0l
data D298 // mov b0l,dvm
mov a0l x0
mov a0l y0
mov a0 b0
mov a0 b1
data 8040 // mpy y0,r0; establish a deterministic zero product
nop
rep 0x0000u8
nop
mov 0x0051u8 a0l
mov a0l [0x$D302]
mov 0x$0000 a0l
mov 0x$0000 a0h
mov 0x$0000 a1l
mov 0x$0000 a1h
// Set condition flags last so conditional raw words never inherit setup flags.
mov 0x$0000 st0
mov 0x$0000 st1
mov 0x$0000 st2
// @expansion-page-begin
br 0x0000$0160 always

// Deterministic backing for every backward relative branch/call target not
// occupied by setup code.
segment p 0154
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
// @expansion-page-end

// The ARM runner patches only this first word. The zero second word is both a
// safe NOP and a safe expansion value for every documented two-word family.
segment p 0160
data 0000
data 0000

// Reaching this path records fallthrough before the final completion marker.
// @post-begin
segment p 0162
mov 0x0052u8 a0l
mov a0l [0x$D303]
mov 0x0053u8 a0l
mov a0l [0x$D300]
br 0x0000$0168 always
// @post-end

// Forward relative targets fall through to a deterministic completion path.
// @forward-fill-begin
segment p 016A
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
br 0x0000$0162 always
// @forward-fill-end

// Scratch and stack words surrounding the configured SP.
segment d D6A0
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
data 0000
