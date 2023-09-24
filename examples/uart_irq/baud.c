#include <stdio.h>
#include <stdint.h>
#include <sys/param.h>

#define ASC_MAX_FDV		511
#define ASC_MAX_BG		8191

static const uint32_t fasc = 26000000;

void asc_calc_fdv_bg(uint32_t baudrate, uint32_t *bg, uint32_t *fdv, uint32_t *real_baudrate) {
	uint32_t max_baudrate = fasc / 16;
	if (baudrate >= max_baudrate) {
		*bg = 0;
		*fdv = 0;
	} else if ((max_baudrate % baudrate) == 0) {
		*bg = (max_baudrate / baudrate) - 1;
		*fdv = 0;
		
		int div = 256;
		while (*bg > ASC_MAX_BG) {
			*fdv = div;
			*bg >>= 1;
			div >>= 1;
		}
	} else {
		uint32_t good_baud = max_baudrate / (max_baudrate / baudrate);
		*bg = (max_baudrate / good_baud) - 1;
		*fdv = baudrate * 512 / good_baud;
		
		while (*bg > ASC_MAX_BG) {
			*bg >>= 1;
			*fdv >>= 1;
		}
	}
	
	*real_baudrate = *fdv ?
		(max_baudrate / (*bg + 1)) * *fdv / 512 :
		(max_baudrate / (*bg + 1));
}

int main() {
	uint32_t baudrates[] = {
		57600, 115200, 230400, 460800, 614400, 921600, 1228800, 1600000, 1500000,
		26,
		21,
	};
	
	for (int i = 0; i < (sizeof(baudrates) / sizeof(baudrates[0])); i++) {
		uint32_t baudrate = baudrates[i];
		uint32_t BG = 0;
		uint32_t FDV = 0;
		uint32_t real_baud = 0;
		
		asc_calc_fdv_bg(baudrate, &BG, &FDV, &real_baud);
		
		float error = (1 - ((float) MIN(baudrate, real_baud) / (float) MAX(baudrate, real_baud))) * 100;
		printf("expected = %d, real_baud=%d [FDV=%d, BG=%d, error=%.02f%%]\n", baudrate, real_baud, FDV, BG, error);
	}
	
	return 0;
	/*
	for (int i = 0; i <= ASC_MAX_FDV; i++) {
		FDV = i;
		
		uint32_t real_baud = FDV ?
			(fasc / (BG + 1) / 16) * FDV / 512 :
			(fasc / (BG + 1) / 16);
		
		float error = (1 - ((float) MIN(baudrate, real_baud) / (float) MAX(baudrate, real_baud))) * 100;
		if (error < 0.2) {
			printf("real_baud=%d [FDV=%d, BG=%d, error=%.02f%%]\n", real_baud, FDV, BG, error);
		}
	}
	
	
	return 0;
	
	for (int j = 0; j <= ASC_MAX_BG; j++) {
		for (int i = 0; i <= ASC_MAX_FDV; i++) {
			BG = j;
			FDV = i;
			
			uint32_t real_baud = FDV ?
				(fasc / (BG + 1) / 16) * 512 / FDV :
				(fasc / (BG + 1) / 16);
			
			float error = (1 - ((float) MIN(baudrate, real_baud) / (float) MAX(baudrate, real_baud))) * 100;
			if (error < 1) {
				printf("real_baud=%d [FDV=%d, BG=%d, error=%.02f%%]\n", real_baud, FDV, BG, error);
			}
		}
	}
	*/
	
	return 0;
}
