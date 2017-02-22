@ (c) Dimadze (https://habrahabr.ru/post/226575/)
#include "arm_defs.h" 

.section .text
.arm

.extern context
.extern abt_stack
.extern emulate_ldr_str

.global da_handler
da_handler:         
	@ Сохраняем контекст R0 - R12 прерванной программы
	LDR     SP, =context
	STMIA   SP, {R0-R12}

	@ Подготавливаем стек для хендера
	LDR     SP, =abt_stack
	ADD     SP, SP, #0x4000
	
	@ Сохраняем CPSR прерванной программы
	MRS     R1, SPSR
	LDR     R0, =context
	STR     R1, [R0, #_CPSR * 4]
	
	@ Сохраняем PC прерванной программы
	SUB     LR, LR, #0x08
	STR     LR, [R0, #_PC * 4]
	
	@ Берём режим процессора прерванной программы
	AND     R2, R1, #MMSK
	CMP     R2, #MUSR
	@ Если режим USR, то возьмём SYS
	MOVEQ   R2, #MSYS
	
	@ Включаем режим прерванной программы
	MRS     R1, CPSR
	BIC     R1, R1, #MMSK
	ORR     R1, R1, R2
	MSR     CPSR_c, R1
	
	@ Сохраняем SP и LR прерванной программы
	STR     SP, [R0, #_SP * 4]
	STR     LR, [R0, #_LR * 4]
	
	@ Контекст полностью сохранён, 
	@ переводим обратно в режим ABT
	BIC     R1, R1, #MMSK
	ORR     R1, R1, #MABT
	MSR     CPSR_c, R1
	
	BL      emualate_ldr_str
	
	@ Загружаем CPSR прерванной программы
	LDR     R0, =context
	LDR     R1, [R0, #_CPSR * 4]
	MSR     SPSR_cxsf, R1
	
	@ Берём режим процессора прерванной программы
	AND     R2, R1, #MMSK
	CMP     R2, #MUSR
	@ Если режим USR, то возьмём SYS
	MOVEQ   R2, #MSYS
	
	@ Включаем режим прерванной программы
	MRS     R1, CPSR
	BIC     R1, R1, #MMSK
	ORR     R1, R1, R2
	MSR     CPSR_c, R1
	
	@ Загружаем SP и LR прерванной программы
	LDR     SP, [R0, #_SP * 4]
	LDR     LR, [R0, #_LR * 4]
	
	@ Включаем снова режим ABT
	BIC     R1, R1, #MMSK
	ORR     R1, R1, #MABT
	MSR     CPSR_c, R1
	
	@ Загружаем PC
	LDR     SP, =context
	LDR     LR, [SP, #_PC * 4]
	
	@ Загружаем регистры R0-R12
	LDMIA   SP, {R0-R12}
	
	@ Переходим к следующей инструкции
	@ Восстанавливаем состояние флагов
	MOVS    PC, LR
