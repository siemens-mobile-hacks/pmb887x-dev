@ (c) Dimadze (https://habrahabr.ru/post/226575/)
#include "arm_defs.h" 

.section .text
.arm

.extern context
.extern abt_stack
.extern emulate_ldr_str

.global da_handler
da_handler:         
	@ ��������� �������� R0 - R12 ���������� ���������
	LDR     SP, =context
	STMIA   SP, {R0-R12}

	@ �������������� ���� ��� �������
	LDR     SP, =abt_stack
	ADD     SP, SP, #0x4000
	
	@ ��������� CPSR ���������� ���������
	MRS     R1, SPSR
	LDR     R0, =context
	STR     R1, [R0, #_CPSR * 4]
	
	@ ��������� PC ���������� ���������
	SUB     LR, LR, #0x08
	STR     LR, [R0, #_PC * 4]
	
	@ ���� ����� ���������� ���������� ���������
	AND     R2, R1, #MMSK
	CMP     R2, #MUSR
	@ ���� ����� USR, �� ������ SYS
	MOVEQ   R2, #MSYS
	
	@ �������� ����� ���������� ���������
	MRS     R1, CPSR
	BIC     R1, R1, #MMSK
	ORR     R1, R1, R2
	MSR     CPSR_c, R1
	
	@ ��������� SP � LR ���������� ���������
	STR     SP, [R0, #_SP * 4]
	STR     LR, [R0, #_LR * 4]
	
	@ �������� ��������� ��������, 
	@ ��������� ������� � ����� ABT
	BIC     R1, R1, #MMSK
	ORR     R1, R1, #MABT
	MSR     CPSR_c, R1
	
	BL      emualate_ldr_str
	
	@ ��������� CPSR ���������� ���������
	LDR     R0, =context
	LDR     R1, [R0, #_CPSR * 4]
	MSR     SPSR_cxsf, R1
	
	@ ���� ����� ���������� ���������� ���������
	AND     R2, R1, #MMSK
	CMP     R2, #MUSR
	@ ���� ����� USR, �� ������ SYS
	MOVEQ   R2, #MSYS
	
	@ �������� ����� ���������� ���������
	MRS     R1, CPSR
	BIC     R1, R1, #MMSK
	ORR     R1, R1, R2
	MSR     CPSR_c, R1
	
	@ ��������� SP � LR ���������� ���������
	LDR     SP, [R0, #_SP * 4]
	LDR     LR, [R0, #_LR * 4]
	
	@ �������� ����� ����� ABT
	BIC     R1, R1, #MMSK
	ORR     R1, R1, #MABT
	MSR     CPSR_c, R1
	
	@ ��������� PC
	LDR     SP, =context
	LDR     LR, [SP, #_PC * 4]
	
	@ ��������� �������� R0-R12
	LDMIA   SP, {R0-R12}
	
	@ ��������� � ��������� ����������
	@ ��������������� ��������� ������
	MOVS    PC, LR
