// Block-repeat sequences that the TeakLite specification forbids.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
br 0x0000$0200 always

// 0: a fifth nested block repeat level.
segment p 0200
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
bkrep 0x0001u8 0x0000$0229
bkrep 0x0001u8 0x0000$0224
bkrep 0x0001u8 0x0000$0220
bkrep 0x0001u8 0x0000$021C
bkrep 0x0001u8 0x0000$0218
inc a1 always
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
nop
nop
nop
nop
inc a0 always
nop
nop
nop
nop
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
mov 0x0010 icr
nop
br 0x0000$0280 always

// 1: two block repeat levels share the last address.
segment p 0280
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
bkrep 0x0001u8 0x0000$0290
bkrep 0x0002u8 0x0000$0290
inc a1 always
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
mov 0x0010 icr
nop
br 0x0000$0300 always

// 2: break as the last instruction of the body.
segment p 0300
mov 0x$0003 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
bkrep 0x0003u8 0x0000$0310
inc a1 always
nop
nop
nop
break
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
mov 0x0010 icr
nop
br 0x0000$0380 always

// 3: a one-word block repeat body.
segment p 0380
mov 0x$0004 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
bkrep 0x0003u8 0x0000$038C
inc a1 always
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
mov 0x0010 icr
nop
br 0x0000$0400 always

// 4: a block repeat whose last address precedes it.
segment p 0400
mov 0x$0005 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
mov 0x0010 icr
nop
mov 0x0000 icr
nop
clr a0 always
clr a1 always
nop
bkrep 0x0003u8 0x0000$040A
inc a1 always
nop
nop
nop
inc a0 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov lc r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
mov icr a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
mov 0x0010 icr
nop
br 0x0000$0F00 always

segment p 0F00
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0F04 always

segment p 0F04
br 0x0000$0F04 always
