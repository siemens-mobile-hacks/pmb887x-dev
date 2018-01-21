#pragma once

#include <stdint.h>

#define	MCI0_BASE	(0xF7200008)
#define	MCI0	((volatile MCI_TypeDef *) MCI0_BASE)

typedef struct {
	union {
		struct {
			uint32_t MOD_REV:8; /* [0..8] */
			uint32_t MOD_32B:8; /* [8..16] */
			uint32_t MOD_NUMBER:16; /* [16..32] */
		} b;
		uint32_t v;
	} ID; /* F7200008 */

	uint32_t _RESERVED0[262141];
	union {
		struct {
			uint32_t DISR:1; /* [0..1] */
			uint32_t DISS:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t RMC:7; /* [8..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CLC; /* F7300000 */

	uint32_t _RESERVED1[1023];
	union {
		struct {
			uint32_t CTRL:2; /* [0..2] */
			uint32_t VOLTAGE:4; /* [2..6] */
			uint32_t OPENDRAIN:1; /* [6..7] */
			uint32_t ROD:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} POWER; /* F7301000 */

	union {
		struct {
			uint32_t CLKDIV:8; /* [0..8] */
			uint32_t ENABLE:1; /* [8..9] */
			uint32_t PWRSAVE:1; /* [9..10] */
			uint32_t BYPASS:1; /* [10..11] */
			uint32_t WIDEBUS:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CLOCK; /* F7301004 */

	union {
		struct {
			uint32_t CMDARG:32; /* [0..32] */
		} b;
		uint32_t v;
	} ARGUMENT; /* F7301008 */

	union {
		struct {
			uint32_t CMDINDEX:6; /* [0..6] */
			uint32_t RESPONSE:1; /* [6..7] */
			uint32_t LONGRSP:1; /* [7..8] */
			uint32_t INTERRUPT:1; /* [8..9] */
			uint32_t PENDING:1; /* [9..10] */
			uint32_t ENABLE:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} COMMAND; /* F730100C */

	union {
		struct {
			uint32_t CMDINDEX:6; /* [0..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RESPCMD; /* F7301010 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RESPONSE0; /* F7301014 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RESPONSE1; /* F7301018 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RESPONSE2; /* F730101C */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RESPONSE3; /* F7301020 */

	union {
		struct {
			uint32_t TIMER:32; /* [0..32] */
		} b;
		uint32_t v;
	} DATATIMER; /* F7301024 */

	union {
		struct {
			uint32_t LENGTH:16; /* [0..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} DATALENGTH; /* F7301028 */

	union {
		struct {
			uint32_t EMABLE:1; /* [0..1] */
			uint32_t DIRECTION:1; /* [1..2] */
			uint32_t MODE:1; /* [2..3] */
			uint32_t DMAENABLE:1; /* [3..4] */
			uint32_t BLOCKSIZE:4; /* [4..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} DATACTRL; /* F730102C */

	union {
		struct {
			uint32_t COUNT:16; /* [0..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} DATACNT; /* F7301030 */

	union {
		struct {
			uint32_t CMDCRCFAIL:1; /* [0..1] */
			uint32_t DATACRCFAIL:1; /* [1..2] */
			uint32_t CMDTIMEOUT:1; /* [2..3] */
			uint32_t DATATIMEOUT:1; /* [3..4] */
			uint32_t TXUNDERRUN:1; /* [4..5] */
			uint32_t RXOVERRUN:1; /* [5..6] */
			uint32_t CMDRESPEND:1; /* [6..7] */
			uint32_t CMDSENT:1; /* [7..8] */
			uint32_t DATAEND:1; /* [8..9] */
			uint32_t STARTBITERR:1; /* [9..10] */
			uint32_t DATABLOCKEND:1; /* [10..11] */
			uint32_t CMDACTIVE:1; /* [11..12] */
			uint32_t TXACTIVE:1; /* [12..13] */
			uint32_t RXACTIVE:1; /* [13..14] */
			uint32_t TXFIFOHALFEMPTY:1; /* [14..15] */
			uint32_t RXFIFOHALFFULL:1; /* [15..16] */
			uint32_t TXFIFOFULL:1; /* [16..17] */
			uint32_t RXFIFOFULL:1; /* [17..18] */
			uint32_t TXFIFOEMPTY:1; /* [18..19] */
			uint32_t RXFIFOEMPTY:1; /* [19..20] */
			uint32_t TXDATAAVLBL:1; /* [20..21] */
			uint32_t RXDATAAVLBL:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} STATUS; /* F7301034 */

	union {
		struct {
			uint32_t CMDCRCFAILCLR:1; /* [0..1] */
			uint32_t DATACRCFAILCLR:1; /* [1..2] */
			uint32_t CMDTIMEOUTCLR:1; /* [2..3] */
			uint32_t DATATIMEOUTCLR:1; /* [3..4] */
			uint32_t TXUNDERRUNCLR:1; /* [4..5] */
			uint32_t RXOVERRUNCLR:1; /* [5..6] */
			uint32_t CMDRESPENDCLR:1; /* [6..7] */
			uint32_t CMDSENTCLR:1; /* [7..8] */
			uint32_t DATAENDCLR:1; /* [8..9] */
			uint32_t STARTBITERRCLR:1; /* [9..10] */
			uint32_t DATABLOCKENDCLR:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CLEAR; /* F7301038 */

	union {
		struct {
			uint32_t CMDCRCFAILMASK:1; /* [0..1] */
			uint32_t DATACRCFAILMASK:1; /* [1..2] */
			uint32_t CMDTIMEOUTMASK:1; /* [2..3] */
			uint32_t DATATIMEOUTMASK:1; /* [3..4] */
			uint32_t TXUNDERRUNMASK:1; /* [4..5] */
			uint32_t RXOVERRUNMASK:1; /* [5..6] */
			uint32_t CMDRESPENDMASK:1; /* [6..7] */
			uint32_t CMDSENTMASK:1; /* [7..8] */
			uint32_t DATAENDMASK:1; /* [8..9] */
			uint32_t STARTBITERRMASK:1; /* [9..10] */
			uint32_t DATABLOCKENDMASK:1; /* [10..11] */
			uint32_t CMDACTIVEMASK:1; /* [11..12] */
			uint32_t TXACTIVEMASK:1; /* [12..13] */
			uint32_t RXACTIVEMASK:1; /* [13..14] */
			uint32_t TXFIFOHALFEMPTYMASK:1; /* [14..15] */
			uint32_t RXFIFOHALFFULLMASK:1; /* [15..16] */
			uint32_t TXFIFOFULLMASK:1; /* [16..17] */
			uint32_t RXFIFOFULLMASK:1; /* [17..18] */
			uint32_t TXFIFOEMPTYMASK:1; /* [18..19] */
			uint32_t RXFIFOEMPTYMASK:1; /* [19..20] */
			uint32_t TXDATAAVLBLMASK:1; /* [20..21] */
			uint32_t RXDATAAVLBLMASK:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} MASK0; /* F730103C */

	union {
		struct {
			uint32_t CMDCRCFAILMASK:1; /* [0..1] */
			uint32_t DATACRCFAILMASK:1; /* [1..2] */
			uint32_t CMDTIMEOUTMASK:1; /* [2..3] */
			uint32_t DATATIMEOUTMASK:1; /* [3..4] */
			uint32_t TXUNDERRUNMASK:1; /* [4..5] */
			uint32_t RXOVERRUNMASK:1; /* [5..6] */
			uint32_t CMDRESPENDMASK:1; /* [6..7] */
			uint32_t CMDSENTMASK:1; /* [7..8] */
			uint32_t DATAENDMASK:1; /* [8..9] */
			uint32_t STARTBITERRMASK:1; /* [9..10] */
			uint32_t DATABLOCKENDMASK:1; /* [10..11] */
			uint32_t CMDACTIVEMASK:1; /* [11..12] */
			uint32_t TXACTIVEMASK:1; /* [12..13] */
			uint32_t RXACTIVEMASK:1; /* [13..14] */
			uint32_t TXFIFOHALFEMPTYMASK:1; /* [14..15] */
			uint32_t RXFIFOHALFFULLMASK:1; /* [15..16] */
			uint32_t TXFIFOFULLMASK:1; /* [16..17] */
			uint32_t RXFIFOFULLMASK:1; /* [17..18] */
			uint32_t TXFIFOEMPTYMASK:1; /* [18..19] */
			uint32_t RXFIFOEMPTYMASK:1; /* [19..20] */
			uint32_t TXDATAAVLBLMASK:1; /* [20..21] */
			uint32_t RXDATAAVLBLMASK:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} MASK1; /* F7301040 */

	union {
		struct {
			uint32_t SDCARD:4; /* [0..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SELECT; /* F7301044 */

	union {
		struct {
			uint32_t COUNT:16; /* [0..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} FIFOCNT; /* F7301048 */

	uint32_t _RESERVED2[13];
	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} FIFO[16]; /* F7301080 */

	uint32_t _RESERVED3[968];
	union {
		struct {
			uint32_t PARTNUMBER0:8; /* [0..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID0; /* F7301FE0 */

	union {
		struct {
			uint32_t PARTNUMBER1:4; /* [0..4] */
			uint32_t DESIGNER0:8; /* [4..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID1; /* F7301FE4 */

	union {
		struct {
			uint32_t DESIGNER1:4; /* [0..4] */
			uint32_t REVISION:8; /* [4..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID2; /* F7301FE8 */

	union {
		struct {
			uint32_t CONFIGURATION:8; /* [0..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID3; /* F7301FEC */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID0; /* F7301FF0 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID1; /* F7301FF4 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID2; /* F7301FF8 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID3; /* F7301FFC */

} __attribute__((aligned(4))) MCI_TypeDef;

#define	STM_BASE	(0xF4B00000)
#define	STM	((volatile STM_TypeDef *) STM_BASE)

typedef struct {
	union {
		struct {
			uint32_t DISR:1; /* [0..1] */
			uint32_t DISS:1; /* [1..2] */
			uint32_t SPEN:1; /* [2..3] */
			uint32_t EDIS:1; /* [3..4] */
			uint32_t SBWE:1; /* [4..5] */
			uint32_t FSOE:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t RMC:3; /* [8..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CLC; /* F4B00000 */

	uint32_t _RESERVED0[1];
	union {
		struct {
			uint32_t REV:8; /* [0..8] */
			uint32_t MOD_32B:8; /* [8..16] */
			uint32_t MOD:16; /* [16..32] */
		} b;
		uint32_t v;
	} ID; /* F4B00008 */

	uint32_t _RESERVED1[1];
	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} TIM[7]; /* F4B00010 */

} __attribute__((aligned(4))) STM_TypeDef;

#define	DMA0_BASE	(0xF3000000)
#define	DMA0	((volatile DMA_TypeDef *) DMA0_BASE)

typedef struct {
	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} INT_STATUS; /* F3000000 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} TC_STATUS; /* F3000004 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} TC_CLEAR; /* F3000008 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} ERR_STATUS; /* F300000C */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} ERR_CLEAR; /* F3000010 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RAW_TC_STATUS; /* F3000014 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} RAW_ERR_CLEAR; /* F3000018 */

	union {
		struct {
			uint32_t CH0:1; /* [0..1] */
			uint32_t CH1:1; /* [1..2] */
			uint32_t CH2:1; /* [2..3] */
			uint32_t CH3:1; /* [3..4] */
			uint32_t CH4:1; /* [4..5] */
			uint32_t CH5:1; /* [5..6] */
			uint32_t CH6:1; /* [6..7] */
			uint32_t CH7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} EN_CHAN; /* F300001C */

	union {
		struct {
			uint32_t CH0_0:1; /* [0..1] */
			uint32_t CH0_1:1; /* [1..2] */
			uint32_t CH1_0:1; /* [2..3] */
			uint32_t CH1_1:1; /* [3..4] */
			uint32_t CH2_0:1; /* [4..5] */
			uint32_t CH2_1:1; /* [5..6] */
			uint32_t CH3_0:1; /* [6..7] */
			uint32_t CH3_1:1; /* [7..8] */
			uint32_t CH4_0:1; /* [8..9] */
			uint32_t CH4_1:1; /* [9..10] */
			uint32_t CH5_0:1; /* [10..11] */
			uint32_t CH5_1:1; /* [11..12] */
			uint32_t CH6_0:1; /* [12..13] */
			uint32_t CH6_1:1; /* [13..14] */
			uint32_t CH7_0:1; /* [14..15] */
			uint32_t CH7_1:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SOFT_BREQ; /* F3000020 */

	union {
		struct {
			uint32_t CH0_0:1; /* [0..1] */
			uint32_t CH0_1:1; /* [1..2] */
			uint32_t CH1_0:1; /* [2..3] */
			uint32_t CH1_1:1; /* [3..4] */
			uint32_t CH2_0:1; /* [4..5] */
			uint32_t CH2_1:1; /* [5..6] */
			uint32_t CH3_0:1; /* [6..7] */
			uint32_t CH3_1:1; /* [7..8] */
			uint32_t CH4_0:1; /* [8..9] */
			uint32_t CH4_1:1; /* [9..10] */
			uint32_t CH5_0:1; /* [10..11] */
			uint32_t CH5_1:1; /* [11..12] */
			uint32_t CH6_0:1; /* [12..13] */
			uint32_t CH6_1:1; /* [13..14] */
			uint32_t CH7_0:1; /* [14..15] */
			uint32_t CH7_1:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SOFT_SREQ; /* F3000024 */

	union {
		struct {
			uint32_t CH0_0:1; /* [0..1] */
			uint32_t CH0_1:1; /* [1..2] */
			uint32_t CH1_0:1; /* [2..3] */
			uint32_t CH1_1:1; /* [3..4] */
			uint32_t CH2_0:1; /* [4..5] */
			uint32_t CH2_1:1; /* [5..6] */
			uint32_t CH3_0:1; /* [6..7] */
			uint32_t CH3_1:1; /* [7..8] */
			uint32_t CH4_0:1; /* [8..9] */
			uint32_t CH4_1:1; /* [9..10] */
			uint32_t CH5_0:1; /* [10..11] */
			uint32_t CH5_1:1; /* [11..12] */
			uint32_t CH6_0:1; /* [12..13] */
			uint32_t CH6_1:1; /* [13..14] */
			uint32_t CH7_0:1; /* [14..15] */
			uint32_t CH7_1:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SOFT_LBREQ; /* F3000028 */

	union {
		struct {
			uint32_t CH0_0:1; /* [0..1] */
			uint32_t CH0_1:1; /* [1..2] */
			uint32_t CH1_0:1; /* [2..3] */
			uint32_t CH1_1:1; /* [3..4] */
			uint32_t CH2_0:1; /* [4..5] */
			uint32_t CH2_1:1; /* [5..6] */
			uint32_t CH3_0:1; /* [6..7] */
			uint32_t CH3_1:1; /* [7..8] */
			uint32_t CH4_0:1; /* [8..9] */
			uint32_t CH4_1:1; /* [9..10] */
			uint32_t CH5_0:1; /* [10..11] */
			uint32_t CH5_1:1; /* [11..12] */
			uint32_t CH6_0:1; /* [12..13] */
			uint32_t CH6_1:1; /* [13..14] */
			uint32_t CH7_0:1; /* [14..15] */
			uint32_t CH7_1:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SOFT_LSREQ; /* F300002C */

	union {
		struct {
			uint32_t ENABLE:1; /* [0..1] */
			uint32_t M1:1; /* [1..2] */
			uint32_t M2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CONFIG; /* F3000030 */

	union {
		struct {
			uint32_t CH0_0:1; /* [0..1] */
			uint32_t CH0_1:1; /* [1..2] */
			uint32_t CH1_0:1; /* [2..3] */
			uint32_t CH1_1:1; /* [3..4] */
			uint32_t CH2_0:1; /* [4..5] */
			uint32_t CH2_1:1; /* [5..6] */
			uint32_t CH3_0:1; /* [6..7] */
			uint32_t CH3_1:1; /* [7..8] */
			uint32_t CH4_0:1; /* [8..9] */
			uint32_t CH4_1:1; /* [9..10] */
			uint32_t CH5_0:1; /* [10..11] */
			uint32_t CH5_1:1; /* [11..12] */
			uint32_t CH6_0:1; /* [12..13] */
			uint32_t CH6_1:1; /* [13..14] */
			uint32_t CH7_0:1; /* [14..15] */
			uint32_t CH7_1:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} SYNC; /* F3000034 */

	uint32_t _RESERVED0[50];
	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CH_SRC_ADDR[8]; /* F3000100 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CH_DST_ADDR[8]; /* F3000104 */

	union {
		struct {
			uint32_t LM:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t LLI:29; /* [2..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CH_LLI[8]; /* F3000108 */

	union {
		struct {
			uint32_t TRANSFER_SZIE:12; /* [0..12] */
			uint32_t SB_SIZE:3; /* [12..15] */
			uint32_t DB_SIZE:3; /* [15..18] */
			uint32_t S_WIDTH:3; /* [18..21] */
			uint32_t D_WIDTH:3; /* [21..24] */
			uint32_t S:1; /* [24..25] */
			uint32_t D:1; /* [25..26] */
			uint32_t SI:1; /* [26..27] */
			uint32_t DI:1; /* [27..28] */
			uint32_t PROTECTION:3; /* [28..31] */
			uint32_t I:1; /* [31..32] */
		} b;
		uint32_t v;
	} CH_CONTROL[8]; /* F300010C */

	union {
		struct {
			uint32_t ENABLE:1; /* [0..1] */
			uint32_t SRC_PERIPH:4; /* [1..5] */
			uint32_t DST_PERIPH:6; /* [5..11] */
			uint32_t FLOW_CTRL:3; /* [11..14] */
			uint32_t INT_MASK_ERR:1; /* [14..15] */
			uint32_t INT_MASK_TC:1; /* [15..16] */
			uint32_t LOCK:1; /* [16..17] */
			uint32_t ACTIVE:1; /* [17..18] */
			uint32_t HALT:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} CH_CONFIG[8]; /* F3000110 */

	uint32_t _RESERVED1[940];
	union {
		struct {
			uint32_t PARTNUMBER0:8; /* [0..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID0; /* F3000FE0 */

	union {
		struct {
			uint32_t PARTNUMBER1:4; /* [0..4] */
			uint32_t DESIGNER0:8; /* [4..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID1; /* F3000FE4 */

	union {
		struct {
			uint32_t DESIGNER1:4; /* [0..4] */
			uint32_t REVISION:8; /* [4..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID2; /* F3000FE8 */

	union {
		struct {
			uint32_t CONFIGURATION:8; /* [0..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PERIPH_ID3; /* F3000FEC */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID0; /* F3000FF0 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID1; /* F3000FF4 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID2; /* F3000FF8 */

	union {
		struct {
			uint32_t _bit0:1; /* [0..1] */
			uint32_t _bit1:1; /* [1..2] */
			uint32_t _bit2:1; /* [2..3] */
			uint32_t _bit3:1; /* [3..4] */
			uint32_t _bit4:1; /* [4..5] */
			uint32_t _bit5:1; /* [5..6] */
			uint32_t _bit6:1; /* [6..7] */
			uint32_t _bit7:1; /* [7..8] */
			uint32_t _bit8:1; /* [8..9] */
			uint32_t _bit9:1; /* [9..10] */
			uint32_t _bit10:1; /* [10..11] */
			uint32_t _bit11:1; /* [11..12] */
			uint32_t _bit12:1; /* [12..13] */
			uint32_t _bit13:1; /* [13..14] */
			uint32_t _bit14:1; /* [14..15] */
			uint32_t _bit15:1; /* [15..16] */
			uint32_t _bit16:1; /* [16..17] */
			uint32_t _bit17:1; /* [17..18] */
			uint32_t _bit18:1; /* [18..19] */
			uint32_t _bit19:1; /* [19..20] */
			uint32_t _bit20:1; /* [20..21] */
			uint32_t _bit21:1; /* [21..22] */
			uint32_t _bit22:1; /* [22..23] */
			uint32_t _bit23:1; /* [23..24] */
			uint32_t _bit24:1; /* [24..25] */
			uint32_t _bit25:1; /* [25..26] */
			uint32_t _bit26:1; /* [26..27] */
			uint32_t _bit27:1; /* [27..28] */
			uint32_t _bit28:1; /* [28..29] */
			uint32_t _bit29:1; /* [29..30] */
			uint32_t _bit30:1; /* [30..31] */
			uint32_t _bit31:1; /* [31..32] */
		} b;
		uint32_t v;
	} PCELL_ID3; /* F3000FFC */

} __attribute__((aligned(4))) DMA_TypeDef;

