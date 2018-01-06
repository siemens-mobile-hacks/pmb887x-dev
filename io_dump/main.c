#include <pmb8876.h>

#include "main.h"

extern void da_handler();
const char *get_mod_name(unsigned int id);

void main() {
	set_einit(0);
	disable_first_whatchdog();
	set_einit(1);
	init_watchdog();
	
	void **vectors = (void **) 0;
	vectors[8] = (void *) da_handler;
	vectors[9] = (void *) da_handler;
	vectors[10] = (void *) loop;
	vectors[11] = (void *) da_handler;
	vectors[12] = (void *) da_handler;
	vectors[13] = (void *) loop;
	vectors[14] = (void *) loop;
	vectors[15] = (void *) loop;
	
	unsigned int addr, v, irq_ok = 0, i;
	for (addr = 0xF0000000; addr <= 0xFFFF0000; addr += 0x100) {
		v = 0xFFFFFFFF;
		v = REG(addr);
		
		if (REG(addr) & 1 || !REG(addr)) {
			REG(addr) = 0x200;
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
			serve_watchdog();
		}
		
		v = 0xFFFFFFFF;
		v = REG(addr + 0xFE0);
		
		if (v && v != 0xFFFFFFFF && v != 0x00000000 && v != 0xDEADDEAD) {
			unsigned int amba_pid = 0, amba_pcid = 0;
			for (i = 0; i < 8; ++i) {
				v = 0xFFFFFFFF;
				v = REG(addr + 0xFE0 + i * 4);
				
				if (v & 0xFFFFFF00) {
					amba_pid = 0;
					amba_pcid = 0;
					break;
				}
				
				unsigned int n = i / 4, m = i - n * 4;
				if (n == 0) {
					amba_pid |= v << m * 8;
				} else if (n == 1) {
					amba_pcid |= v << m * 8;
				}
			}
			
			if (amba_pid || amba_pcid) {
				hexnum(&addr, 4);
				pmb8876_serial_print(": ");
				pmb8876_serial_print("AMBA ");
				hexnum(&amba_pid, 4);
				pmb8876_serial_print("; ");
				hexnum(&amba_pcid, 4);
				pmb8876_serial_print("\n");
				
				while (1) {
					unsigned int a1 = REG(addr + 0xFE0) | REG(addr + 0xFE4) << 8 | REG(addr + 0xFE8) << 16 || REG(addr + 0xFEC) << 24;
					unsigned int b1 = REG(addr + 0xFF0) | REG(addr + 0xFF4) << 8 | REG(addr + 0xFF8) << 16 || REG(addr + 0xFFC) << 24;
					
					unsigned next_addr = addr + 0x1000;
					
					unsigned int a2 = REG(next_addr + 0xFE0) | REG(next_addr + 0xFE4) << 8 | REG(next_addr + 0xFE8) << 16 || REG(next_addr + 0xFEC) << 24;
					unsigned int b2 = REG(next_addr + 0xFF0) | REG(next_addr + 0xFF4) << 8 | REG(next_addr + 0xFF8) << 16 || REG(next_addr + 0xFFC) << 24;
					
					if (a1 == a2 && b1 == b2) {
						addr = next_addr;
						continue;
					}
					
					serve_watchdog();
					
					break;
				}
				
				continue;
			}
		}
		
		serve_watchdog();
		
		for (i = 0; i <= 8; i += 4) {
			v = 0xFFFFFFFF;
			v = REG(addr + i);
			
			if (v != 0xFFFFFFFF && v != 0x00000000 && v != 0xDEADDEAD) {
				unsigned int MOD_REV = v & 0xFF;
				unsigned int MOD_32B = (v >> 8) & 0xFF;
				unsigned int MOD_NUM = (v >> 16) & 0xFFFF;
				
				if (!MOD_NUM && MOD_32B != 0xC0 && i != 0) {
					MOD_NUM = MOD_32B;
					MOD_32B = 0;
				}
				
				unsigned int id_addr = addr + i;
				
				if (MOD_32B ? MOD_32B == 0xC0 : i != 0 && id_addr != 0xF0000208 && (addr & 0xFFF00000) != 0xF6400000 && MOD_REV && !(v & 0xFFFF0000)) {
					if (MOD_32B == 0xC0 && MOD_NUM == 0x31) {
						if (irq_ok)
							break;
						irq_ok = 1;
					}
					
					hexnum(&id_addr, 4);
					pmb8876_serial_print(": ");
					
					pmb8876_serial_print("MOD_REV=");
					hexnum(&MOD_REV, 1);
					pmb8876_serial_print("; ");
					pmb8876_serial_print("MOD_32B=");
					hexnum(&MOD_32B, 1);
					pmb8876_serial_print("; ");
					pmb8876_serial_print("MOD_NUM=");
					hexnum(&MOD_NUM, 2);
					
					const char *name = get_mod_name(MOD_NUM);
					if (name) {
						pmb8876_serial_print(" (");
						pmb8876_serial_print(name);
						pmb8876_serial_print(")");
					}
					
					pmb8876_serial_print("\n");
					
					break;
				}
			}
		}
	}
	
	pmb8876_serial_print("Done\n");
	while (1);
}

const char *get_mod_name(unsigned int id) {
	switch (id) {
		case 0x0000:	return "STM"; 
		case 0x0001:	return "GPTU"; 
		case 0x0014:	return "EBU"; 
		case 0x0031:	return "IRQ"; 
		case 0x0044:	return "USART"; 
		case 0x0045:	return "SSC"; 
		case 0x0050:	return "CAPCOM"; 
		
		case 0xF000:	return "SIM"; 
		case 0xF022:	return "MODEM";
		case 0xF003:	return "GPRS_CHIP";
		case 0xF004:	return "GSMFRM";
		case 0xF021:	return "GSM_TPU"; 
		case 0xF023:	return "PCL"; 
		case 0xF024:	return "AMCPWR"; 
		case 0xF040:	return "SCU"; 
		case 0xF041:	return "AMBA_MMCI";
		case 0xF043:	return "DIF"; 
		case 0xF046:	return "KEYPAD"; 
		case 0xF052:	return "CIF"; 
		case 0xF053:	return "ATI-MMICIF";
		case 0xF057:	return "I2C";
	}
	
	return 0;
}

void __IRQ loop() {
	while (1);
}
