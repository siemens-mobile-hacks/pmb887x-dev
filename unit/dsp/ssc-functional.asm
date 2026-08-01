// DSP-side SSC internal-loopback behavior, FIFO operation, and interrupt delivery.
segment p 0006
br 0x0000$0900 always

segment p 0100
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
dint
data 4F8E // mov #0x0E,icr: enable context switching for INT0/INT1/INT2.
set 0x$000C st0
set 0x$0040 st2
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0500)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0510) r5

// Width 2, all HB/PH/PO combinations.
mov 0x$6081 a0l
mov 0x$A55A a1l
call 0x0000$0800 always
mov 0x$6091 a0l
mov 0x$3CC3 a1l
call 0x0000$0800 always
mov 0x$60A1 a0l
mov 0x$9695 a1l
call 0x0000$0800 always
mov 0x$60B1 a0l
mov 0x$5AA5 a1l
call 0x0000$0800 always
mov 0x$60C1 a0l
mov 0x$C33C a1l
call 0x0000$0800 always
mov 0x$60D1 a0l
mov 0x$6996 a1l
call 0x0000$0800 always
mov 0x$60E1 a0l
mov 0x$0F81 a1l
call 0x0000$0800 always
mov 0x$60F1 a0l
mov 0x$81F0 a1l
call 0x0000$0800 always

// Width 4, all HB/PH/PO combinations.
mov 0x$6083 a0l
mov 0x$A55A a1l
call 0x0000$0800 always
mov 0x$6093 a0l
mov 0x$3CC3 a1l
call 0x0000$0800 always
mov 0x$60A3 a0l
mov 0x$9695 a1l
call 0x0000$0800 always
mov 0x$60B3 a0l
mov 0x$5AA5 a1l
call 0x0000$0800 always
mov 0x$60C3 a0l
mov 0x$C33C a1l
call 0x0000$0800 always
mov 0x$60D3 a0l
mov 0x$6996 a1l
call 0x0000$0800 always
mov 0x$60E3 a0l
mov 0x$0F81 a1l
call 0x0000$0800 always
mov 0x$60F3 a0l
mov 0x$81F0 a1l
call 0x0000$0800 always

// Width 8, all HB/PH/PO combinations.
mov 0x$6087 a0l
mov 0x$A55A a1l
call 0x0000$0800 always
mov 0x$6097 a0l
mov 0x$3CC3 a1l
call 0x0000$0800 always
mov 0x$60A7 a0l
mov 0x$9695 a1l
call 0x0000$0800 always
mov 0x$60B7 a0l
mov 0x$5AA5 a1l
call 0x0000$0800 always
mov 0x$60C7 a0l
mov 0x$C33C a1l
call 0x0000$0800 always
mov 0x$60D7 a0l
mov 0x$6996 a1l
call 0x0000$0800 always
mov 0x$60E7 a0l
mov 0x$0F81 a1l
call 0x0000$0800 always
mov 0x$60F7 a0l
mov 0x$81F0 a1l
call 0x0000$0800 always

// Width 12, all HB/PH/PO combinations.
mov 0x$608B a0l
mov 0x$A55A a1l
call 0x0000$0800 always
mov 0x$609B a0l
mov 0x$3CC3 a1l
call 0x0000$0800 always
mov 0x$60AB a0l
mov 0x$9695 a1l
call 0x0000$0800 always
mov 0x$60BB a0l
mov 0x$5AA5 a1l
call 0x0000$0800 always
mov 0x$60CB a0l
mov 0x$C33C a1l
call 0x0000$0800 always
mov 0x$60DB a0l
mov 0x$6996 a1l
call 0x0000$0800 always
mov 0x$60EB a0l
mov 0x$0F81 a1l
call 0x0000$0800 always
mov 0x$60FB a0l
mov 0x$81F0 a1l
call 0x0000$0800 always

// Width 16, all HB/PH/PO combinations.
mov 0x$608F a0l
mov 0x$A55A a1l
call 0x0000$0800 always
mov 0x$609F a0l
mov 0x$3CC3 a1l
call 0x0000$0800 always
mov 0x$60AF a0l
mov 0x$9695 a1l
call 0x0000$0800 always
mov 0x$60BF a0l
mov 0x$5AA5 a1l
call 0x0000$0800 always
mov 0x$60CF a0l
mov 0x$C33C a1l
call 0x0000$0800 always
mov 0x$60DF a0l
mov 0x$6996 a1l
call 0x0000$0800 always
mov 0x$60EF a0l
mov 0x$0F81 a1l
call 0x0000$0800 always
mov 0x$60FF a0l
mov 0x$81F0 a1l
call 0x0000$0800 always
br 0x0000$0400 always

segment p 0400
// Deliberately slow transfer: capture BSY and bit-count progress.
mov 0x$0002 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0100 a0l
mov a0l [0x$TEAK_SSC_BR]
clr a0 always
mov a0l [0x$TEAK_SSC_FDV]
mov a0l [0x$TEAK_SSC_RXFCON]
mov a0l [0x$TEAK_SSC_TXFCON]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E08F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$5AA5 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov [0x$TEAK_SSC_CON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0540)]
mov 0x$0400 r0
rep r0
nop
mov [0x$TEAK_SSC_CON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0541)]
mov 0x$4000 r0
rep r0
nop
mov [0x$TEAK_SSC_CON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0542)]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0543)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]

// Deliver naturally generated RX/TX requests through the real INT0 vector.
mov 0x$0003 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057F)]
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0003 a0l
mov a0l [0x$TEAK_SSC_BR]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E08F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$96A5 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$0800 r0
rep r0
nop
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0550)]
mov 0x$0180 a0l
mov a0l [0x$TEAK_INT_EINTB0]
eint
mov 0x$0400 r0
rep r0
nop
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0551)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057F)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0552)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0553)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
br 0x0000$0600 always

segment p 0600
// Fill a 32-word FIFO, drain it, then transfer another 9 words without reset.
mov 0x$0004 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$000F a0l
mov a0l [0x$TEAK_SSC_BR]
clr a0 always
mov a0l [0x$TEAK_SSC_FDV]
mov 0x$1003 a0l
mov a0l [0x$TEAK_SSC_RXFCON]
mov 0x$0403 a0l
mov a0l [0x$TEAK_SSC_TXFCON]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E08F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$1001 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$2112 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$3223 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$4334 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$5445 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$6556 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$7667 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$8778 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$9889 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A99A a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$BAAB a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$CBBC a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$DCCD a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$EDDE a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$FEEF a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$0FF0 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$1357 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$2468 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$369C a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$47AD a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$58BE a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$69CF a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$7AD0 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$8BE1 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$9CF2 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$AD03 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$BE14 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$CF25 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$D036 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$E147 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$F258 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$0369 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0560)]
mov 0x$FFFF r0
rep r0
nop
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0561)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0562)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0600) r5
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0563)]
mov 0x$0180 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov 0x$147A a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$258B a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$369D a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$47AE a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$58BF a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$69C0 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$7AD1 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$8BE2 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$9CF3 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$4000 r0
rep r0
nop
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0564)]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0565)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
br 0x0000$0A00 always

segment p 0A00
// FIFO flushes and a clean transfer afterwards.
mov 0x$0005 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_SSC_BR]
mov 0x$0103 a0l
mov a0l [0x$TEAK_SSC_RXFCON]
mov a0l [0x$TEAK_SSC_TXFCON]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E08F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$1111 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$2222 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$3333 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$4444 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$5555 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$6666 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$7777 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$8888 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0566)]
mov 0x$0103 a0l
mov a0l [0x$TEAK_SSC_TXFCON]
mov [0x$TEAK_SSC_TXFCON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0567)]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0568)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0003 a0l
mov a0l [0x$TEAK_SSC_BR]
mov 0x$0103 a0l
mov a0l [0x$TEAK_SSC_RXFCON]
mov a0l [0x$TEAK_SSC_TXFCON]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E08F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$A001 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A002 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A003 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A004 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A005 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A006 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A007 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$A008 a0l
mov a0l [0x$TEAK_SSC_TXB]
mov 0x$2000 r0
rep r0
nop
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0569)]
mov 0x$0103 a0l
mov a0l [0x$TEAK_SSC_RXFCON]
mov [0x$TEAK_SSC_RXFCON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x056A)]
mov [0x$TEAK_SSC_FSTAT] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x056B)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x056C) r5
mov 0x$608F a0l
mov 0x$5A3C a1l
call 0x0000$0800 always

// Empty RX FIFO access with REN enabled must raise RE and SSC1ERR.
mov 0x$0006 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0502)]
clr a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)]
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057F)]
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0101 a0l
mov a0l [0x$TEAK_SSC_RXFCON]
mov 0x$628F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$E28F a0l
mov a0l [0x$TEAK_SSC_CON]
mov [0x$TEAK_SSC_RXB] a0
mov 0x$0400 r0
rep r0
nop
mov [0x$TEAK_SSC_CON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0570)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0571)]
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_EINTB0]
eint
mov 0x$0400 r0
rep r0
nop
dint
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0572)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057F)] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0573)]
// Hardware follows the detailed WHBCON description despite the final table saying Not Used.
mov 0x$0200 a0l
mov a0l [0x$TEAK_SSC_WHBCON]
mov 0x$0400 r0
rep r0
nop
mov [0x$TEAK_SSC_CON] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0576)]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0577)]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$608F a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0200 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0574)]
mov 0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0575) r5
mov 0x$608F a0l
mov 0x$C35A a1l
call 0x0000$0800 always

clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0501)]
br 0x0000$0AF0 always

// One-word loopback helper. A0L=config, A1L=word, R5=output pointer.
segment p 0800
mov a0l r4
mov 0x$2000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0003 a0l
mov a0l [0x$TEAK_SSC_BR]
clr a0 always
mov a0l [0x$TEAK_SSC_FDV]
mov a0l [0x$TEAK_SSC_RXFCON]
mov a0l [0x$TEAK_SSC_TXFCON]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
mov r4 a0l
mov a0l [0x$TEAK_SSC_CON]
set 0x$8000 a0l
mov a0l [0x$TEAK_SSC_CON]
mov a1l [0x$TEAK_SSC_TXB]
mov 0x$0800 r0
rep r0
nop
mov [0x$TEAK_SSC_RXB] a0
mov a0l [r5++]
clr a0 always
mov a0l [0x$TEAK_SSC_CON]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
ret always

// INT0 handler records all SSC sources, masks them, and acknowledges them.
segment p 0900
mov [0x$TEAK_INT_FINTB0] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057F)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x057E)]
clr a0 always
mov a0l [0x$TEAK_INT_EINTB0]
mov 0x$0380 a0l
mov a0l [0x$TEAK_INT_RINTB0]
data 45D0 // reti always,context.
