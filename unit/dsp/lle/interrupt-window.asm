// Interrupt acceptance inside straight-line windows opened and closed by IE/IMx writes.
segment p 0006
br 0x0000$0900 always

segment p 000E
br 0x0000$0980 always

segment p 0016
br 0x0000$0940 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
mov 0x0000 icr
rst 0x$0004 st0
rst 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINTB0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
br 0x0000$0200 always

// 0: eint opens a four-nop window.
segment p 0200
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0600 always

segment p 0600
eint
nop
nop
nop
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0230 always

// 1: set st0 IE opens a four-nop window.
segment p 0230
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0620 always

segment p 0620
set 0x$0002 st0
nop
nop
nop
nop
rst 0x$0002 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0260 always

// 2: mov long immediate st0 opens a four-nop window.
segment p 0260
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0640 always

segment p 0640
mov 0x$0006 st0
nop
nop
nop
nop
mov 0x$0004 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0290 always

// 3: pop st0 opens a four-nop window.
segment p 0290
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
mov 0x$0006 r1
push r1
br 0x0000$0660 always

segment p 0660
pop st0
nop
nop
nop
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0416)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0417)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$02C0 always

// 4: set st0 IM0 opens a four-nop window.
segment p 02C0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
rst 0x$0004 st0
eint
br 0x0000$0680 always

segment p 0680
set 0x$0004 st0
nop
nop
nop
nop
rst 0x$0004 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$02F0 always

// 5: set st2 IM2 opens a four-nop window.
segment p 02F0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
set 0x$0040 st2
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
rst 0x$0040 st2
eint
br 0x0000$06A0 always

segment p 06A0
set 0x$0040 st2
nop
nop
nop
nop
rst 0x$0040 st2
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
br 0x0000$0320 always

// 6: source raised inside an enabled window.
segment p 0320
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
eint
br 0x0000$06C0 always

segment p 06C0
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_SINTA0]
nop
nop
nop
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041D)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0350 always

// 7: eint directly followed by dint.
segment p 0350
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$06E0 always

segment p 06E0
eint
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041E)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041F)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0380 always

// 8: eint nop dint.
segment p 0380
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0700 always

segment p 0700
eint
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$03B0 always

// 9: set st0 IE directly followed by rst.
segment p 03B0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0720 always

segment p 0720
set 0x$0002 st0
rst 0x$0002 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$03E0 always

// 10: set st0 IE nop rst.
segment p 03E0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0740 always

segment p 0740
set 0x$0002 st0
nop
rst 0x$0002 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0425)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0410 always

// 11: mov long immediate st0 directly followed by mov.
segment p 0410
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
br 0x0000$0760 always

segment p 0760
mov 0x$0006 st0
mov 0x$0004 st0
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0426)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0427)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0440 always

// 12: pop st0 directly followed by dint.
segment p 0440
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
mov 0x$0006 r1
push r1
br 0x0000$0780 always

segment p 0780
pop st0
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0428)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0429)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0470 always

// 13: mov long immediate st2 opens an IM2 window.
segment p 0470
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
set 0x$0040 st2
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
rst 0x$0040 st2
eint
br 0x0000$07A0 always

segment p 07A0
mov 0x$0040 st2
nop
nop
nop
nop
rst 0x$0040 st2
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042A)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042B)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
br 0x0000$04A0 always

// 14: pop st2 opens an IM2 window.
segment p 04A0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
set 0x$0040 st2
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
rst 0x$0040 st2
eint
mov 0x$0040 r1
push r1
br 0x0000$07C0 always

segment p 07C0
pop st2
nop
nop
nop
nop
rst 0x$0040 st2
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042C)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042D)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
br 0x0000$04D0 always

// 15: mov register st0 opens a window.
segment p 04D0
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_SINTA0]
rep 0x00ffu8
nop
mov 0x$0006 r1
br 0x0000$07E0 always

segment p 07E0
mov r1 st0
nop
nop
nop
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042E)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x042F)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
br 0x0000$0500 always

// 16: mov register st2 opens an IM2 window.
segment p 0500
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$0004 st0
set 0x$0040 st2
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
rst 0x$0040 st2
eint
mov 0x$0040 r1
br 0x0000$0800 always

segment p 0800
mov r1 st2
nop
nop
nop
nop
rst 0x$0040 st2
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0430)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0431)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINT2]
br 0x0000$0530 always

// 17: INT0 INT1 and INT2 pending together.
segment p 0530
dint
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
set 0x$000C st0
set 0x$0040 st2
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
mov a0l [0x$TEAK_INT_SINTA0]
mov a0l [0x$TEAK_INT_SINT1]
mov a0l [0x$TEAK_INT_SINT2]
rep 0x00ffu8
nop
br 0x0000$0820 always

segment p 0820
eint
nop
nop
nop
nop
nop
nop
nop
nop
dint
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0432)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0433)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
mov a0l [0x$TEAK_INT_RINT1]
mov a0l [0x$TEAK_INT_RINT2]
clr a0 always
mov a0l [0x$TEAK_INT_EINTA0]
mov a0l [0x$TEAK_INT_EINT1]
mov a0l [0x$TEAK_INT_EINT2]
br 0x0000$0880 always

segment p 0880
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0884 always

segment p 0884
br 0x0000$0884 always

segment p 0900
pop r0
push r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0405)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINTA0]
reti true

segment p 0980
pop r0
push r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0406)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT1]
reti true

segment p 0940
pop r0
push r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0403)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0402)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0407)]
mov 0x$0001 a0l
mov a0l [0x$TEAK_INT_RINT2]
reti true
