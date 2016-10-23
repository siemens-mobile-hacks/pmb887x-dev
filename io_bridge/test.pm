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
		# USART
		["USART0_*", 0xf1000000, 0xf1000100], 
		
		["USART0_CLC", 0xf1000000], 
		["USART0_BG", 0xf1000014], 
		["USART0_FDV", 0xf1000018], 
		["USART0_TXB", 0xf1000020], 
		["USART0_RXB", 0xf1000024], 
		["USART0_FCSTAT", 0xf1000068], 
		["USART0_ICR", 0xf1000070], 
		
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
		
		["GPTU0_*", 0xF4900000, 0xF49000FF], 
		["GPTU1_*", 0xF4A00000, 0xF4A000FF], 
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