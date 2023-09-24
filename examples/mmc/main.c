#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	
	/*
		fSYS = 26 MHz
		fMMC = fSYS / 4
		
		BYPASS = 6.5 MHz
		
		div(1)	= 1.624 (fMCI / 4)
		div(2)	= 1.083 (fMCI / 6)
		
		mmc_freq = fSYS / ((clkdiv + 1) * 2)
	*/
	
	printf("Init MMC pins\n");
	GPIO_PIN(GPIO_MMC_VCC_EN) = GPIO_PS_MANUAL | GPIO_DIR_OUT | GPIO_DATA_HIGH;
	GPIO_PIN(GPIO_MMCI_CLK) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_OS_ALT0;
	GPIO_PIN(GPIO_MMCI_CMD) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_OS_ALT0;
	GPIO_PIN(GPIO_MMCI_DAT0) = GPIO_PS_ALT | GPIO_IS_ALT0 | GPIO_OS_ALT0;
	
	printf("Enable MCI clock\n");
	MMCI_CLC = 7 << MOD_CLC_RMC_SHIFT;
	
	printf("Start MCI\n");
	MCI_POWER = MCI_POWER_CTRL_POWER_UP;
	// MCI_CLOCK = MCI_CLOCK_BYPASS | MCI_CLOCK_ENABLE;
	MCI_CLOCK = (3 << MCI_CLOCK_CLKDIV_SHIFT) | MCI_CLOCK_ENABLE;
	MCI_POWER = MCI_POWER_CTRL_POWER_ON;
	
	printf("Wait 60 sec for oscilloscope!");
	stopwatch_sleep_wd(5);
	
	while (1);
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
