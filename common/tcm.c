#include "tcm.h"

void test_tcm(int type) {
	const int tcm_sizes[16] = { 0, -1, -1, 4, 8, 16, 32, 64, 128,
				    256, 512, 1024, -1, -1, -1, -1 };
	
	pmb8876_serial_print(type ? "[ITCM]\n" : "[DTCM]\n");
	
	unsigned int tcm_status = read_cpuid(CPUID_TCM);
	if (tcm_status & TCMTR_FORMAT_MASK)
		pmb8876_serial_print("Unsupported TCMTR\n");
	
	unsigned int dtcm_banks = (tcm_status >> 16) & 0x03;
	unsigned int itcm_banks = (tcm_status & 0x03);
	
	pmb8876_serial_print("dtcm_banks = ");
	hexdump((const char *) &dtcm_banks, 4);
	pmb8876_serial_print("\n");
	
	pmb8876_serial_print("itcm_banks = ");
	hexdump((const char *) &itcm_banks, 4);
	pmb8876_serial_print("\n");
	
	// DTCM
	unsigned int tcm_region = type ? __mrc(15, 0, 9, 1, 1) : __mrc(15, 0, 9, 1, 0);
	pmb8876_serial_print("tcm_region = ");
	hexdump((const char *) &tcm_region, 4);
	pmb8876_serial_print("\n");
	
	int tcm_size = tcm_sizes[(tcm_region >> 2) & 0x0f];
	pmb8876_serial_print("tcm_size = ");
	hexdump((const char *) &tcm_size, 4);
	pmb8876_serial_print("\n");
	
	if (tcm_region & 1)
		pmb8876_serial_print("...enabled!\n");
	
	pmb8876_serial_print("\n\n\n");
}
