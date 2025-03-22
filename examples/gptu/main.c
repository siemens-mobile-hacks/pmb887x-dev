#include <pmb887x.h>
/*
WRITE[4] F4900000: 00000100 (GPTU0_CLC): RMC(0x01) (PC: 004010B8, LR: 00400584)
WRITE[4] F4900010: 0003003C (GPTU0_T01IRS): T0AINS(BYPASS) | T0BINS(CONCAT) | T0CINS(CONCAT) | T0DINS(BYPASS) | T1AINS(BYPASS) | T1BINS(BYPASS) | T1CINS(BYPASS) | T1DINS(BYPASS) | T0AREL | T0BREL (PC: 004010B8, LR: 00400584)
 READ[4] F4900014: 00000000 (GPTU0_T01OTS): SOUT00(A) | SOUT01(A) | STRG00(A) | STRG01(A) | SSR00(A) | SSR01(A) | SOUT10(A) | SOUT11(A) | STRG10(A) | STRG11(A) | SSR10(A) | SSR11(A) (PC: 004010B8, LR: 00400584)
WRITE[4] F4900014: 00000200 (GPTU0_T01OTS): SOUT00(A) | SOUT01(A) | STRG00(A) | STRG01(A) | SSR00(C) | SSR01(A) | SOUT10(A) | SOUT11(A) | STRG10(A) | STRG11(A) | SSR10(A) | SSR11(A) (PC: 004010B8, LR: 00400584)
 READ[4] F49000DC: 00000000 (GPTU0_SRSEL): SSR7(START_A) | SSR6(START_A) | SSR5(START_A) | SSR4(START_A) | SSR3(START_A) | SSR2(START_A) | SSR1(START_A) | SSR0(START_A) (PC: 004010B8, LR: 00400584)
WRITE[4] F49000DC: 0000000C (GPTU0_SRSEL): SSR7(SR00) | SSR6(START_A) | SSR5(START_A) | SSR4(START_A) | SSR3(START_A) | SSR2(START_A) | SSR1(START_A) | SSR0(START_A) (PC: 004010B8, LR: 00400584)
WRITE[4] F4900038: 00D06480 (GPTU0_T0CBA): T0A(0x80) | T0B(0x64) | T0C(0xD0) (PC: 004010B8, LR: 00400584)
WRITE[4] F4900060: 00000007 (GPTU0_T012RUN): T0ARUN | T0BRUN | T0CRUN (PC: 004010B8, LR: 00400584)
 READ[4] F49000FC: 00000000 (GPTU0_SRC0) (PC: 0040118C, LR: 00400778)
*/

static void test1(void) {
	GPTU_CLC(GPTU0) = (1 << MOD_CLC_RMC_SHIFT);
	
	// C -> B -> A
	// Concat B + C
	GPTU_T01IRS(GPTU0) =
		GPTU_T01IRS_T0BINS_CONCAT |
		GPTU_T01IRS_T0CINS_CONCAT |
		GPTU_T01IRS_T0AREL |
		GPTU_T01IRS_T0BREL;
	
	// Trigger on T0C overflow
	GPTU_T01OTS(GPTU0) = GPTU_T01OTS_SSR00_C;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR0_SR00;
	
	GPTU_T0CBA(GPTU0) = 0;
	GPTU_T0RCBA(GPTU0) = 0;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T0ARUN | GPTU_T012RUN_T0BRUN | GPTU_T012RUN_T0CRUN;
	
	stopwatch_t start = stopwatch_get();
	
	uint32_t max_cnt = 0;
	while (!(GPTU_SRC(GPTU0, 0) & MOD_SRC_SRR)) {
		max_cnt = MAX(max_cnt, GPTU_T0CBA(GPTU0));
		//printf("%08X\n", GPTU_T0CBA(GPTU0));
	}
	uint32_t elapsed = stopwatch_elapsed_ms(start);
	
	printf("max_cnt=%08X\n", max_cnt);
	printf("elapsed: %d ms\n", elapsed);
}

static void test2(void) {
	uint8_t ram[0xFFF] = {0};
	int ram_idx = 0;
	uint32_t old_cnt = 0x55;
	
	GPTU_CLC(GPTU0) = (0xFF << MOD_CLC_RMC_SHIFT);
	
	GPTU_T01IRS(GPTU0) = 0;
	GPTU_T01OTS(GPTU0) = GPTU_T01OTS_SSR00_A;
	GPTU_SRSEL(GPTU0) = GPTU_SRSEL_SSR0_SR00;
	GPTU_T0CBA(GPTU0) = 0;
	GPTU_T0RCBA(GPTU0) = 66;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T0ARUN;
	
	stopwatch_t start = stopwatch_get();
	
	while (!(GPTU_SRC(GPTU0, 0) & MOD_SRC_SRR)) {
		uint32_t new_cnt = GPTU_T0CBA(GPTU0);
		
		if (old_cnt != new_cnt)
			ram[ram_idx++] = new_cnt & 0xFF;
		old_cnt = new_cnt;
	}
	uint32_t elapsed = stopwatch_elapsed(start);
	
	printf("ram_idx=%d\n", ram_idx);
	printf("elapsed: %d ns\n", elapsed);
	
	for (int i = 0; i < ram_idx; i++)
		printf("%d\n", ram[i]);
	printf("\n");
}

static void test3(void) {
	GPTU_CLC(GPTU0) = (1 << MOD_CLC_RMC_SHIFT);
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ACLRR | GPTU_T012RUN_T2BCLRR;
	GPTU_T01IRS(GPTU0) =
		GPTU_T01IRS_T0BINS_CONCAT |
		GPTU_T01IRS_T0CINS_CONCAT |
		GPTU_T01IRS_T0DINS_CONCAT |
		GPTU_T01IRS_T1BINS_CONCAT |
		GPTU_T01IRS_T1CINS_CONCAT |
		GPTU_T01IRS_T1DINS_CONCAT |
		GPTU_T01IRS_T0AREL |
		GPTU_T01IRS_T0BREL |
		GPTU_T01IRS_T0CREL |
		GPTU_T01IRS_T1AREL |
		GPTU_T01IRS_T1BREL |
		GPTU_T01IRS_T1CREL |
		GPTU_T01IRS_T01IN0_POS_IN0 |
		GPTU_T01IRS_T01IN1_POS_IN1;
	GPTU_T01OTS(GPTU0) =
		GPTU_T01OTS_SOUT00_D |
		GPTU_T01OTS_SOUT01_D |
		GPTU_T01OTS_STRG00_D |
		GPTU_T01OTS_STRG01_D |
		GPTU_T01OTS_SSR00_D |
		GPTU_T01OTS_SSR01_D |
		GPTU_T01OTS_SOUT10_D |
		GPTU_T01OTS_SOUT11_D |
		GPTU_T01OTS_STRG10_D |
		GPTU_T01OTS_STRG11_D |
		GPTU_T01OTS_SSR10_D |
		GPTU_T01OTS_SSR11_D;
	GPTU_T2CON(GPTU0) = GPTU_T2CON_T2ACDIR_COUNT_DOWN | GPTU_T2CON_T2ACOS;
	GPTU_T2RCCON(GPTU0) = 0;
	GPTU_T2ES(GPTU0) = 0;
	GPTU_T2AIS(GPTU0) = 0;
	GPTU_T2BIS(GPTU0) = 0;
	GPTU_OSEL(GPTU0) =
		GPTU_OSEL_SO0_OUT00 |
		GPTU_OSEL_SO1_OUT10 |
		GPTU_OSEL_SO2_OUV_T2B |
		GPTU_OSEL_SO3_UNK1 |
		GPTU_OSEL_SO4_UNK1 |
		GPTU_OSEL_SO5_UNK1 |
		GPTU_OSEL_SO6_UNK1 |
		GPTU_OSEL_SO7_UNK1;
	GPTU_OUT(GPTU0) =
		GPTU_OUT_CLRO0 |
		GPTU_OUT_CLRO1 |
		GPTU_OUT_CLRO2 |
		GPTU_OUT_CLRO3 |
		GPTU_OUT_CLRO4 |
		GPTU_OUT_CLRO5 |
		GPTU_OUT_CLRO6 |
		GPTU_OUT_CLRO7;
	
	GPTU_SRSEL(GPTU0) =
		GPTU_SRSEL_SSR7_SR00 |
		GPTU_SRSEL_SSR6_SR10 |
		GPTU_SRSEL_SSR5_OUV_T2B;
	
	GPTU_T2(GPTU0) = 0x00FFFFFF;
	GPTU_T012RUN(GPTU0) = GPTU_T012RUN_T2ASETR | GPTU_T012RUN_T2BSETR;
	
	stopwatch_t start = stopwatch_get();
	
	uint32_t max_cnt = 0;
	while (!(GPTU_SRC(GPTU0, 5) & MOD_SRC_SRR)) {
		max_cnt = MAX(max_cnt, GPTU_T2(GPTU0));
		//printf("%08X\n", GPTU_T0CBA(GPTU0));
	}
	max_cnt = MAX(max_cnt, GPTU_T2(GPTU0));
	uint32_t elapsed = stopwatch_elapsed_ms(start);
	
	printf("max_cnt=%08X\n", max_cnt);
	printf("elapsed: %d ms\n", elapsed);
}

int main(void) {
	wdt_init();
	// test1();
	// test2();
	test3();
	return 0;
}

__IRQ void data_abort_handler(void) {
	printf("data_abort_handler\n");
	while (true);
}

__IRQ void undef_handler(void) {
	printf("undef_handler\n");
	while (true);
}

__IRQ void prefetch_abort_handler(void) {
	printf("prefetch_abort_handler\n");
	while (true);
}
