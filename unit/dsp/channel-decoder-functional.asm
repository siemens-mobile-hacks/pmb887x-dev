// Channel-decoder ext0 memory transport and packing modes.
segment p 0006
br 0x0000$0A00 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0600)]

// RAM2 SIN01, unpacked 16-bit transfers. Writes intentionally follow pointer reset directly.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$0000 a0l
data 5A9A // mov a0l,ext0
mov 0x$0001 a0l
data 5A9A // mov a0l,ext0
mov 0x$7FFF a0l
data 5A9A // mov a0l,ext0
mov 0x$8000 a0l
data 5A9A // mov a0l,ext0
mov 0x$A55A a0l
data 5A9A // mov a0l,ext0
mov 0x$5AA5 a0l
data 5A9A // mov a0l,ext0
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0610) r0
call 0x0000$0800 always

// RAM2 SIN2 is independent of SIN01.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0004 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$1357 a0l
data 5A9A // mov a0l,ext0
mov 0x$2468 a0l
data 5A9A // mov a0l,ext0
mov 0x$369C a0l
data 5A9A // mov a0l,ext0
mov 0x$47AD a0l
data 5A9A // mov a0l,ext0
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0004 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0620) r0
call 0x0000$0820 always

// RAM2 packs two 8-bit writes and sign-extends each unpacked read.
mov 0x$2001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$0000 a0l
data 5A9A // mov a0l,ext0
mov 0x$007F a0l
data 5A9A // mov a0l,ext0
mov 0x$0080 a0l
data 5A9A // mov a0l,ext0
mov 0x$00FF a0l
data 5A9A // mov a0l,ext0
mov 0x$0055 a0l
data 5A9A // mov a0l,ext0
mov 0x$00AA a0l
data 5A9A // mov a0l,ext0
mov 0x$2801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0630) r0
call 0x0000$0800 always

// RAMW1 packs low then high halfword into each 32-bit metric entry.
mov 0x$1001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$1122 a0l
data 5A9A // mov a0l,ext0
mov 0x$3344 a0l
data 5A9A // mov a0l,ext0
mov 0x$5566 a0l
data 5A9A // mov a0l,ext0
mov 0x$7788 a0l
data 5A9A // mov a0l,ext0
mov 0x$1801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0640) r0
call 0x0000$0820 always

// RAMW2 uses the same pointer command with RES_RW1_RW2 set.
mov 0x$1001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$89AB a0l
data 5A9A // mov a0l,ext0
mov 0x$CDEF a0l
data 5A9A // mov a0l,ext0
mov 0x$0F1E a0l
data 5A9A // mov a0l,ext0
mov 0x$2D3C a0l
data 5A9A // mov a0l,ext0
mov 0x$1801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0650) r0
call 0x0000$0820 always

// Re-select RAMW1 and prove that RAMW2 writes did not alias it.
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0660) r0
call 0x0000$0820 always

// RAMW1 unpacked mode preserves one 16-bit metric per ext0 transfer.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$1357 a0l
data 5A9A // mov a0l,ext0
mov 0x$8ACE a0l
data 5A9A // mov a0l,ext0
mov 0x$2468 a0l
data 5A9A // mov a0l,ext0
mov 0x$9BDF a0l
data 5A9A // mov a0l,ext0
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0672) r0
call 0x0000$0820 always

// Run a 16-state decode with an artificial reference table and a noiseless K=5 stream.
dint
data 4F8E // mov #0x0E,icr: enable context switching for INT0/INT1/INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0684)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0685)]
mov a0l [0x$TEAK_INT_EINTA0]
mov 0x$0100 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_EINTA0]
eint

mov 0x$0101 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
nop
nop
nop
nop
mov 0x$0003 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
nop
nop
nop
nop
// Verify a direct write/read round trip at the traceback base.
mov 0x$0001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$A55A a0l
data 5A9A // mov a0l,ext0
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
data 5B54 // mov ext0,a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x068F)]
// Load 16 old metrics in RAMW1, clear the ping-pong RAMW2 bank, then program eight references.
mov 0x$1001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0080 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$0780 r0
rep 0x000fu8
data 1E88 // mov (r0++),ext0
mov 0x$0081 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
clr a0 always
rep 0x000fu8
data 5A9A // mov a0l,ext0
mov 0x$6CA0 a0l
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY0]
mov 0x$0AC6 a0l
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY1]
clr a0 always
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY2]
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY3]
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY4]
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY5]
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY6]
mov a0l [0x$TEAK_CHDEC_REF_BR_BFLY7]
mov [0x$TEAK_CHDEC_REF_BR_BFLY0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0686)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY1] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0687)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0688)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY3] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0689)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY4] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x068A)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY5] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x068B)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY6] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x068C)]
mov [0x$TEAK_CHDEC_REF_BR_BFLY7] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x068D)]

// Load 16 input triplets; the first 15 are processed by the configured decode.
mov 0x$2001 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0002 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$0700 r0
rep 0x001fu8
data 1E88 // mov (r0++),ext0
mov 0x$0004 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
mov 0x$0740 r0
rep 0x001fu8
data 1E88 // mov (r0++),ext0

mov 0x$000F a0l
mov a0l [0x$TEAK_CHDEC_CONF_CNT]
mov 0x$0005 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov [0x$TEAK_CHDEC_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0680)]
mov [0x$TEAK_CHDEC_STATUS] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0681)]
call 0x0000$0840 always
mov [0x$TEAK_CHDEC_STAT_CNT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0682)]
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0683)]

// Read two 8-bit traceback halves for each of 15 timestamps.
mov 0x$0801 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov 0x$0008 a0l
mov a0l [0x$TEAK_CHDEC_CONF1]
nop
nop
nop
nop
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0690) r0
call 0x0000$0800 always
call 0x0000$0800 always
call 0x0000$0800 always
call 0x0000$0800 always
call 0x0000$0800 always

// RES_ALL clears itself and disables the accelerator within four cycles.
mov 0x$0101 a0l
mov a0l [0x$TEAK_CHDEC_CONF2]
mov [0x$TEAK_CHDEC_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0670)]
nop
nop
nop
nop
mov [0x$TEAK_CHDEC_CONF2] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0671)]

mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0602)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0601)]
br 0x0000$0900 always

// Copy six ext0 values to shared RAM.
segment p 0800
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
ret always

// Copy four ext0 values to shared RAM.
segment p 0820
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
data 5B54 // mov ext0,a0l
mov a0l [r0++]
ret always

// Wait for DEC_BUSY to clear after the decoder has accepted DEC_ON.
segment p 0840
mov [0x$TEAK_CHDEC_STATUS] a0
and 0x$4000 a0
br 0x0000$0840 neq
ret always

segment p 0900
br 0x0000$0900 always

// INT0 handler records and acknowledges a natural CHADEC completion.
segment p 0A00
mov [0x$TEAK_INT_FINTA0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0685)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0684)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0684)]
mov 0x$0100 a0l
mov a0l [0x$TEAK_INT_RINTA0]
data 45D0 // reti always,context.

// Asymmetric signed soft inputs avoid branch-metric ties at every timestamp.
segment d 0700
data 000D
data 00C7
data 009B
data 002C
data 003F
data 0012
data 00F7
data 006F
data 0057
data 00E8
data 00BC
data 00FD
data 001D
data 0049
data 0081
data 0006
data 0005
data 00A8
data 0068
data 0033
data 00D2
data 0061
data 0047
data 0093
data 00AC
data 0027
data 0016
data 00BE
data 0076
data 00F2
data 00DD
data 0086

segment d 0740
data 005C
data 0000
data 0007
data 0000
data 00B3
data 0000
data 00DC
data 0000
data 008D
data 0000
data 0036
data 0000
data 00D6
data 0000
data 0077
data 0000
data 0021
data 0000
data 00F4
data 0000
data 001A
data 0000
data 00FF
data 0000
data 007D
data 0000
data 00A3
data 0000
data 003D
data 0000
data 0030
data 0000

// Initial metrics: all states neutral in both ping-pong banks.
segment d 0780
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
