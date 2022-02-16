#include "main.h"

#define D1601AA_I2C_ADDR		0x31 // Dialog/Twigo

/* STM */
#define PMB8876_STM_CLC			0xF4B00000
#define PMB8876_STM_ID 			0xF4B00008
#define PMB8876_STM_0			0xF4B00010

#define PMB8876_STM_CLC_RMC(x)		((x << 8) & 0x7)

#define PMB8876_STM_CLOCK_FREQ		26000000

#define MAX_DELTA			( 32767 )
#define MIN_DELTA			( 10 )


#define IOPLL_CON0()		PLL_CON0
#define IOPLL_CON0_SET(x)	(PLL_CON0 = x)

#define IOPLL_CON1()		PLL_CON1
#define IOPLL_CON1_SET(x)	(PLL_CON1 = x)

#define IOPLL_CON2()		PLL_CON2
#define IOPLL_CON2_SET(x)	(PLL_CON2 = x)

#define IOPLL_ICR()			PLL_SRC
#define IOPLL_ICR_SET(x)	(PLL_SRC = x)

#define IOPLL_OSC()			PLL_OSC
#define IOPLL_OSC_SET(x)	(PLL_OSC = x)

#define IOPLL_STAT()		PLL_STAT

#define IOSCU_PLLCLC()		SCU_PLLCLC
#define IOSCU_PLLCLC_SET(x)	(SCU_PLLCLC = x)

#define IOEBU_CON()			EBU_CON
#define IOEBU_CON_SET(x)	(EBU_CON = x)

static unsigned int init_pll(void) {
    /*IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFFF8) | 3 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFFF87) | 8 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFFF8FF) );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFFF87FF) | 0x1800 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFFF8FFFF) | 0x40000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xFF87FFFF) | 0x80000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0xF8FFFFFF) | 0x1000000 );
    IOPLL_CON0_SET( (IOPLL_CON0() & 0x87FFFFFF) | 0x10000000 );*/
    
    //1120080B
    //110C180B
    //( 1 << 12 ) - делитель, 3х битный
    
    IOPLL_CON0_SET( 0x11200800 | (0 << 16) | (0 << 12) ); 

    IOPLL_ICR_SET( IOPLL_ICR() & 0xFFFFEFFF );
    IOPLL_OSC_SET( IOPLL_OSC() & 0xF0FFFFFF );
    IOPLL_OSC_SET( (IOPLL_OSC() & 0xFFC0FFFF) | 0x30000 );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x1000u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x100u );
    IOPLL_OSC_SET( IOPLL_OSC() | 0x10u );
    IOPLL_OSC_SET( IOPLL_OSC() | 1u );

    while ( !(IOPLL_STAT() & 0x2000) );

    IOPLL_CON2_SET( (IOPLL_CON2() & 0xFFFF3FFF) | 0x8000 );
    writel( (readl((void *)0xF45000B4) & 0xFCFFFFFF) | 0x1000000, (void *)0xF45000B4 );
    
    return 0;
}

static void EBU_wtf_clock_reinit_2(void) {
    writel( readl(0xF4400044) | 1u, (void *)0xF4400044 );
    while ( !(readl(0xF4400044) & 0x10) );
}

static void EBU_wtf_finish(void) {
    REG(0xF4400040) |= 1u;
    while ( !(REG(0xF4400040) & 0x10) );
    REG(EBU_CON) |= 0x4000000;
    REG(0xF4400040) = 0;
}


/*
F45000A0: 00030505 (PLL_OSC)
F45000A4: 1120080B (PLL_CON0)
F45000A8: 00420002 (PLL_CON1)
F45000AC: 0x1000E127 (PLL_CON2)
F45000B0: 00002000 (PLL_STAT)
F45000B4: 10000303 (PLL_*)
*/


// 1 - 26
// 2 - 78
// 3 - 104
// 4 - 130
// 5 - 156
// 3179900
// 2590256

/*
F45000A0: 00030505 (PLL_OSC)
F45000A4: 1120080B (PLL_CON0)
F45000A8: 00420002 (PLL_CON1)
F45000AC: 1000E127 (PLL_CON2)
F45000B0: 00002000 (PLL_STAT)
F45000B4: 10000303 (PLL_*)
F45000B8: FFFFFFFF (PLL_*)
F45000BC: FFFFFFFF (PLL_*)
F45000C0: FFFFFFFF (PLL_*)
F45000C4: FFFFFFFF (PLL_*)
F45000C8: FFFFFFFF (PLL_*)
F45000CC: 00001000 (PLL_ICR)*/



static unsigned int _pll_reclock(char mul1, char mul2, char div1, char div2);

/*
 12  MHz: 2, 2, 1, 6
 18  MHz: 2, 2, 1, 4
 
 26  MHz: 1, 1, 1, 0
 52  MHz: 2, 2, 1, 1
 78  MHz: 2, 1, 1, 0
 104 MHz: 3, 1, 1, 0
 156 MHz: 2, 2, 1, 0
 208 MHz: 3, 2, 1, 0
 260 MHz: 4, 2, 1, 0
 312 MHz: 5, 2, 1, 0
 
 */

static unsigned int pll_reclock(char mul1) {
	return _pll_reclock(3, 1, 1, 0);
}

static unsigned int _pll_reclock(char mul1, char mul2, char div1, char div2) {
    //( 1 << 12 ) - делитель, 3х битный
    IOPLL_CON0_SET( 0x11200800 | (0 << 16) | (div2 << 12) ); 
    
    // (5 << 16) - множитель 1 - 5
    // (5 << 8) - тоже какая-то хуитка, похожая на множитель
    PLL_OSC = ((mul1 << 16) | (0 << 12) | (5 << 8) | 5);
    
    printf(" -> osc: %X\n", PLL_OSC);

    // (1 << 5) - PLL_CONNECT
    // (1 << 8) - делитель, очень стрёмный, при увеличении вроде тупее работает, но богомипсов одинаково
    PLL_CON2 = (0x10 << 24) | (7 << 13) | (0 << 12) | (div1 << 8) | (1 << 5) | 0x7;
    printf(" -> con2: %X\n", PLL_CON2);
    
    EBU_wtf_clock_reinit_2();
    
    // (1 << (20+0)) - иножитель
    // (1 << (16+0)) - хрень какая-то
    // (1 << 1) - если установлен, нужно пересчитывать CLC периферии
    PLL_CON1 = ((1 << (20+mul2)) | (0 << 16) | 0);
    printf(" -> con1: %X\n", PLL_CON1);
    
    writel(readl((void *)0xF45000B4) | 0x310000, (void *)0xF45000B4);
    printf(" -> b4: %X\n", readl((void *)0xF45000B4));
    
    printf("con0: %X\n", IOPLL_CON0());
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040);
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() | 0x4000000u );
    writel(0, (void *)0xF4400040);
    
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040 );
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() & 0xFBFFFFFF );
    writel(0, (void *)0xF4400040);
    
    return 0;
}

static void EBU_some_state_wait(void) {
    writel( readl(0xF4400044) & 0xFFFFFFFE, (void *)0xF4400044 );
    while ( !(readl(0xF4400044) & 0x10) );
}


static void activate_rtc(void) {
    int a1 = 1487538810, result;
    SCU_RTCIF = 0xAA;
    
    REG(0xF4700000) = (1 << 8);
    RTC_CTRL |= RTC_CTRL_PU32K | RTC_CTRL_CLK32KEN;
    
    REG(0xF4700014) = 2;
    REG(0xF4700018) = 0xF000F000;
    
    //REG(0xF470001C) = a1;
    
    REG(0xF4700020) = 0;
    REG(0xF470002C) = -21075;
    REG(0xF4700024) = 0;
    REG(0xF4700028) = 1;
    
	RTC_CTRL |= RTC_CTRL_CLK_SEL | RTC_CTRL_CLR_RTCBAD | RTC_CTRL_CLR_RTCINT;
    if (RTC_CTRL & RTC_CTRL_RTCBAD) {
	
    } else {
		// Start count
		REG(0xF4700014) |= 1u;
    }
}



static int cpu_rate(void) {
    /* volatile unsigned long cnt = 0;
    unsigned long start = REG(0xF470001C)+1, end;
    
    while( start != REG(0xF470001C) );
    end = start+1;
    
    while( end != REG(0xF470001C) ) {
	cnt ++;
    }
    
    
    /* unsigned int overflow = 0;
    for( int i = 0; i < 2; i ++ ) 
    {
	unsigned long stm_start = REG(STM_0);
	cnt = 0;
	while( (overflow = ((REG(STM_0) - stm_start))) < 26000000) 
	{
	    ++ cnt;
	}
    }*/
    
    
    unsigned long lpj = calibrate_delay_converge();
    
    printf("%d.%02d BogoMIPS (lpj=%d)\n",
			lpj/(500000/HZ),
			(lpj/(5000/HZ)) % 100, lpj);
    
    
    return ((lpj / (500000 / HZ) + 1) * 2);
}

int main(void) {
	wdt_init();
	activate_rtc();
	
	/* fuck watchdog */
	i2c_smbus_write_byte(D1601AA_I2C_ADDR, 0xE, 0b11);
	
	//REG(STM_CLC) = 0x001A1A04;
	
	/*REG(EBU_CLC) = 0;
	REG(EBU_CON) = 0x68;
	REG(EBU_BFCON) = 0x104D03;
	REG(EBU_SDRMREF0) = 0x219;
	REG(EBU_SDRMREF1) = 0x00000000;
	*/
	
	//(16 - 3) - Row cycle time counter
	//(14 - 2) - Row to column delay counter
	//(10 - 2) - Row precharge time counter
	
	//0x00D7A870
	EBU_SDRMCON(0) = 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10);
	EBU_SDRMCON(1) = 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10);
	
	
	//(4 - 3) - cas latency
	// 0x33
	REG(0xF0000060) = (3 << 4) | 3;
	REG(0xF0000068) = (3 << 4) | 3;
	
	//REG(EBU_SDRMCON1) = 0x00000000;
	
	/*REG(EBU_ADDRSEL2) = 0x90000010;
	REG(EBU_ADDRSEL3) = 0xC0000041;
	REG(EBU_ADDRSEL5) = 0xA8000030;
	REG(EBU_ADDRSEL6) = 0x90000010;
	
	REG(EBU_BUSCON0)  = 0xA2520E00;
	
	REG(EBU_BUSCON2) = 0x00000000;
	REG(EBU_BUSCON3) = 0x00422670;
	REG(EBU_BUSCON4) = 0x80522600;
	REG(EBU_BUSCON5) = 0x30420200;
	REG(EBU_BUSCON6) = 0x80520637;
	
	REG(EBU_BUSAP0) = 0x8CE11222;
	REG(EBU_BUSAP1) = 0x00000000;
	REG(EBU_BUSAP2) = 0x00000000;
	REG(EBU_BUSAP3) = 0x41F8FF00;
	REG(EBU_BUSAP4) = 0x44D15211;
	REG(EBU_BUSAP5) = 0x00000000;
	
	REG(EBU_EMUAS)  = 0xC0000040;
	REG(EBU_EMUBC)  = 0x80520637;
	REG(EBU_EMUOVL) = 0x00000000;
	
	REG(EBU_USERCON) = 0x00000000;*/
	
	
	for(volatile int i = 0; i<5000; i++);
	
	printf("Init PLL...\n");
	
	init_pll();
	pll_reclock(5);
	
	USART_CLC(USART0) = (1 << (8)) | 8;
	
	for(int i = 0; i<2000; i++);
	
	printf("Xuj!\n");
	printf("Xuj!\n");
	printf("Xuj!\n");
	
	printf(" ->  osc: %X\n", PLL_OSC);
	printf(" -> con0: %X\n", PLL_CON0);
	printf(" -> con1: %X\n", PLL_CON1);
	printf(" -> con2: %X\n", PLL_CON2);
	printf(" -> 00b4: %X\n", REG(0xF45000B4));
	
	int mhz = cpu_rate();
	printf("CPU: %d MHz\n", mhz);
	
	
	volatile int cnt = 0;
	int last_stm = STM_TIM4;
	int i = 0;
	
	printf("last_stm: %d\n", last_stm);
	
	while (true) {
	    if (i > 60000) {
			printf("\rcnt %d, %d", cnt ++, STM_TIM4 - last_stm);
			last_stm = STM_TIM4;
			i = 0;
	    }
	    i++;
	}
	
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
