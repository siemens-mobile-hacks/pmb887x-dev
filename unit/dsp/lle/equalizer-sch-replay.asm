// Replays an equalizer command stream from shared word 0x0100 and stores every
// ext1 read from shared word 0x0A00. Commands: 0 end, 1 register write (address,
// value), 2 ext1 writes (count - 1, values), 3 ext1 reads (count - 1), 4 wait for
// EQ_BUSY to clear, 5 register read (address).
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0100) r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0A00) r2
br 0x0000$0200 always

segment p 0200
mov [r0++] a0
cmp 0x$0001 a0
br 0x0000$0300 eq
cmp 0x$0002 a0
br 0x0000$0320 eq
cmp 0x$0003 a0
br 0x0000$0340 eq
cmp 0x$0004 a0
br 0x0000$0360 eq
cmp 0x$0005 a0
br 0x0000$0380 eq
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0280 always

segment p 0280
br 0x0000$0280 always

segment p 0300
mov [r0++] a1
mov a1l r1
mov [r0++] a0
mov a0l [r1]
br 0x0000$0200 always

segment p 0320
mov [r0++] a1
mov a1l r1
bkrep r1 0x0000$0325
mov [r0++] a0
mov a0l ext1
br 0x0000$0200 always

segment p 0340
mov [r0++] a1
mov a1l r1
bkrep r1 0x0000$0345
mov ext1 a0l
mov a0l [r2++]
br 0x0000$0200 always

segment p 0360
mov [0x$TEAK_EQ_STATUS] a0
and 0x$8000 a0
br 0x0000$0360 neq
br 0x0000$0200 always

segment p 0380
mov [r0++] a1
mov a1l r1
mov [r1] a0
mov a0l [r2++]
br 0x0000$0200 always
