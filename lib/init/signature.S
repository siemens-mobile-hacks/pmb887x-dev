#include <gen/board.h>

#if defined(SECURE_BOOT) && !defined(BOOT_FLASH) && defined(BOARD_PLATFORM_APOXI)
.global signature_data
.global signature_r2
.global signature_rsa
.global signature_end
.section .signature, "a"

signature_data:
	.int	1
#if defined(PMB8875)
	.int	0x1A			@ PMB8875
#elif defined(PMB8876)
	.int	0x1B			@ PMB8876
#else
#error Unsupported secure boot CPU
#endif
	.int	0x801			@ CHIP_UID1 derived value
	.int	0xA01			@ GPIO102+32+33 derived value
	.int	2048			@ RSA bit length
	.int	3				@ RSA exponent
	.fill	256, 1, 0x67	@ Signature hash padding
	.fill	256, 1, 0x42	@ Metadata signature
signature_r2:
	.fill	256, 1, 0x22	@ Montgomery R^2 constant
signature_rsa:
	.fill	256, 1, 0x69	@ Signature
signature_end:
#endif
