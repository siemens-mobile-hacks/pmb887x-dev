#ifndef __SNIFFER_H__
#define __SNIFFER_H__

#include "arm_defs.h" 

extern  void da_handler();

#define IO_ADDRESS                    0xF0000000
#define IO_ADDRESS_MIRROR             0xE0000000
#define IO_ADDRESS_DIF                0x10000000

#define VECTOR_DATAABORT_JUMPER       0xE59FF018
#define VECTOR_DATAABORT_JUMPER_OFS   0x10
#define VECTOR_DATAABORT_HANDLER_OFS  0x30

#define MMU_ATTR                      0xC12
#define MMU_GRID(a)                   * ( (unsigned int   *) ( (MMU_TABLE + (((a) >> 20) & 0xFFF) * 4) ) )  
#define MMU_GRID_MIRROR(a)            MMU_GRID(a - IO_ADDRESS_DIF)
#define MMU_GRID_SETATTR(a)           ((a & 0xFFF00000) | MMU_ATTR)

#pragma pack(push, 1)
typedef struct 
{
  unsigned int r0;
  unsigned int r1;
  unsigned int r2;
  unsigned int r3;
  unsigned int r4;
  unsigned int r5;
  unsigned int r6;
  unsigned int r7;
  unsigned int r8;
  unsigned int r9;
  unsigned int r10;
  unsigned int r11;
  unsigned int r12;
  unsigned int sp;
  unsigned int lr;
  unsigned int pc;
  unsigned int cpsr;
}REGISTERS;

typedef union 
{
	struct {
		REGISTERS    s;
	};
	unsigned int a[MAX_REGS];
}CONTEXT;
#pragma pack(pop)

void  io_sniffer_init(void (*sniff_prc)(unsigned int address, unsigned int value, unsigned int pc, char is_ldr));
void  io_sniffer_deinit();
int   io_sniffer_add(unsigned int io_address);
int   io_sniffer_remove(unsigned int io_address);

#endif // __SNIFFER_H__

