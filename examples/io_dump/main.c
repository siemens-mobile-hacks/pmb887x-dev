#include <pmb887x.h>
#include <printf.h>

const char *get_mod_name(uint32_t id, bool is32);
void data_abort_handler2(void);

volatile bool is_data_abort = false;
volatile bool need_ignore_data_abort = false;
volatile uint32_t data_abort_context[16];
volatile uint8_t data_abort_stack[0x4000];

static void write_addr(uint32_t addr, uint32_t v) {
	need_ignore_data_abort = true;
	REG(addr) = v;
	need_ignore_data_abort = false;
}

static uint32_t read_addr(uint32_t addr) {
	is_data_abort = false;
	need_ignore_data_abort = true;
	uint32_t v = REG(addr);
	need_ignore_data_abort = false;
	return is_data_abort ? 0xFFFFFFFF : v;
}

static bool check_addr(uint32_t addr) {
	return read_addr(addr) != 0xFFFFFFFF;
}

void data_abort_handler2(void) {
	is_data_abort = true;
	if (!need_ignore_data_abort) {
		printf("data abort!\n");
		while (true);
	}
}

int main(void) {
	wdt_init();
	
	const bool for_config = true;
	
	SCU_RTCIF = 0xAA;
	SCU_CLC = 0x200;
	
	const uint32_t chunk = 0x100;
	
	for (uint32_t addr = 0xF0000000; addr <= 0xFFFF0000; addr += chunk) {
		wdt_serve();
		
		if (!check_addr(addr))
			continue;
		
		if (!check_addr(addr + 4) || !check_addr(addr + 8) || (REG(addr) & 1)) {
			write_addr(addr, 0x200);
			stopwatch_msleep_wd(100);
		}
		
		// AMBA
		uint32_t check_amba = read_addr(addr + 0xFE0);
		if (check_amba != 0xFFFFFFFF && check_amba != 0x00000000 && check_amba != 0xDEADDEAD) {
			uint32_t amba_pid = 0, amba_pcid = 0;
			for (int i = 0; i < 8; i++) {
				uint32_t v = read_addr(addr + 0xFE0 + i * 4);
				
				if (v & 0xFFFFFF00) {
					amba_pid = 0;
					amba_pcid = 0;
					break;
				}
				
				uint32_t n = i / 4, m = i - n * 4;
				if (n == 0) {
					amba_pid |= v << m * 8;
				} else if (n == 1) {
					amba_pcid |= v << m * 8;
				}
			}
			
			if (amba_pid || amba_pcid) {
				if (for_config) {
					printf("PL%03X\t%08X\tAMBA\t%08X /* %08X; %08X */\n", amba_pid & 0xFFF, addr, amba_pid, amba_pid, amba_pcid);
				} else {
					printf("%08X: AMBA %08X; %08X [PL%03X]\n", addr, amba_pid, amba_pcid, amba_pid & 0xFFF);
				}
				
				while (1) {
					uint32_t a1 = read_addr(addr + 0xFE0) | read_addr(addr + 0xFE4) << 8 | read_addr(addr + 0xFE8) << 16 || read_addr(addr + 0xFEC) << 24;
					uint32_t b1 = read_addr(addr + 0xFF0) | read_addr(addr + 0xFF4) << 8 | read_addr(addr + 0xFF8) << 16 || read_addr(addr + 0xFFC) << 24;
					
					unsigned next_addr = addr + 0x1000;
					
					uint32_t a2 = read_addr(next_addr + 0xFE0) | read_addr(next_addr + 0xFE4) << 8 | read_addr(next_addr + 0xFE8) << 16 || read_addr(next_addr + 0xFEC) << 24;
					uint32_t b2 = read_addr(next_addr + 0xFF0) | read_addr(next_addr + 0xFF4) << 8 | read_addr(next_addr + 0xFF8) << 16 || read_addr(next_addr + 0xFFC) << 24;
					
					if (a1 == a2 && b1 == b2) {
						addr = next_addr - chunk;
						continue;
					}
					
					wdt_serve();
					
					break;
				}
				
				continue;
			}
		}
		
	//	if (!check_addr(addr + 0x10))
	//		continue;
		
		wdt_serve();
		
		for (int i = 0; i < 3; i++) {
			wdt_serve();
			
			uint32_t id_addr = addr + ((2 - i)) * 4;
			
			uint32_t v = read_addr(id_addr);
			if (v == 0xFFFFFFFF || v == 0x00000000 || v == 0xDEADDEAD)
				continue;
			
			uint32_t MOD_REV = v & 0xFF;
			uint32_t MOD_32B = (v >> 8) & 0xFF;
			uint32_t MOD_NUM = (v >> 16) & 0xFFFF;
			
			if (MOD_32B != 0xC0) {
				if (MOD_NUM)
					continue;
				
				MOD_NUM = MOD_32B;
				MOD_32B = 0;
			}
			
			if (for_config) {
				printf("%s\t%08X\tMODULE\t%08X /* MOD_REV=%02X; MOD_32B=%02X; MOD_NUM=%04X */\n", get_mod_name(MOD_NUM, MOD_32B != 0), addr, v,  MOD_REV, MOD_32B, MOD_NUM);
			} else {
				printf("%08X: MOD_REV=%02X; MOD_32B=%02X; MOD_NUM=%04X [%s]\n", id_addr, MOD_REV, MOD_32B, MOD_NUM, get_mod_name(MOD_NUM, MOD_32B != 0));
			}
			
			if (MOD_32B && MOD_NUM == 0x0031)
				addr = (addr + 0x800000) - chunk;

			if (MOD_32B && MOD_NUM == 0xF021)
				addr = (addr + 0x10000) - chunk;
			
			if (MOD_32B && MOD_NUM == 0xF022)
				addr = (addr + 0x10000) - chunk;
			
			if (MOD_32B && MOD_NUM == 0xF023)
				addr = (addr + 0x1000) - chunk;
			
			if (MOD_32B && MOD_NUM == 0x0014)
				addr = (addr + 0x300) - chunk;
			
			break;
		}
		
		wdt_serve();
	}
	
	printf("Done.\n");
	return 0;
}

const char *get_mod_name(uint32_t id, bool is32) {
	if (is32) {
		switch (id) {
			case 0x0000:	return "STM";
			case 0x0001:	return "GPTU";
			case 0x0014:	return "EBU";
			case 0x0031:	return "VIC";

			case 0xF000:	return "SIM";
			case 0xF003:	return "GPRSCU";
			case 0xF004:	return "AFC";
			case 0xF00F:	return "MCI";
			case 0xF021:	return "TPU";
			case 0xF022:	return "DSP";
			case 0xF023:	return "PCL";
			case 0xF024:	return "ADC";
			case 0xF040:	return "SCU";
			case 0xF041:	return "MMCI";
			case 0xF043:	return "DIF";
			case 0xF046:	return "KEYPAD";
			case 0xF047:	return "USB";
			case 0xF048:	return "IRDA";
			case 0xF049:	return "RTC";
			case 0xF051:	return "USIF";
			case 0xF052:	return "CIF";
			case 0xF053:	return "MMICIF";
			case 0xF057:	return "I2C";
		}
	} else {
		switch (id) {
			case 0x0044:	return "USART";
			case 0x0045:	return "SSC";
			case 0x0050:	return "CAPCOM";
			case 0x0046:	return "I2C";
		}
	}
	return "???";
}
