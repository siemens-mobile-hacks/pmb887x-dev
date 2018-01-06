#include <printf.h>
#include "main.h"
#include "i2c.h"


/* stack */
char stack_top[512 * 1024];
char irq_stack_top[512 * 1024];
unsigned int max_wd_time = 4;


#define D1601AA_I2C_ADDR		0x31 // Dialog/Twigo


#define RTC_CTRL		(0xF4700010)

#define RTC_OUTEN		(1 << 0)	/* external interrupt output enable */
#define RTC_INT			(1 << 1)	/* rtc interrupt status */
#define RTC_32KEN		(1 << 2) 	/* enable 32k osc */
#define RTC_PU32K		(1 << 3) 	/* power up 32k osc */
/*
    Logic Clock Select
	0 - 32 kHz clock operation mode (Asynchronous to microcontroller clock, low power, read only)
	1 - Bus clock operation mode (Synchronous to microcontroller clock, required for register write operation for some registers)
 */
#define RTC_CLK_SEL		(1 << 4) 
#define RTC_CLRINT		(1 << 8)	/* clear interrupt bit */
#define RTC_BAD			(1 << 9)	/* rtc bad detect bit */
#define RTC_CLRBAD		(1 << 10)	/* clear bad bit */


/* STM */
#define PMB8876_STM_CLC			0xF4B00000
#define PMB8876_STM_ID 			0xF4B00008
#define PMB8876_STM_0			0xF4B00010

#define PMB8876_STM_CLC_RMC(x)		((x << 8) & 0x7)

#define PMB8876_STM_CLOCK_FREQ		26000000

#define MAX_DELTA			( 32767 )
#define MIN_DELTA			( 10 )


#define IOPLL_CON0()		readl((void *)PLL_CON0)
#define IOPLL_CON0_SET(x)	writel(x, (void *)PLL_CON0)

#define IOPLL_CON1()		readl((void *)PLL_CON1)
#define IOPLL_CON1_SET(x)	writel(x, (void *)PLL_CON1)

#define IOPLL_CON2()		readl((void *)PLL_CON2)
#define IOPLL_CON2_SET(x)	writel(x, (void *)PLL_CON2)

#define IOPLL_ICR()		readl((void *)PLL_ICR)
#define IOPLL_ICR_SET(x)	writel(x, (void *)PLL_ICR)

#define IOPLL_OSC()		readl((void *)PLL_OSC)
#define IOPLL_OSC_SET(x)	writel(x, (void *)PLL_OSC)

#define IOPLL_STAT()		readl((void *)PLL_STAT)

#define IOSCU_PLLCLC()		readl((void *)SCU_PLLCLC)
#define IOSCU_PLLCLC_SET(x)	writel(x, (void *)SCU_PLLCLC)

#define IOEBU_CON()		readl((void *)EBU_CON)
#define IOEBU_CON_SET(x)	writel(x, (void *)EBU_CON)



unsigned int init_pll()
{
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
}

void EBU_wtf_clock_reinit_2()
{
    writel( readl(0xF4400044) | 1u, (void *)0xF4400044 );
    while ( !(readl(0xF4400044) & 0x10) );
}

void EBU_wtf_finish(void)
{
    REG(0xF4400040) |= 1u;
    while ( !(REG(0xF4400040) & 0x10) )
    ;
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



unsigned int _pmb8876_pll_reclock(char mul1, char mul2, char div1, char div2);

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

unsigned int pll_reclock(char mul1)
{
    _pmb8876_pll_reclock(3, 1, 1, 0);
}

unsigned int _pmb8876_pll_reclock(char mul1, char mul2, char div1, char div2)
{
    //( 1 << 12 ) - делитель, 3х битный
    IOPLL_CON0_SET( 0x11200800 | (0 << 16) | (div2 << 12) ); 
    
    // (5 << 16) - множитель 1 - 5
    // (5 << 8) - тоже какая-то хуитка, похожая на множитель
    writel((mul1 << 16) | (0 << 12) | (5 << 8) | 5, (void *)PLL_OSC);
    
    printk(" -> osc: %X\n", readl((void *)PLL_OSC));

    // (1 << 5) - PLL_CONNECT
    // (1 << 8) - делитель, очень стрёмный, при увеличении вроде тупее работает, но богомипсов одинаково
    writel((0x10 << 24) | (7 << 13) | (0 << 12) | (div1 << 8) | (1 << 5) | 0x7, (void *)PLL_CON2);
    printk(" -> con2: %X\n", readl((void *)PLL_CON2));
    
    EBU_wtf_clock_reinit_2();
    
    // (1 << (20+0)) - иножитель
    // (1 << (16+0)) - хрень какая-то
    // (1 << 1) - если установлен, нужно пересчитывать CLC периферии
    writel((1 << (20+mul2)) | (0 << 16) | 0, (void *)PLL_CON1);
    printk(" -> con1: %X\n", readl((void *)PLL_CON1));
    
    writel(readl((void *)0xF45000B4) | 0x310000, (void *)0xF45000B4);
    printk(" -> b4: %X\n", readl((void *)0xF45000B4));
    
    printk("con0: %X\n", IOPLL_CON0());
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040);
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() | 0x4000000u );
    writel(0, (void *)0xF4400040);
    
    
    writel( readl((void *)0xF4400040) | 1u, (void *)0xF4400040 );
    while ( !(readl((void *)0xF4400040) & 0x10) );
    
    IOEBU_CON_SET( IOEBU_CON() & 0xFBFFFFFF );
    writel(0, (void *)0xF4400040);
}

void EBU_some_state_wait()
{
    writel( readl(0xF4400044) & 0xFFFFFFFE, (void *)0xF4400044 );
    while ( !(readl(0xF4400044) & 0x10) );
}


void activate_rtc()
{
    int a1 = 1487538810, result;
    REG(SCU_RTCIF) = 0xAA;
    
    REG(0xF4700000) = (1 << 8);
    REG(RTC_CTRL) |= RTC_PU32K | RTC_32KEN;
    
    REG(0xF4700014) = 2;
    REG(0xF4700018) = 0xF000F000;
    
    //REG(0xF470001C) = a1;
    
    REG(0xF4700020) = 0;
    REG(0xF470002C) = -21075;
    REG(0xF4700024) = 0;
    REG(0xF4700028) = 1;
    
    REG(RTC_CTRL) |= RTC_CLK_SEL | RTC_CLRBAD | RTC_CLRINT;
    if ( REG(RTC_CTRL) & RTC_BAD ) {
	
    } else {
	// Start count
	REG(0xF4700014) |= 1u;
    }
}



int cpu_rate()
{
    /*volatile unsigned long cnt = 0;
    unsigned long start = REG(0xF470001C)+1, end;
    
    while( start != REG(0xF470001C) );
    end = start+1;
    
    while( end != REG(0xF470001C) ) {
	cnt ++;
    }
    
    
    /*unsigned int overflow = 0;
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


void main() {
	init_watchdog_noinit();
	
	volatile int i;
	void **vectors = (void **) 0;
	vectors[8] = reset_addr;
	vectors[9] = undef_addr;
	vectors[10] = swi_addr;
	vectors[11] = prefetch_addr;
	vectors[12] = abort_addr;
	vectors[13] = reserved_addr;
	vectors[14] = c_irq_handler; // asm_irq_handler;
	vectors[15] = fiq_test;
	
	unsigned int addr;
	for (addr = 0xf2800030; addr <= 0xf28002a8; ++addr) {
		REG(addr) = 0;
	}
	
	enable_irq(1);
	
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
	REG(EBU_SDRMCON0) = 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10);
	REG(EBU_SDRMCON1) = 0xD02070 | (7 << 16) | (2 << 14) | (2 << 10);
	
	
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
	
	REG(USART0_CLC) = (1 << (8)) | 8;
	//REG(USART0_BG) = 0;
	//REG(USART0_FDV) = 0;
	
	for(int i = 0; i<2000; i++);
	
	printf("Xuj!\n");
	printf("Xuj!\n");
	printf("Xuj!\n");
	
	printk(" ->  osc: %X\n", readl((void *)PLL_OSC));
	printk(" -> con0: %X\n", readl((void *)PLL_CON0));
	printk(" -> con1: %X\n", readl((void *)PLL_CON1));
	printk(" -> con2: %X\n", readl((void *)PLL_CON2));
	printk(" -> 00b4: %X\n", readl((void *)0xF45000B4));
	
	
	int mhz = cpu_rate();
	printf("CPU: %d MHz\n", mhz);
	
	
	volatile int cnt = 0;
	int last_stm = REG(STM_4);
	i = 0;
	
	printf("last_stm: %d\n", last_stm);
	
	while(1) {
	    if( i > 60000 )
	    {
		printf("\rcnt %d, %d", cnt ++, REG(STM_4) - last_stm);
		last_stm = REG(STM_4);
		i = 0;
	    }
	    i++;
	}
}

