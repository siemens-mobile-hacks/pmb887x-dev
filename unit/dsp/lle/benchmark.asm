// Dense 16x16 fixed-point matrix-vector benchmark.
segment p 0006
br 0x0000$0100 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
clr a0 always
mov a0l st1
mov a0l st2
call 0x0000$0400 always
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
br 0x0000$0180 always

segment p 0180
mov 0x$B001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
cmp 0x$B002 a0
br 0x0000$0180 neq
br 0x0000$0200 always

segment p 0200
mov 0x$B004 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
mov 0x$1000 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]

mov 0x$TEAK_YRAM_BASE r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0010) r3
clr a1 always
mov 0x0010u8 a1l

mov 0x$TEAK_ADDR(TEAK_XRAM_BASE, 0x0100) r2
clr a0 always
mpy [r4++] [r2++] a0
rep 0x000eu8
mac [r4++] [r2++] a0
add p* a0
mov a0h [r3++]
dec a1 always
br 0x0000$020E neq

mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)] a0
dec a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0002)]
br 0x0000$0208 neq
mov 0x$B003 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0000)]
br 0x0000$0300 always

segment p 0300
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0001)] a0
br 0x0000$0300 neq
br 0x0000$0180 always

// Snapshot the loaded X vector and first Y-matrix row before benchmarking.
segment p 0400
mov 0x$TEAK_ADDR(TEAK_XRAM_BASE, 0x0100) r0
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0040) r2
clr a1 always
mov 0x0010u8 a1l
mov [r0++] a0l
mov a0l [r2++]
dec a1 always
br 0x0000$0406 neq
mov 0x$TEAK_YRAM_BASE r4
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0060) r2
clr a1 always
mov 0x0010u8 a1l
mov [r4++] a0l
mov a0l [r2++]
dec a1 always
br 0x0000$0411 neq
ret always

// x[column] = (column + 1) * 0x0080.
segment d TEAK_ADDR(TEAK_XRAM_BASE, 0x0100)
data 0080
data 0100
data 0180
data 0200
data 0280
data 0300
data 0380
data 0400
data 0480
data 0500
data 0580
data 0600
data 0680
data 0700
data 0780
data 0800

// A[row][column] = (row + 1) * 0x0100.
segment d TEAK_YRAM_BASE
// Row 1.
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
data 0100
// Row 2.
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
data 0200
// Row 3.
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
data 0300
// Row 4.
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
data 0400
// Row 5.
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
data 0500
// Row 6.
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
data 0600
// Row 7.
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
data 0700
// Row 8.
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
data 0800
// Row 9.
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
data 0900
// Row 10.
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
data 0A00
// Row 11.
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
data 0B00
// Row 12.
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
data 0C00
// Row 13.
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
data 0D00
// Row 14.
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
data 0E00
// Row 15.
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
data 0F00
// Row 16.
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
data 1000
