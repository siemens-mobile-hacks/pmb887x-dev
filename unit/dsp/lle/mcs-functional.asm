// Bidirectional communication flags and semaphore ownership transfer.
segment p 0100
mov 0x$01FF a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0120 always

// MCU-to-DSP request through CF1, DSP response through CF2.
segment p 0120
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0002 a0
br 0x0000$0120 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$0004 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$0140 always

// DSP-to-MCU request through CF3, MCU response through CF4.
segment p 0140
mov 0x$2468 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0304)]
mov 0x$0008 a0l
mov a0l [0x$TEAK_MCS_CFSET]
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0010 a0
br 0x0000$0148 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0305)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$0010 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$1111 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
br 0x0000$0180 always

// MCU owns semaphore 5; DSP request blocks until MCU releases it.
segment p 0180
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0020 a0
br 0x0000$0180 neq
mov 0x$0020 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov a0l [0x$TEAK_MCS_MCU_SEMS]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0307)]
br 0x0000$01A0 always

segment p 01A0
mov [0x$TEAK_MCS_MCU_SEM] a0
cmp 0x$FFDF a0
br 0x0000$01A0 neq
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030C)]
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)]
mov 0x$0020 a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov 0x$0040 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$01C0 always

// DSP owns semaphore 5; MCU request blocks until DSP releases it.
segment p 01C0
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0080 a0
br 0x0000$01C0 neq
mov 0x$0080 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$0020 a0l
mov a0l [0x$TEAK_MCS_MCU_SEMS]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0309)]
mov 0x$A1A1 a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0308)]
br 0x0000$01E0 always

segment p 01E0
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030B)] a0
cmp 0x$0001 a0
br 0x0000$01E0 neq
mov [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)] a0
inc a0 always
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)]
mov 0x$0020 a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov 0x$0100 a0l
mov a0l [0x$TEAK_MCS_CFSET]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x030A)]
br 0x0000$0200 always

segment p 0200
br 0x0000$0200 always

// Exercise all 16 MCS semaphore latches from the DSP side.
segment p 0220
mov 0x$01FF a0l
mov a0l [0x$TEAK_MCS_CFR]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0306)]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov 0x$A55A a0l
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0300)]
br 0x0000$0240 always

// DSP release as non-owner, then request all MCU-owned semaphores.
segment p 0240
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$0240 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov a0l [0x$TEAK_MCS_MCU_SEMS]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0301)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$0260 always

// Observe alternating ownership after the first MCU release.
segment p 0260
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$0260 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0302)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$0280 always

// Re-request DSP-owned semaphores after the second transfer.
segment p 0280
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$0280 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_MCS_MCU_SEMS]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0303)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$02A0 always

// Release the low half to a waiting MCU.
segment p 02A0
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$02A0 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$00FF a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0304)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$02C0 always

// Release the high half to a waiting MCU.
segment p 02C0
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$02C0 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$FF00 a0l
mov a0l [0x$TEAK_MCS_MCU_SEMR]
mov [0x$TEAK_MCS_MCU_SEM] a0
mov a0l [0x$TEAK_ADDR(TEAK_SHARED_RAM_BASE, 0x0305)]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$02E0 always

// Queue a final DSP request so reset is tested with a pending waiter.
segment p 02E0
mov [0x$TEAK_MCS_CFSTA] a0
cmp 0x$0001 a0
br 0x0000$02E0 neq
mov 0x$0001 a0l
mov a0l [0x$TEAK_MCS_CFR]
mov 0x$FFFF a0l
mov a0l [0x$TEAK_MCS_MCU_SEMS]
mov 0x$0002 a0l
mov a0l [0x$TEAK_MCS_CFSET]
br 0x0000$0300 always

segment p 0300
br 0x0000$0300 always
