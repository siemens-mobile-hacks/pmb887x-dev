// Delayed return with the arithmetic delay slots used by the EL71 DSP firmware.
segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
clr a0 always
mov 0x$1234 r0
addh r0 a0
call 0x0000$0200 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov a0h a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov st0 r0
mov r0 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0400 always

segment p 0180
br 0x0000$0180 always

segment p 0200
retd
sqr a0h a0
pacr a0 always

// Delay-slot contents for retd and retid.

// 0: retd with two one-cycle slots.
segment p 0400
dint
mov 0x$0001 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$0419 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0410)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0411)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0412)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0413)]
br 0x0000$0440 always
retd
inc a1 always
inc a1 always
inc a0 always
inc a0 always
inc a0 always
ret always

// 1: retd with a long immediate add slot.
segment p 0440
dint
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$0459 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0414)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0415)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0416)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0417)]
br 0x0000$0480 always
retd
add 0x$0005 a1
inc a0 always
inc a0 always
inc a0 always
ret always

// 2: retd with a long immediate mov slot.
segment p 0480
dint
mov 0x$0003 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$0499 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0418)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0419)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041A)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041B)]
br 0x0000$04C0 always
retd
mov 0x$1234 r1
inc a0 always
inc a0 always
inc a0 always
ret always

// 3: retd with a norm slot.
segment p 04C0
dint
mov 0x$0004 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$04D9 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041C)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041D)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041E)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x041F)]
br 0x0000$0500 always
retd
norm a1 [r1]
inc a0 always
inc a0 always
inc a0 always
ret always

// 4: retid with two one-cycle slots.
segment p 0500
dint
mov 0x$0005 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$0519 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0420)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0421)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0422)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0423)]
br 0x0000$0540 always
retid
inc a1 always
inc a1 always
inc a0 always
inc a0 always
inc a0 always
ret always

// 5: retid with a long immediate add slot.
segment p 0540
dint
mov 0x$0006 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0401)]
clr a1 always
mov 0x$0001 a1l
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0480) r1
clr a0 always
call 0x0000$0559 always
mov a1l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0424)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0425)]
mov r1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0426)]
mov sp a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0427)]
br 0x0000$0F00 always
retid
add 0x$0005 a1
inc a0 always
inc a0 always
inc a0 always
ret always

segment p 0F00
dint
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0400)]
br 0x0000$0F05 always

segment p 0F05
br 0x0000$0F05 always
