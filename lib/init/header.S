#include <gen/board.h>

.global start
.arm
.section .boot.header, "ax"

#define BSL_PAYLOAD_ADDR	0x00082000

#if defined(BOOT_FLASH)
	# Boot header (64 bytes)
	.int	0xFFFFFFFF
	.int	0xFFFFFFFF
	.int	0xFFFFFFFF
	.int	0xFFFFFFFF
	.int	0xFFFFFFFF
	.int	0xFFFFFFFF
	.int	0x00000000
	.int	0x00000000
	.int	0x00000000
	.int	0x00000000
	.int	0x00000000
	.int	0x00000000
	.int	BSL_PAYLOAD_ADDR
	.int	~BSL_PAYLOAD_ADDR
	.int	start
	.ascii	"CJKT"
#elif defined(SECURE_BOOT) && defined(BOARD_PLATFORM_APOXI)
start:
	# Secure boot header (64 bytes)
	b	boot_entry
	.ascii	"liHU"
	.int	0x102					@ Unknown purpose, seems to not matter
	.int	0x0F0F0F0F				@ Lock word, must not be 0 or 0xFFFFFFFF
	.int	signature_end - start	@ Total signed size
	.int	signature_data - start	@ Offset to metadata
	.int	0						@ Entry point relative to flash base
	.int	0x400					@ Unknown purpose, seems to not matter
	.int	signature_r2 - start	@ Offset to Montgomery R^2 constant
	.int	0xFFFFFFFF				@ Unused
	.int	signature_rsa - start	@ Offset to signature
	.int	0xFFFFFFFF				@ Unused
	.int	0x22000					@ Old BootROM payload validator address
	.int	~0x22000				@ Checksum for the above
	.int	0						@ Old BootROM entry point
	.ascii	"CJKT"
#elif defined(SECURE_BOOT) && defined(BOARD_PLATFORM_SIEMENS)
start:
	# Siemens secure boot header (96 bytes)
	b	boot_entry
	.int	0, 0, 0, 0, 0, 0, 0, 0, 0
	.ascii	"SIEMENS_BOOTCODE"
	.int	3, 0
	.int	0, 0, 0, 0				@ Boot key
	.int	0, 0, 0, 0
#else
	b	boot_entry
#endif

#if !defined(BOOT_FLASH)
# --------------------------------------------------
.Lpreloader_header:
#if defined(PMB8875)
	.ascii	"PMB8875"
#elif defined(PMB8876)
	.ascii	"PMB8876"
#else
#error Unsupported CPU
#endif

#if defined(BOARD_PLATFORM_SIEMENS)
	.byte	0
#elif defined(BOARD_PLATFORM_APOXI)
	.byte	1
#else
#error Unsupported platform
#endif

#if defined(BOOT_FLASH)
	.int	0xA0000000
#elif defined(BOOT_EXTRAM)
	.int	0xB0000000
#elif defined(BOOT_INTRAM) || defined(BOOT_TCM)
	.int	0x00082000
#else
#error Unsupported boot mode
#endif

.Lpreloader_board_name:
	.ascii	BOARD_NAME
	.fill	20 - (. - .Lpreloader_board_name), 1, 0
.Lpreloader_header_end:
	.if	.Lpreloader_header_end - .Lpreloader_header != 32
	.error	"invalid preloader header size"
	.endif
# --------------------------------------------------
#endif
