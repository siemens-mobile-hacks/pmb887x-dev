// Block-repeat control: ICR LP aborts, branches around the loop end, and lc reads.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
br 0x0000$0200 always

// 0: ICR LP write at the first body word aborts a single loop.
segment p 0200
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0004u8 0x0000$020F
mov 0x0010 icr
inc a1 always
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov 0x0010 icr
nop
br 0x0000$0280 always

// 1: ICR LP write three words before the end aborts a single loop.
segment p 0280
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0004u8 0x0000$028F
inc a1 always
mov 0x0010 icr
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
mov 0x0010 icr
nop
br 0x0000$0300 always

// 2: ICR LP write in the inner loop aborts both levels.
segment p 0300
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0002u8 0x0000$0316
bkrep 0x0003u8 0x0000$0311
inc a1 always
mov 0x0010 icr
nop
nop
nop
nop
inc a0 always
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
mov 0x0010 icr
nop
br 0x0000$0380 always

// 3: ICR LP write in the innermost of four levels aborts all levels.
segment p 0380
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0001u8 0x0000$03A2
bkrep 0x0001u8 0x0000$039D
bkrep 0x0001u8 0x0000$0399
bkrep 0x0002u8 0x0000$0395
inc a1 always
mov 0x0010 icr
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
inc a0 always
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)]
mov 0x0010 icr
nop
br 0x0000$0400 always

// 4: ICR write without LP leaves the loop running.
segment p 0400
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0004u8 0x0000$040F
mov 0x000e icr
inc a1 always
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0434)]
mov 0x0010 icr
nop
br 0x0000$0480 always

// 5: branch out and back to the last instruction keeps looping.
segment p 0480
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0003u8 0x0000$048F
inc a1 always
br 0x0000$0492 always
nop
nop
nop
br 0x0000$0495 always
inc a0 always
br 0x0000$048F always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0438)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0439)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x043C)]
mov 0x0010 icr
nop
br 0x0000$0500 always

// 6: branch to the last instruction continues the loop.
segment p 0500
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0003u8 0x0000$0510
inc a1 always
br 0x0000$0510 always
inc a0 always
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0440)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0441)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0442)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0443)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0444)]
mov 0x0010 icr
nop
br 0x0000$0580 always

// 7: branch out of the body without break leaves the loop active.
segment p 0580
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0003u8 0x0000$058F
inc a1 always
br 0x0000$0590 always
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0448)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0449)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x044A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x044B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x044C)]
mov 0x0010 icr
nop
br 0x0000$0600 always

// 8: lc reads the loop counter inside the body and after completion.
segment p 0600
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0003u8 0x0000$060F
inc a1 always
mov lc r1
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0450)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0451)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0452)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0453)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0454)]
mov 0x0010 icr
nop
br 0x0000$0680 always

// 9: break at the outer level keeps lc.
segment p 0680
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0005u8 0x0000$0694
inc a1 always
cmp 0x$0002 a1
br 0x0000$0691 neq
break
mov lc r1
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0458)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0459)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x045A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x045B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x045C)]
mov 0x0010 icr
nop
br 0x0000$0700 always

// 10: lc reads the outer counter after the inner loop completes.
segment p 0700
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0002u8 0x0000$0716
bkrep 0x0003u8 0x0000$0710
inc a1 always
nop
nop
nop
nop
mov lc r1
inc a0 always
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0460)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0461)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0462)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0463)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0464)]
mov 0x0010 icr
nop
br 0x0000$0780 always

// 11: rep inside a block repeat body runs every repetition.
segment p 0780
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
mov 0x$0000 r1
bkrep 0x0003u8 0x0000$078F
rep 0x0002u8
inc a1 always
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0468)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0469)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x046A)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x046B)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x046C)]
mov 0x0010 icr
nop
br 0x0000$0F00 always

segment p 0F00
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0F04 always

segment p 0F04
br 0x0000$0F04 always
