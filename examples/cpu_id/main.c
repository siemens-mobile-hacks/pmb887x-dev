#include <pmb887x.h>

int main(void) {
	uint8_t cpu_id[16];

	cpu_id[0] = SCU_ID0 & 0xFF;
	cpu_id[1] = (SCU_ID0 >> 8) & 0xFF;
	cpu_id[2] = (SCU_ID0 >> 16) & 0xFF;

	cpu_id[3] = SCU_ID1 & 0xFF;
	cpu_id[4] = (SCU_ID1 >> 8) & 0xFF;
	cpu_id[5] = (SCU_ID1 >> 16) & 0xFF;

	if ((SCU_CHIPID & 0xFF) == 0) {
		cpu_id[6] = SCU_BOOT_CFG & 0xFF;
		cpu_id[7] = (SCU_BOOT_CFG >> 8) & 0xFF;
		cpu_id[8] = (SCU_BOOT_CFG >> 16) & 0xFF;
	} else {
		cpu_id[6] = (SCU_ID1 >> 24) & 0xF;
		cpu_id[7] = 0;
		cpu_id[8] = 0;
	}

	cpu_id[9] = 0;
	cpu_id[10] = 0;
	cpu_id[11] = 0;
	cpu_id[12] = 0;
	cpu_id[13] = 0;
	cpu_id[14] = 0;
	cpu_id[15] = 0;

	printf("APOXI CPU ID: ");
	for (int i = 0; i < 16; i++)
		printf("%02X", cpu_id[i]);
	printf("\n");

	printf("SCU_ID0=%08X\n", SCU_ID0);
	printf("SCU_ID1=%08X\n", SCU_ID1);
	printf("SCU_BOOT_CFG=%08X\n", SCU_BOOT_CFG);
	printf("SCU_MANID=%08X\n", SCU_MANID);
	printf("SCU_CHIPID=%08X\n", SCU_CHIPID);

	return 0;
}
