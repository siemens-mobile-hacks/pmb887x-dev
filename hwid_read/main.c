#include <pmb887x.h>
#include <printf.h>

int main(void) {
	wdt_init();
	
	gpio_init_input(GPIO_HW_DET_MOB_TYPE1, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_HW_DET_MOB_TYPE2, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_HW_DET_MOB_TYPE3, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_HW_DET_MOB_TYPE4, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_HW_DET_BLUETOOTH, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	gpio_init_input(GPIO_HW_DET_BAND_SEL, GPIO_IS_NONE, GPIO_PS_MANUAL, GPIO_PDPU_NONE, false);
	
	uint32_t hwid = 0;
	hwid |= gpio_get(GPIO_HW_DET_MOB_TYPE4) << 3;
	hwid |= gpio_get(GPIO_HW_DET_MOB_TYPE3) << 2;
	hwid |= gpio_get(GPIO_HW_DET_MOB_TYPE2) << 1;
	hwid |= gpio_get(GPIO_HW_DET_MOB_TYPE1) << 0;
	
	printf("hwid=%X (%d)\n", hwid, 400 + hwid);
	
	printf("GPIO_HW_DET_MOB_TYPE1=%d\n", gpio_get(GPIO_HW_DET_MOB_TYPE1));
	printf("GPIO_HW_DET_MOB_TYPE2=%d\n", gpio_get(GPIO_HW_DET_MOB_TYPE2));
	printf("GPIO_HW_DET_MOB_TYPE3=%d\n", gpio_get(GPIO_HW_DET_MOB_TYPE3));
	printf("GPIO_HW_DET_MOB_TYPE4=%d\n", gpio_get(GPIO_HW_DET_MOB_TYPE4));
	printf("GPIO_HW_DET_BLUETOOTH=%d\n", gpio_get(GPIO_HW_DET_BLUETOOTH));
	printf("GPIO_HW_DET_BAND_SEL=%d\n", gpio_get(GPIO_HW_DET_BAND_SEL));
	
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
