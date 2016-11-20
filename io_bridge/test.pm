package siemens_boot;

use warnings;
use strict;
use Device::SerialPort;
my $start = time;

sub boot_module_init {
	my $port = shift;
	
	$port->read_char_time(1000);
	$port->read_const_time(1000);
	
	my $buf;
	open F, "+>/dev/shm/siemens_io_sniff" or die("$!");
	binmode F;
	chmod 0666, "/dev/shm/siemens_io_sniff";
	
	sub reset_cmd {
		truncate F, 0;
		seek(F, 0, 0);
	}
	my $xuj = 1;
	reset_cmd();
	while (1) {
		seek F, 0, 0;
		read(F, $buf, 128);
		
		my $cmd = substr($buf, 0, 1);
		if ($cmd eq "R") {
			
			my $addr = unpack("V", substr($buf, 1, 4));
			my $data = cmd_read($port, $addr, 1);
			
			reset_cmd();
			print F "!".$data; # OK + data
			
			my $vv = unpack("V", $data);
			printf("READ from %08X (%08X)%s\n", $addr, $vv, reg_name($addr, $vv));
			
			$xuj = 1;
			if ($addr >= 0xf4400000 && $addr <= 0xf440FFFF) {
				$xuj = 0;
			}
		} elsif ($cmd eq "W") {
			reset_cmd();
			my $addr = unpack("V", substr($buf, 1, 4));
			my $value = unpack("V", substr($buf, 5, 4));
			
			my $valid = 1;
			if ($addr == 0xF430004C || $addr == 0xF4300050) {
				# Запретим писать в TX/RX пины, мы же работаем по UART :)
				$valid = 0;
			}
			
			if ($addr == 0xF4400024 || $addr == 0xF4400028 || $addr == 0xF4300118 || $addr == 0xf440007C) {
				# Собаку отключили, нет смысла пробрасывать её регистры для записи, да и падает из-за этого
				$valid = 0;
			}
			
			if ($addr >= 0xF4500000 && $addr <= 0xF4500FFF) {
				# UART отваливается от смены частоты CPU, нужно что-то с этим делать...
				$valid = 0;
			}
			
			if ($addr == 0xF4B00000) {
				# Если сменить частоту клока - всё валится
				$valid = 0;
			}
			
#	if (offset == 0xf4400024 || offset == 0xf4400028 || offset == 0xf45000a8 || (offset >= 0xf6400000 && offset <= 0xf640ffff))
#		return;
			
			printf("WRITE %08X to %08X%s%s\n", $value, $addr, reg_name($addr, $value), $valid ? "" : " | SKIP!!!!");
			cmd_write($port, $addr, $value) if ($valid);
			
			print F "!"; # OK
			
			$xuj = 1;
			if ($addr >= 0xf4400000 && $addr <= 0xf440FFFF) {
				$xuj = 0;
			}
		} else {
			if ($xuj) {
				cmd_ping($port);
			}
		}
		
#		cmd_ping($port);
#		cmd_write($port, 0x80000, 0xDEADBEEF);
#		printf("%08X\n", cmd_read($port, 0x80000));
	}
	close F;
	
	my $c;
	while (($c = readb($port)) >= 0) {
		print chr($c);
	}
}

sub reg_name {
	my $addr = shift;
	my $value = shift;
	
	my $names = [
		["PCL_*", 0xf4300000, 0xf4300200], 
		["SIM_*", 0xF000C010, 0xF000C010 + 0x100], 
		["PCL_*", 0xf4300000, 0xf4300200], 
		
		
		# USART0
		["USART0_CLC", 0xF1000000], 
		["USART0_PISEL", 0xF1000004], 
		["USART0_ID", 0xF1000008], 
		["USART0_CON", 0xF1000010], 
		["USART0_BG", 0xF1000014], 
		["USART0_FDV", 0xF1000018], 
		["USART0_PMW", 0xF100001C], 
		["USART0_TXB", 0xF1000020], 
		["USART0_RXB", 0xF1000024], 
		["USART0_ABCON", 0xF1000030], 
		["USART0_ABSTAT", 0xF1000034], 
		["USART0_RXFCON", 0xF1000040], 
		["USART0_TXFCON", 0xF1000044], 
		["USART0_FSTAT", 0xF1000048], 
		["USART0_WHBCON", 0xF1000050], 
		["USART0_FCSTAT", 0xF1000068], 
		["USART0_ICR", 0xF1000070], 
		
		# USART2
		["USART1_CLC", 0xF1800000], 
		["USART1_PISEL", 0xF1800004], 
		["USART1_ID", 0xF1800008], 
		["USART1_CON", 0xF1800010], 
		["USART1_BG", 0xF1800014], 
		["USART1_FDV", 0xF1800018], 
		["USART1_PMW", 0xF180001C], 
		["USART1_TXB", 0xF1800020], 
		["USART1_RXB", 0xF1800024], 
		["USART1_ABCON", 0xF1800030], 
		["USART1_ABSTAT", 0xF1800034], 
		["USART1_RXFCON", 0xF1800040], 
		["USART1_TXFCON", 0xF1800044], 
		["USART1_FSTAT", 0xF1800048], 
		["USART1_WHBCON", 0xF1800050], 
		["USART1_FCSTAT", 0xF1800068], 
		["USART1_ICR", 0xF1800070], 
		
		# SSC0
		["SSC0_CLC", 0xF1100000], 
		["SSC0_PISEL", 0xF1100004], 
		["SSC0_ID", 0xF1100008], 
		["SSC0_CON", 0xF1100010], 
		["SSC0_BR", 0xF1100014], 
		["SSC0_SSOC", 0xF1100018], 
		["SSC0_SSOTC", 0xF110001C], 
		["SSC0_TB", 0xF1100020], 
		["SSC0_RB", 0xF1100024], 
		["SSC0_STAT", 0xF1100028], 
		["SSC0_EFM", 0xF110002C], 
		["SSC0_RXFCON", 0xF1100030], 
		["SSC0_TXFCON", 0xF1100034], 
		["SSC0_FSTAT", 0xF1100038], 
		
		# SSC2
		["SSC2_CLC", 0xF1B00000], 
		["SSC2_PISEL", 0xF1B00004], 
		["SSC2_ID", 0xF1B00008], 
		["SSC2_CON", 0xF1B00010], 
		["SSC2_BR", 0xF1B00014], 
		["SSC2_SSOC", 0xF1B00018], 
		["SSC2_SSOTC", 0xF1B0001C], 
		["SSC2_TB", 0xF1B00020], 
		["SSC2_RB", 0xF1B00024], 
		["SSC2_STAT", 0xF1B00028], 
		["SSC2_EFM", 0xF1B0002C], 
		["SSC2_RXFCON", 0xF1B00030], 
		["SSC2_TXFCON", 0xF1B00034], 
		["SSC2_FSTAT", 0xF1B00038], 
		
		# USB
		["USB_CLC", 0xF2200000], 
		["USB_PISEL", 0xF2200004], 
		["USB_ID", 0xF2200008], 
		["USB_DCR", 0xF2200010], 
		["USB_DSR", 0xF2200014], 
		["USB_EPSTL", 0xF2200018], 
		["USB_EPSSR", 0xF220001C], 
		["USB_CNFR", 0xF2200020], 
		["USB_FNR", 0xF2200024], 
		["USB_EPDIR", 0xF2200028], 
		["USB_EPDSR", 0xF220002C], 
		["USB_FCON", 0xF2200030], 
		["USB_CPLPR", 0xF2200034], 
		["USB_DATA32", 0xF2200038], 
		["USB_DATA16", 0xF220003C], 
		["USB_DATA8", 0xF2200040], 
		["USB_EPVLD", 0xF2200044], 
		["USB_EVSR", 0xF2200048], 
		["USB_ZLPEN", 0xF2200090], 
		["USB_ZLPSR", 0xF2200094], 
		["USB_DIER", 0xF220004C], 
		["USB_DIRR", 0xF2200050], 
		["USB_DIRST", 0xF2200054], 
		["USB_DINP", 0xF2200058], 
		["USB_EPIR0", 0xF220005C], 
		["USB_EPIR1", 0xF2200060], 
		["USB_EPIR2", 0xF2200064], 
		["USB_EPIR3", 0xF2200068], 
		["USB_EPIRST0", 0xF220006C], 
		["USB_EPIRST1", 0xF2200070], 
		["USB_EPIRST2", 0xF2200074], 
		["USB_EPIRST3", 0xF2200078], 
		
		# I2C
		["I2C_CLC", 0xF4800000], 
		["I2C_PISEL", 0xF4800004], 
		["I2C_ID", 0xF4800008], 
		["I2C_SYSCON", 0xF4800010], 
		["I2C_BUSCON", 0xF4800014], 
		["I2C_RTB", 0xF4800018], 
		["I2C_WHBSYSCON", 0xF4800020], 
		
		# EBU
		["EBU_*", 0xF0000000, 0xF0000200], 
		
		["EBU_CLC", 0xF0000000], 
		["EBU_ID", 0xF0000008], 
		["EBU_CON", 0xF0000010], 
		["EBU_BFCON", 0xF0000020], 
		
		["EBU_ADDRSEL0", 0xF0000080], 
		["EBU_ADDRSEL1", 0xF0000088], 
		["EBU_ADDRSEL2", 0xF0000090], 
		["EBU_ADDRSEL3", 0xF0000098], 
		["EBU_ADDRSEL4", 0xF00000A0], 
		["EBU_ADDRSEL5", 0xF00000A8], 
		["EBU_ADDRSEL6", 0xF00000B0], 
		
		["EBU_BUSCON0", 0xF00000C0], 
		["EBU_BUSCON1", 0xF00000C8], 
		["EBU_BUSCON2", 0xF00000D0], 
		["EBU_BUSCON3", 0xF00000D8], 
		["EBU_BUSCON4", 0xF00000E0], 
		["EBU_BUSCON5", 0xF00000E8], 
		["EBU_BUSCON6", 0xF00000F0], 
		
		["EBU_SDRMREF0", 0xF0000040], 
		["EBU_SDRMCON0", 0xF0000050], 
		["EBU_SDRMOD0", 0xF0000060], 
		
		["EBU_BUSAP0", 0xF0000100], 
		["EBU_BUSAP1", 0xF0000108], 
		["EBU_BUSAP2", 0xF0000110], 
		["EBU_BUSAP3", 0xF0000118], 
		["EBU_BUSAP4", 0xF0000120], 
		["EBU_BUSAP5", 0xF0000128], 
		["EBU_BUSAP6", 0xF0000130], 
		
		["EBU_EMUAS", 0xF0000160], 
		["EBU_EMUBC", 0xF0000168], 
		["EBU_EMUBAP", 0xF0000170], 
		["EBU_EMUOVL", 0xF0000178], 
		["EBU_USERCON", 0xF0000190], 
		
		# SCU
		["SCU_*", 0xf4400000, 0xf4400200], 
		["SCU_BASE", 0xf4400000], 
		["SCU_ROMAMCR", 0xf440007C], 
		["SCU_WDTCON0", 0xf4400024], 
		["SCU_WDTCON1", 0xf4400028], 
		["SCU_CHIPID", 0xf4400060], 
		["SCU_RST_CTRL_ST", 0xF4400018], 
		["SCU_RTCIF", 0xF4400064], 
		["SCU_CHIPID", 0xf4400060], 
		
		# PCL
		["PCL_*", 0xf4300000, 0xf4300200], 
		["PCL_CLC", 0xf4300000], 
		["PCL_ID", 0xf4300008], 
		["MON_CR1", 0xf4300010], 
		["MON_CR2", 0xf4300014], 
		["MON_CR3", 0xf4300018], 
		["MON_CR4", 0xf430001C], 
		["GPIO_KP_IN0", 0xF4300020], 
		["GPIO_KP_IN1", 0xF4300024], 
		["GPIO_KP_IN2", 0xF4300028], 
		["GPIO_KP_IN3", 0xF430002C], 
		["GPIO_KP_IN4", 0xF4300030], 
		["GPIO_KP_IN5", 0xF4300034], 
		["GPIO_KP_OUT5_OUT6", 0xF4300038], 
		["GPIO_KP_OUT0", 0xF430003C], 
		["GPIO_KP_OUT1", 0xF4300040], 
		["GPIO_KP_OUT2", 0xF4300044], 
		["GPIO_KP_OUT3", 0xF4300048], 
		["GPIO_USART0_RXD", 0xF430004C], 
		["GPIO_USART0_TXD", 0xF4300050], 
		["GPIO_USART0_RTS", 0xF4300054], 
		["GPIO_USART0_CTS", 0xF4300058], 
		["GPIO_DSPOUT0", 0xF430005C], 
		["PCL_16", 0xF4300060], 
		["PCL_17", 0xF4300064], 
		["PCL_18", 0xF4300068], 
		["PCL_19", 0xF430006C], 
		["PCL_20", 0xF4300070], 
		["PCL_21", 0xF4300074], 
		["PCL_22", 0xF4300078], 
		["PCL_23", 0xF430007C], 
		["PCL_24", 0xF4300080], 
		["PCL_25", 0xF4300084], 
		["PCL_26", 0xF4300088], 
		["PCL_27", 0xF430008C], 
		["GPIO_I2C_SCL", 0xF4300090], 
		["GPIO_I2C_SDA", 0xF4300094], 
		["PCL_30", 0xF4300098], 
		
		["I2S2_*", 0xF430009B, 0xF43000A8], 
		
#		["PCL_31", 0xF430009C], 
#		["PCL_32", 0xF43000A0], 
#		["PCL_33", 0xF43000A4], 
#		["PCL_34", 0xF43000A8], 
		
		["PCL_35", 0xF43000AC], 
		["PCL_36", 0xF43000B0], 
		["PCL_37", 0xF43000B4], 
		["PCL_38", 0xF43000B8], 
		["PCL_39", 0xF43000BC], 
		["PCL_40", 0xF43000C0], 
		["PCL_41", 0xF43000C4], 
		["PCL_42", 0xF43000C8], 
		["PCL_43", 0xF43000CC], 
		["GPIO_TOUT1_PM_CHARGE_UC", 0xF43000D0], 
		["PCL_45", 0xF43000D4], 
		["PCL_46", 0xF43000D8], 
		["PCL_47", 0xF43000DC], 
		["PCL_48", 0xF43000E0], 
		["PCL_49", 0xF43000E4], 
		["GPIO_TOUT7_PM_RF2_EN", 0xF43000E8], 
		["PCL_51", 0xF43000EC], 
		["GPIO_TOUT9_I2C_2_DAT", 0xF43000F0], 
		["GPIO_TOUT10_SERIAL_EN", 0xF43000F4], 
		["GPIO_TOUT11_I2C_2_CLK", 0xF43000F8], 
		["PCL_55", 0xF43000FC], 
		["PCL_56", 0xF4300100], 
		["GPIO_RF_STR1", 0xF4300104], 
		["PCL_58", 0xF4300108], 
		["PCL_59", 0xF430010C], 
		["PCL_60", 0xF4300110], 
		["PCL_61", 0xF4300114], 
		["GPIO_DSPOUT1_PM_WADOG", 0xF4300118],
		
		["GPIO_CIF_D0", 0xF4300158], 
		["GPIO_CIF_D1", 0xF430015C], 
		["GPIO_CIF_D2", 0xF4300160], 
		["GPIO_CIF_D3", 0xF4300164], 
		["GPIO_CIF_D4", 0xF4300168], 
		["GPIO_CIF_D5", 0xF430016C], 
		["GPIO_CIF_D6", 0xF4300170], 
		["GPIO_CIF_D7", 0xF4300174], 
		["GPIO_CIF_PCLK", 0xF4300178], 
		["GPIO_CIF_HSYNC", 0xF430017C], 
		["GPIO_CIF_VSYNC", 0xF4300180], 
		["GPIO_CLKOUT2", 0xF4300184], 
		["GPIO_CIF_PD", 0xF43001E4],
		
		# STM
		["STM_*", 0xF4B00000, 0xF4B00100], 
		["STM_CLC", 0xF4B00000], 
		["STM_ID", 0xF4B00008], 
		["STM_0", 0xF4B00010], 
		["STM_1", 0xF4B00014], 
		["STM_2", 0xF4B00018], 
		["STM_3", 0xF4B0001C], 
		["STM_4", 0xF4B00020], 
		["STM_5", 0xF4B00024], 
		["STM_6", 0xF4B00028], 
		
		# PLL
		["PLL_*", 0xF4500000, 0xF4500FFF], 
		
		# GSM_TPU
		["GSM_TPU_*", 0xF6400000, 0xF6400FFF], 
		
		# DSP??
		["DSP??_*", 0xF6401000, 0xF6401FFF], 
		
		# GPTU*
		["GPTU1_SRC0", 0xF4A000FC], 	
		["GPTU1_SRC1", 0xF4A000F8], 	
		["GPTU1_SRC2", 0xF4A000F4], 	
		["GPTU1_SRC3", 0xF4A000F0], 	
		["GPTU1_SRC4", 0xF4A000EC], 	
		["GPTU1_SRC5", 0xF4A000E8], 	
		["GPTU1_SRC6", 0xF4A000E4], 	
		["GPTU1_SRC7", 0xF4A000E0], 	
		["GPTU1_SRSEL", 0xF4A000DC], 	
		["GPTU1_T012RUN", 0xF4A00060], 	
		["GPTU1_T2RC1", 0xF4A0005C], 	
		["GPTU1_T2RC0", 0xF4A00058], 	
		["GPTU1_T2", 0xF4A00054], 	
		["GPTU1_T1RCBA", 0xF4A00050], 	
		["GPTU1_T1RDCBA", 0xF4A0004C], 	
		["GPTU1_T1CBA", 0xF4A00048], 	
		["GPTU1_T1DCBA", 0xF4A00044], 	
		["GPTU1_T0RCBA", 0xF4A00040], 	
		["GPTU1_T0RDCBA", 0xF4A0003C], 	
		["GPTU1_T0CBA", 0xF4A00038], 	
		["GPTU1_T0DCBA", 0xF4A00034], 	
		["GPTU1_OUT", 0xF4A00030], 	
		["GPTU1_OSEL", 0xF4A0002C], 	
		["GPTU1_T2ES", 0xF4A00028], 	
		["GPTU1_T2BIS", 0xF4A00024], 	
		["GPTU1_T2AIS", 0xF4A00020], 	
		["GPTU1_T2RCCON", 0xF4A0001C], 	
		["GPTU1_T2CON", 0xF4A00018], 	
		["GPTU1_T01OTS", 0xF4A00014], 	
		["GPTU1_T01IRS", 0xF4A00010], 	
		["GPTU1_ID", 0xF4A00008], 	
		["GPTU1_CLC", 0xF4A00000], 	
		
		# GPTU*
		["GPTU0_SRC0", 0xF49000FC], 	
		["GPTU0_SRC1", 0xF49000F8], 	
		["GPTU0_SRC2", 0xF49000F4], 	
		["GPTU0_SRC3", 0xF49000F0], 	
		["GPTU0_SRC4", 0xF49000EC], 	
		["GPTU0_SRC5", 0xF49000E8], 	
		["GPTU0_SRC6", 0xF49000E4], 	
		["GPTU0_SRC7", 0xF49000E0], 	
		["GPTU0_SRSEL", 0xF49000DC], 	
		["GPTU0_T012RUN", 0xF4900060], 	
		["GPTU0_T2RC1", 0xF490005C], 	
		["GPTU0_T2RC0", 0xF4900058], 	
		["GPTU0_T2", 0xF4900054], 	
		["GPTU0_T1RCBA", 0xF4900050], 	
		["GPTU0_T1RDCBA", 0xF490004C], 	
		["GPTU0_T1CBA", 0xF4900048], 	
		["GPTU0_T1DCBA", 0xF4900044], 	
		["GPTU0_T0RCBA", 0xF4900040], 	
		["GPTU0_T0RDCBA", 0xF490003C], 	
		["GPTU0_T0CBA", 0xF4900038], 	
		["GPTU0_T0DCBA", 0xF4900034], 	
		["GPTU0_OUT", 0xF4900030], 	
		["GPTU0_OSEL", 0xF490002C], 	
		["GPTU0_T2ES", 0xF4900028], 	
		["GPTU0_T2BIS", 0xF4900024], 	
		["GPTU0_T2AIS", 0xF4900020], 	
		["GPTU0_T2RCCON", 0xF490001C], 	
		["GPTU0_T2CON", 0xF4900018], 	
		["GPTU0_T01OTS", 0xF4900014], 	
		["GPTU0_T01IRS", 0xF4900010], 	
		["GPTU0_ID", 0xF4900008], 	
		["GPTU0_CLC", 0xF4900000], 	
		
		# IRQ
		["FIQ_ACK", 0xF2800008], # сюда пишется 1 в конце fiq_handler
		["IRQ_ACK", 0xF2800014], # сюда пишется 1 в конце irq_handler
		
		["FIQ_CURRENT_NUM", 0xF2800018], # тут текущий номер интеррапта в fiq_handler
		["IRQ_CURRENT_NUM", 0xF280001C], # тут текущий номер интеррапта в irq_handler
		
		["GPTU0_*", 0xF4900000, 0xF49000FF], 
		["GPTU1_*", 0xF4A00000, 0xF4A000FF], 
		
		# Контрольные регистры для каждого интеррапта
		# Значение 1 - активирует интеррапт, 0 - дактивирует
		# Значения остальных битов неизвестны
		["IRQ_000", 0xF2800030], 
		["IRQ_001", 0xF2800034], 
		["IRQ_002", 0xF2800038], 
		["IRQ_003", 0xF280003C], 
		["IRQ_004", 0xF2800040], 
		["IRQ_005", 0xF2800044], 
		["IRQ_006", 0xF2800048], 
		["IRQ_007", 0xF280004C], 
		["IRQ_008", 0xF2800050], 
		["IRQ_009", 0xF2800054], 
		["IRQ_010", 0xF2800058], 
		["IRQ_011", 0xF280005C], 
		["IRQ_012", 0xF2800060], 
		["IRQ_013", 0xF2800064], 
		["IRQ_014", 0xF2800068], 
		["IRQ_015", 0xF280006C], 
		["IRQ_016", 0xF2800070], 
		["IRQ_017", 0xF2800074], 
		["IRQ_018", 0xF2800078], 
		["IRQ_019", 0xF280007C], 
		["IRQ_020", 0xF2800080], 
		["IRQ_021", 0xF2800084], 
		["IRQ_022", 0xF2800088], 
		["IRQ_023", 0xF280008C], 
		["IRQ_024", 0xF2800090], 
		["IRQ_025", 0xF2800094], 
		["IRQ_026", 0xF2800098], 
		["IRQ_027", 0xF280009C], 
		["IRQ_028", 0xF28000A0], 
		["IRQ_029", 0xF28000A4], 
		["IRQ_030", 0xF28000A8], 
		["IRQ_031", 0xF28000AC], 
		["IRQ_032", 0xF28000B0], 
		["IRQ_033", 0xF28000B4], 
		["IRQ_034", 0xF28000B8], 
		["IRQ_035", 0xF28000BC], 
		["IRQ_036", 0xF28000C0], 
		["IRQ_037", 0xF28000C4], 
		["IRQ_038", 0xF28000C8], 
		["IRQ_039", 0xF28000CC], 
		["IRQ_040", 0xF28000D0], 
		["IRQ_041", 0xF28000D4], 
		["IRQ_042", 0xF28000D8], 
		["IRQ_043", 0xF28000DC], 
		["IRQ_044", 0xF28000E0], 
		["IRQ_045", 0xF28000E4], 
		["IRQ_046", 0xF28000E8], 
		["IRQ_047", 0xF28000EC], 
		["IRQ_048", 0xF28000F0], 
		["IRQ_049", 0xF28000F4], 
		["IRQ_050", 0xF28000F8], 
		["IRQ_051", 0xF28000FC], 
		["IRQ_052", 0xF2800100], 
		["IRQ_053", 0xF2800104], 
		["IRQ_054", 0xF2800108], 
		["IRQ_055", 0xF280010C], 
		["IRQ_056", 0xF2800110], 
		["IRQ_057", 0xF2800114], 
		["IRQ_058", 0xF2800118], 
		["IRQ_059", 0xF280011C], 
		["IRQ_060", 0xF2800120], 
		["IRQ_061", 0xF2800124], 
		["IRQ_062", 0xF2800128], 
		["IRQ_063", 0xF280012C], 
		["IRQ_064", 0xF2800130], 
		["IRQ_065", 0xF2800134], 
		["IRQ_066", 0xF2800138], 
		["IRQ_067", 0xF280013C], 
		["IRQ_068", 0xF2800140], 
		["IRQ_069", 0xF2800144], 
		["IRQ_070", 0xF2800148], 
		["IRQ_071", 0xF280014C], 
		
		# CAPCOM0
		["IRQ_072_CCU0_T0", 0xF2800150], 
		["IRQ_073_CCU0_T1", 0xF2800154], 
		["IRQ_074_CCU0_CC0", 0xF2800158], 
		["IRQ_075_CCU0_CC1", 0xF280015C], 
		["IRQ_076_CCU0_CC2", 0xF2800160], 
		["IRQ_077_CCU0_CC3", 0xF2800164], 
		["IRQ_078_CCU0_CC4", 0xF2800168], 
		["IRQ_079_CCU0_CC5", 0xF280016C], 
		["IRQ_080_CCU0_CC6", 0xF2800170], 
		["IRQ_081_CCU0_CC7", 0xF2800174], 
		
		# CAPCOM1
		["IRQ_082_CCU1_T0", 0xF2800178], 
		["IRQ_083_CCU1_T1", 0xF280017C], 
		["IRQ_084_CCU0_CC0", 0xF2800180], 
		["IRQ_085_CCU0_CC1", 0xF2800184], 
		["IRQ_086_CCU0_CC2", 0xF2800188], 
		["IRQ_087_CCU0_CC3", 0xF280018C], 
		["IRQ_088_CCU0_CC4", 0xF2800190], 
		["IRQ_089_CCU0_CC5", 0xF2800194], 
		["IRQ_090_CCU0_CC6", 0xF2800198], 
		["IRQ_091_CCU0_CC7", 0xF280019C], 
		
		["IRQ_092", 0xF28001A0], 
		["IRQ_093", 0xF28001A4], 
		["IRQ_094", 0xF28001A8], 
		["IRQ_095", 0xF28001AC], 
		["IRQ_096", 0xF28001B0], 
		["IRQ_097", 0xF28001B4], 
		["IRQ_098", 0xF28001B8], 
		["IRQ_099", 0xF28001BC], 
		["IRQ_100", 0xF28001C0], 
		["IRQ_101", 0xF28001C4], 
		["IRQ_102", 0xF28001C8], 
		["IRQ_103", 0xF28001CC], 
		["IRQ_104", 0xF28001D0], 
		["IRQ_105", 0xF28001D4], 
		["IRQ_106", 0xF28001D8], 
		["IRQ_107", 0xF28001DC], 
		["IRQ_108", 0xF28001E0], 
		["IRQ_109", 0xF28001E4], 
		["IRQ_110", 0xF28001E8], 
		["IRQ_111", 0xF28001EC], 
		["IRQ_112", 0xF28001F0], 
		["IRQ_113", 0xF28001F4], 
		["IRQ_114", 0xF28001F8], 
		["IRQ_115", 0xF28001FC], 
		["IRQ_116", 0xF2800200], 
		["IRQ_117", 0xF2800204], 
		["IRQ_118", 0xF2800208], 
		["IRQ_119", 0xF280020C], 
		["IRQ_120", 0xF2800210], 
		["IRQ_121", 0xF2800214], 
		["IRQ_122", 0xF2800218], 
		["IRQ_123", 0xF280021C], 
		["IRQ_124", 0xF2800220], 
		["IRQ_125", 0xF2800224], 
		["IRQ_126", 0xF2800228], 
		["IRQ_127", 0xF280022C], 
		["IRQ_128", 0xF2800230], 
		["IRQ_129", 0xF2800234], 
		["IRQ_130", 0xF2800238], 
		["IRQ_131", 0xF280023C], 
		["IRQ_132", 0xF2800240], 
		["IRQ_133", 0xF2800244], 
		["IRQ_134", 0xF2800248], 
		["IRQ_135", 0xF280024C], 
		["IRQ_136", 0xF2800250], 
		["IRQ_137", 0xF2800254], 
		["IRQ_138", 0xF2800258], 
		["IRQ_139", 0xF280025C], 
		["IRQ_140", 0xF2800260], 
		["IRQ_141", 0xF2800264], 
		["IRQ_142", 0xF2800268], 
		["IRQ_143", 0xF280026C], 
		["IRQ_144", 0xF2800270], 
		["IRQ_145", 0xF2800274], 
		["IRQ_146", 0xF2800278], 
		["IRQ_147", 0xF280027C], 
		["IRQ_148", 0xF2800280], 
		["IRQ_149", 0xF2800284], 
		["IRQ_150", 0xF2800288], 
		["IRQ_151", 0xF280028C], 
		["IRQ_152", 0xF2800290], 
		["IRQ_153", 0xF2800294], 
		["IRQ_154", 0xF2800298], 
		["IRQ_155", 0xF280029C], 
		["IRQ_156", 0xF28002A0], 
		["IRQ_157", 0xF28002A4], 
		["IRQ_158", 0xF28002A8], 
		
		["CAPCOM0_CLC", 0xF4000000], 
		["CAPCOM0_CCPISEL", 0xF4000004], 
		["CAPCOM0_CCID", 0xF4000008], 
		["CAPCOM0_ZERO1", 0xF400000C], 
		["CAPCOM0_T01CON", 0xF4000010], 
		["CAPCOM0_CCM0", 0xF4000014], 
		["CAPCOM0_CCM1", 0xF4000018], 
		["CAPCOM0_RESERVED1[0]", 0xF400001C], 
		["CAPCOM0_RESERVED1[1]", 0xF4000020], 
		["CAPCOM0_CCOUT", 0xF4000024], 
		["CAPCOM0_CCIOC", 0xF4000028], 
		["CAPCOM0_CCSEE", 0xF400002C], 
		["CAPCOM0_CCSEM", 0xF4000030], 
		["CAPCOM0_CCDRM", 0xF4000034], 
		["CAPCOM0_RESERVED3[0]", 0xF4000038], 
		["CAPCOM0_RESERVED3[1]", 0xF400003C], 
		["CAPCOM0_T0", 0xF4000040], 
		["CAPCOM0_T0REL", 0xF4000044], 
		["CAPCOM0_T1", 0xF4000048], 
		["CAPCOM0_T1REL", 0xF400004C], 
		["CAPCOM0_CC0", 0xF4000050], 
		["CAPCOM0_CC1", 0xF4000054], 
		["CAPCOM0_CC2", 0xF4000058], 
		["CAPCOM0_CC3", 0xF400005C], 
		["CAPCOM0_CC4", 0xF4000060], 
		["CAPCOM0_CC5", 0xF4000064], 
		["CAPCOM0_CC6", 0xF4000068], 
		["CAPCOM0_CC7", 0xF400006C], 
		["CAPCOM0_RESERVED5[0]", 0xF4000070], 
		["CAPCOM0_RESERVED5[1]", 0xF4000074], 
		["CAPCOM0_RESERVED5[2]", 0xF4000078], 
		["CAPCOM0_RESERVED5[3]", 0xF400007C], 
		["CAPCOM0_RESERVED5[4]", 0xF4000080], 
		["CAPCOM0_RESERVED5[5]", 0xF4000084], 
		["CAPCOM0_RESERVED5[6]", 0xF4000088], 
		["CAPCOM0_RESERVED5[7]", 0xF400008C], 
		["CAPCOM0_ZERO2", 0xF4000090], 
		["CAPCOM0_ZERO3", 0xF4000094], 
		["CAPCOM0_ZERO4", 0xF4000098], 
		["CAPCOM0_ZERO5", 0xF400009C], 
		["CAPCOM0_RESERVED13[0]", 0xF40000A0], 
		["CAPCOM0_RESERVED13[1]", 0xF40000A4], 
		["CAPCOM0_RESERVED13[2]", 0xF40000A8], 
		["CAPCOM0_RESERVED13[3]", 0xF40000AC], 
		["CAPCOM0_RESERVED13[4]", 0xF40000B0], 
		["CAPCOM0_RESERVED13[5]", 0xF40000B4], 
		["CAPCOM0_RESERVED13[6]", 0xF40000B8], 
		["CAPCOM0_RESERVED13[7]", 0xF40000BC], 
		["CAPCOM0_RESERVED13[8]", 0xF40000C0], 
		["CAPCOM0_RESERVED13[9]", 0xF40000C4], 
		["CAPCOM0_RESERVED13[10]", 0xF40000C8], 
		["CAPCOM0_RESERVED13[11]", 0xF40000CC], 
		["CAPCOM0_RESERVED13[12]", 0xF40000D0], 
		["CAPCOM0_RESERVED13[13]", 0xF40000D4], 
		["CAPCOM0_CC7IC", 0xF40000D8], 
		["CAPCOM0_CC6IC", 0xF40000DC], 
		["CAPCOM0_CC5IC", 0xF40000E0], 
		["CAPCOM0_CC4IC", 0xF40000E4], 
		["CAPCOM0_CC3IC", 0xF40000E8], 
		["CAPCOM0_CC2IC", 0xF40000EC], 
		["CAPCOM0_CC1IC", 0xF40000F0], 
		["CAPCOM0_CC0IC", 0xF40000F4], 
		["CAPCOM0_T1IC", 0xF40000F8], 
		["CAPCOM0_T0IC", 0xF40000FC], 
		
		["CAPCOM1_CLC", 0xF4000000], 
		["CAPCOM1_CCPISEL", 0xF4000004], 
		["CAPCOM1_CCID", 0xF4000008], 
		["CAPCOM1_ZERO1", 0xF400000C], 
		["CAPCOM1_T01CON", 0xF4000010], 
		["CAPCOM1_CCM0", 0xF4000014], 
		["CAPCOM1_CCM1", 0xF4000018], 
		["CAPCOM1_RESERVED1[0]", 0xF400001C], 
		["CAPCOM1_RESERVED1[1]", 0xF4000020], 
		["CAPCOM1_CCOUT", 0xF4000024], 
		["CAPCOM1_CCIOC", 0xF4000028], 
		["CAPCOM1_CCSEE", 0xF400002C], 
		["CAPCOM1_CCSEM", 0xF4000030], 
		["CAPCOM1_CCDRM", 0xF4000034], 
		["CAPCOM1_RESERVED3[0]", 0xF4000038], 
		["CAPCOM1_RESERVED3[1]", 0xF400003C], 
		["CAPCOM1_T0", 0xF4000040], 
		["CAPCOM1_T0REL", 0xF4000044], 
		["CAPCOM1_T1", 0xF4000048], 
		["CAPCOM1_T1REL", 0xF400004C], 
		["CAPCOM1_CC0", 0xF4000050], 
		["CAPCOM1_CC1", 0xF4000054], 
		["CAPCOM1_CC2", 0xF4000058], 
		["CAPCOM1_CC3", 0xF400005C], 
		["CAPCOM1_CC4", 0xF4000060], 
		["CAPCOM1_CC5", 0xF4000064], 
		["CAPCOM1_CC6", 0xF4000068], 
		["CAPCOM1_CC7", 0xF400006C], 
		["CAPCOM1_RESERVED5[0]", 0xF4000070], 
		["CAPCOM1_RESERVED5[1]", 0xF4000074], 
		["CAPCOM1_RESERVED5[2]", 0xF4000078], 
		["CAPCOM1_RESERVED5[3]", 0xF400007C], 
		["CAPCOM1_RESERVED5[4]", 0xF4000080], 
		["CAPCOM1_RESERVED5[5]", 0xF4000084], 
		["CAPCOM1_RESERVED5[6]", 0xF4000088], 
		["CAPCOM1_RESERVED5[7]", 0xF400008C], 
		["CAPCOM1_ZERO2", 0xF4000090], 
		["CAPCOM1_ZERO3", 0xF4000094], 
		["CAPCOM1_ZERO4", 0xF4000098], 
		["CAPCOM1_ZERO5", 0xF400009C], 
		["CAPCOM1_RESERVED13[0]", 0xF40000A0], 
		["CAPCOM1_RESERVED13[1]", 0xF40000A4], 
		["CAPCOM1_RESERVED13[2]", 0xF40000A8], 
		["CAPCOM1_RESERVED13[3]", 0xF40000AC], 
		["CAPCOM1_RESERVED13[4]", 0xF40000B0], 
		["CAPCOM1_RESERVED13[5]", 0xF40000B4], 
		["CAPCOM1_RESERVED13[6]", 0xF40000B8], 
		["CAPCOM1_RESERVED13[7]", 0xF40000BC], 
		["CAPCOM1_RESERVED13[8]", 0xF40000C0], 
		["CAPCOM1_RESERVED13[9]", 0xF40000C4], 
		["CAPCOM1_RESERVED13[10]", 0xF40000C8], 
		["CAPCOM1_RESERVED13[11]", 0xF40000CC], 
		["CAPCOM1_RESERVED13[12]", 0xF40000D0], 
		["CAPCOM1_RESERVED13[13]", 0xF40000D4], 
		["CAPCOM1_CC7IC", 0xF40000D8], 
		["CAPCOM1_CC6IC", 0xF40000DC], 
		["CAPCOM1_CC5IC", 0xF40000E0], 
		["CAPCOM1_CC4IC", 0xF40000E4], 
		["CAPCOM1_CC3IC", 0xF40000E8], 
		["CAPCOM1_CC2IC", 0xF40000EC], 
		["CAPCOM1_CC1IC", 0xF40000F0], 
		["CAPCOM1_CC0IC", 0xF40000F4], 
		["CAPCOM1_T1IC", 0xF40000F8], 
		["CAPCOM1_T0IC", 0xF40000FC], 
	];
	
	my $add = "";
	if ($addr >= 0xF4300020 && $addr <= 0xF43001E4) {
		my $IS		= $value & 0x7;
		my $OS		= ($value >> 3) & 0x7;
		my $PS		= ($value >> 7) & 1;
		my $DATA	= ($value >> 8) & 1;
		my $DIR		= ($value >> 9) & 1;
		my $PPEN	= ($value >> 11) & 1;
		my $PDPU	= ($value >> 12) & 3;
		my $ENAQ	= ($value >> 14) & 1;
		
		my @gpio = ();
		push @gpio, "PS=$PS";
		if ($IS) {
			push @gpio, "IS=ALT".($IS - 1);
		}
		if ($OS) {
			push @gpio, "OS=ALT".($OS - 1);
		}
		push @gpio, "DATA=$DATA";
		push @gpio, "DIR=".(!$DIR ? "IN" : "OUT");
		if ($PPEN) {
			push @gpio, "PPEN=1";
		}
		if ($PDPU) {
			push @gpio, "PDPU=".($PDPU == 2 ? "DOWN" : ($PDPU == 1 ? "UP" : "$PDPU"));
		}
		if ($ENAQ) {
			push @gpio, "ENAQ=1";
		}
		$add = " [".join("; ", @gpio)."]";
	}
	
	my $def = "";
	for my $v (@$names) {
		if ($v->[1] == $addr && !$v->[2]) {
			return " (".$v->[0].")".$add;
		} elsif ($v->[2] && $v->[1] <= $addr && $addr <= $v->[2]) {
			$def = " (".$v->[0].")".$add;
		}
	}
	return $def;
}

sub cmd_write {
	my ($port, $addr, $value) = @_;
	$port->write("W".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF).
		chr(($value >> 24) & 0xFF).chr(($value >> 16) & 0xFF).chr(($value >> 8) & 0xFF).chr($value & 0xFF));
	my ($count, $ack) = $port->read(1);
	if ($count != 1) {
		die "Transfer error ($count != 1)";
	} else {
		if ($ack ne ";") {
			die "invalid ack = ".sprintf("%02X", ord($ack))."\n";
		}
	}
}

sub cmd_read {
	my ($port, $addr, $raw) = @_;
	$port->write("R".chr(($addr >> 24) & 0xFF).chr(($addr >> 16) & 0xFF).chr(($addr >> 8) & 0xFF).chr($addr & 0xFF));
	my ($count, $data) = $port->read(5);
	if ($count != 5) {
		printf("%02X\n", ord(substr($data, 0, 1)));
		die "Transfer error ($count != 5)";
	} else {
		my ($buf, $ack) = (substr($data, 0, 4), substr($data, 4, 1));
		if ($ack eq ";") {
			return $raw ? $buf : unpack("V", $buf);
		} else {
			die "invalid ack = ".sprintf("%02X", ord($ack))."\n";
		}
	}
}

sub cmd_ping {
	my ($port) = @_;
	$port->write(".");
	my $c = readb($port);
	die "PING ERROR (c=".sprintf("%02X (%c)", $c, $c & 0xFF).")" if ($c != 0x2E);
}

sub readb {
	my ($port) = @_;
	my ($count, $char) = $port->read(1);
	return ord($char) if ($count);
	return -1;
}
1;