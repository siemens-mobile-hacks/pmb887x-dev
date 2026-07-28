# DSP

## Firmware

The DSP uses both the mask firmware in Program ROM and startup firmware loaded
by the ARM into DSP Program/Data RAM. A boot stream consists of these records:

```text
PLOAD  destination, length, program words...
DLOAD  destination, length, data words...
BRANCH entry point
```

SL98 contains two boot streams for different DSP mask firmware versions:

- `0xA073A3C2`, mask firmware `0x0801`: 7509 program words, 769 data words,
  entry point `0x0020`;
- `0xA073E502`, mask firmware `0x0800`: 4551 program words, 74 data words,
  entry point `0x0020`.

Extract one container from a raw ARM firmware image with:

```sh
perl tools/extract_sl98_dsp.pl \
  --address 0xA073A3C2 \
  --out /tmp/sl98-dsp-fw-0801 \
  /path/to/SL98.bin
```

The script writes the original `container.bin`, assembled `pram.bin` and
`dram.bin` images, separate payload files for all load records, and `map.tsv`.
It extracts only the code and data loaded by the ARM, not the factory mask ROM.
This `container.bin` is an ARM-side `PLOAD`/`DLOAD`/`BRANCH` stream, not a DSP1
file. Output from `makedsp1` must first be parsed as a DSP1 header, segment
table, and segment payloads; it is not raw instruction bytecode.

Raw ROM images kept in the repository are:

- `bsp/rom/dsp/0602-program-rom.bin` and `0602-data-rom.bin`: Mask ROM
  `0x0602` from PMB8875;
- `bsp/rom/dsp/0801-program-rom.bin` and `0801-data-rom.bin`: Mask ROM
  `0x0801` from PMB8876;
- `bsp/rom/dsp/sl98/0800.bin` and `0801.bin`: sparse P-space images assembled from
  the two SL98 startup-firmware `PLOAD` streams. They are raw DSP user program
  images and do not include `DLOAD` payloads;
- `bsp/rom/dsp/sl98/0800-container.bin` and `0801-container.bin`: the corresponding
  original ARM-side `PLOAD`/`DLOAD`/`BRANCH` streams.

## Address spaces

TEAKLite uses a Harvard architecture: P-space and D-space are separate address
spaces. Identical numerical addresses such as `P:0020` and `D:0020` identify
different physical words. DSP addresses are measured in 16-bit words, while
ARM addresses and file offsets are measured in bytes.

### PMB8875 and PMB8876 differences

Addresses in this table are DSP 16-bit word addresses. The Mask IDs are the
revisions verified on hardware, not phone or startup-firmware identifiers.

| Property | PMB8875 / Mask ID `0602` | PMB8876 / Mask ID `0801` |
|---|---|---|
| Program Mask ROM | 80K words | 104K words |
| Program RAM | 4K words | 8K words |
| Data Mask ROM | 48K words | 60K words |
| Data RAM | 27K words: 24K XRAM + 3K YRAM | 37K words: 32K XRAM + 5K YRAM |
| Shared RAM | 1.5K x 16-bit words | 1.5K x 32-bit words, seen as 3K x 16-bit words by the DSP |
| Incremental Redundancy memory | Not specified in the inspected memory summary | 35904 x 16-bit words |
| Program RAM | `P:0000..0FFF` | `P:0000..1FFF` |
| Fixed Program ROM | `P:1000..AFFF`, 40K words | `P:2000..9FFF`, 32K words |
| Banked Program ROM window | `P:B000..FFFF`, 20K words, two banks | `P:A000..FFFF`, 24K words, three banks |
| Fixed Data ROM | `D:6000..8FFF`, 12K words | `D:8000..8FFF`, 4K words |
| Banked Data ROM window | `D:9000..D7FF`, 18K words, two banks | `D:9000..CFFF`, 16K-word window, four selectable views |
| Shared RAM in D-space | `D:D800..DDFF` | `D:D000..DBFF` |
| DSP register page | `D:E600..E6FF` | Starts at `D:DE00` |
| `DSP_PAGE` | `D:E6A3`; `DATA_PAGE` bit 0, `PROG_PAGE` bit 1 | `D:DEA3`; `DATA_PAGE` bits 1:0, `PROG_PAGE` bits 3:2 |
| Pipe interrupt request | `INT_EINTA0` at `D:E601` | `INT_EINTA0` at `D:DE01` |
| DSP-to-MCU request | `INT_TOMCU` at `D:E610` | `INT_TOMCU` at `D:DE10` |
| Command-finished register | `DSP_CFR` at `D:E692` | `DSP_CFR` at `D:DE92` |
| Runtime command table | `D:64B7`, accepted IDs `1..52` | `D:854E`, accepted IDs `1..67` |
| Direct built-in handlers | IDs `1..34` and `51`; ID 52 is null | IDs `1..34` and `67` |
| User/extension slots | IDs `35..50` at `P:0FE0..0FFE` | IDs `35..66` at `P:1FC0..1FFE` |
| Pipe interrupt wrappers | `P:14B0` for pipes 0/1, `P:13C3` for pipe 2 | `P:24B0` for pipes 0/1, `P:23C3` for pipe 2 |
| Runtime `st1` page | `0x005D` | `0x007D` |
| ARM-side `DSP_SRC0..3` | `SCU:00D0..00DC`, VIC 54..57 | `SCU:00D0..00DC`, VIC 57..60 |

The PMB8875 runtime dispatcher compares the command against `0x34`, so ID 52
is within its numeric range even though the table entry is null. Tests reject
ID 53 rather than calling that null entry. On PMB8876, ID 68 is the first ID
above the table.

### PMB8875 P-space

| Word address | Size | Purpose |
|---|---:|---|
| `P:0000..0FFF` | 4K words | Program RAM; `P:0000..0001` form the reset branch |
| `P:1000..AFFF` | 40K words | Fixed Program Mask ROM |
| `P:B000..FFFF` | 20K words | Banked Program Mask ROM window |

`DSP_PAGE.PROG_PAGE`, bit 1 of `D:E6A3`, selects one of two window banks. The
physical size is `40K + 2 * 20K = 80K` words.

### PMB8875 D-space

| Word address | Size | Purpose |
|---|---:|---|
| `D:0000..5FFF` | 24K words | Main Data RAM, XRAM |
| `D:6000..8FFF` | 12K words | Fixed Data Mask ROM; command table at `D:64B7` |
| `D:9000..D7FF` | 18K words | Banked Data Mask ROM window |
| `D:D800..DDFF` | 1.5K words | Shared RAM |
| `D:DE00..E3FF` | 1.5K words | Unavailable range |
| `D:E400..E5FF` | 512 words | Modulator RAM |
| `D:E600..E6FF` | 256 words | DSP registers |
| `D:E700..F3FF` | 3.25K words | Specialized or reserved DSP/baseband windows |
| `D:F400..FFFF` | 3K words | Upper Data RAM, YRAM |

`DSP_PAGE.DATA_PAGE`, bit 0 of `D:E6A3`, selects one of two window banks. The
physical size is `12K + 2 * 18K = 48K` words.

### PMB8876 P-space

| Word address | Size | Purpose |
|---|---:|---|
| `P:0000..1FFF` | 8K words | Program RAM: startup code, vectors, and trampolines |
| `P:2000..9FFF` | 32K words | Fixed Program Mask ROM |
| `P:A000..FFFF` | 24K words | Banked Program Mask ROM window |

`DSP_PAGE.PROG_PAGE`, bits 3:2 of `D:DEA3`, selects one of three window banks.
The physical size is `32K + 3 * 24K = 104K` words.

### PMB8876 D-space

| Word address | Size | Purpose |
|---|---:|---|
| `D:0000..7FFF` | 32K words | Main Data RAM, XRAM |
| `D:8000..8FFF` | 4K words | Fixed Data Mask ROM; command table at `D:854E` |
| `D:9000..CFFF` | 16K words | Banked Data Mask ROM window |
| `D:D000..DBFF` | 3K words | Shared RAM, equivalent to 1.5K x 32 bits |
| `D:DC00..DDFF` | 512 words | Unavailable or reserved range |
| `D:DE00..EBFF` | 3.5K words | DSP registers and specialized baseband/audio windows |
| `D:EC00..FFFF` | 5K words | Upper Data RAM, YRAM |

`DSP_PAGE.DATA_PAGE`, bits 1:0 of `D:DEA3`, selects one of four visible window
views. The physical Data Mask ROM is 60K words, but the complete physical
mapping cannot be inferred by adding all four visible views; see the dump
results below.

The PMB8876 35904-word Incremental Redundancy memory belongs to a separate
channel-codec memory. Its direct mapping into ordinary P-space or D-space is
not yet known.

### Shared RAM from the ARM side

The ARM-side aperture starts at `DSP_RAM_BASE`. The three runtime pipes use
word offsets `0x005`, `0x021`, and `0x03D` on both CPUs, while their DSP
addresses differ because the D-space Shared RAM bases are `D:D800` and
`D:D000` respectively.

The current QEMU model exposes a 4K-byte ARM window. This must not be used as
evidence of the complete physical Shared RAM size.

### Program Mask ROM dump layout

`bsp/rom/dsp/<mask-id>-program-rom.bin` linearizes fixed ROM followed by every
banked window.

| CPU | Fixed ROM | Bank windows | Total |
|---|---|---|---:|
| PMB8875 | offset `0x00000`, `P:1000..AFFF`, `0x14000` bytes | offsets `0x14000`, `0x1E000`; each `P:B000..FFFF`, `0xA000` bytes | `0x28000` = 163840 bytes |
| PMB8876 | offset `0x00000`, `P:2000..9FFF`, `0x10000` bytes | offsets `0x10000`, `0x1C000`, `0x28000`; each `P:A000..FFFF`, `0xC000` bytes | `0x34000` = 212992 bytes |

## Mask ROM and startup firmware interaction

`PLOAD` loads mutable code into Program RAM; it does not load an additional
ROM. Program RAM and Program Mask ROM occupy different ranges of the same
P-space and remain accessible at the same time. Loading Program RAM neither
disables nor overlays Mask ROM.

After reset, the DSP starts in boot code built into Mask ROM. This boot loop
accepts only boot commands:

| Phase | Executed code | Accepted commands |
|---|---|---|
| Before `BRANCH` | Boot loop in Mask ROM | `PLOAD`, `DLOAD`, `PREAD`, `DREAD`, `FAST`, `BRANCH` |
| After `BRANCH` | Mask ROM and startup firmware in Program RAM | Normal DSP firmware runtime commands |

Several `PLOAD` and `DLOAD` commands may be issued. The last command in a
normal boot is always `BRANCH`: it leaves the boot loop and moves the program
counter to the loaded entry point, `P:0020` on SL98. Boot commands belong to
the boot phase, so Program RAM firmware does not accept the same command set
as the Mask ROM boot loop. After `BRANCH`, the runtime command dispatcher
continues to execute from Mask ROM, while the loaded code supplies extension
points and calls other Mask ROM functions.

These are not two firmwares executing concurrently: TEAKLite has one program
counter. At any instant, the core executes an instruction from either Program
RAM or Mask ROM. DSP/baseband hardware blocks and interrupt sources can operate
in parallel with the core.

After leaving the boot loop, Mask ROM remains a library of built-in functions.
For example, SL98 code starting at `P:0020` calls fixed Mask ROM addresses
`P:2779`, `P:9520`, `P:9979`, and `P:9AC2`. Before calling `P:E9C9`, the startup
code writes `0x8` to `D:DEA3`, selecting Program ROM page 2. This directly
demonstrates that loaded PRAM code continues to use Mask ROM.

The practical division of responsibilities is:

- Mask ROM contains the immutable boot protocol and basic low-level
  DSP/baseband functions;
- Program RAM contains the replaceable high-level control layer, entry points,
  interrupt vectors, command trampolines, extensions, and fixes;
- Data RAM contains loaded tables, coefficients, and state.

Program RAM cannot physically modify Mask ROM. It can replace part of its
behavior logically by installing another handler, selecting a different path,
wrapping a ROM call, or omitting that call. Program RAM is lost after reset,
and the DSP enters the built-in Mask ROM boot loop again.

## Runtime command dispatcher

The DSP has three equivalent command pipes. Their Shared RAM word offsets are
`0x005`, `0x021`, and `0x03D`. The Mask ROM dispatcher validates the command ID
and invokes a handler from its Data Mask ROM table:

```c
command = pipe[0];
if (command >= 1 && command <= max_command) {
	ack = -command;
	handler = data_mask_rom[command_table + command];
	handler(&pipe[1]);
	pipe[0] = ack; /* The handler sets this to 0 on error. */
}
```

PMB8875 uses `command_table=D:64B7` and `max_command=0x34`. Its IDs `1..34`
are built in, IDs `35..50` route through Program RAM slots
`P:0FE0..P:0FFE`, ID 51 is built in, and ID 52 has a null table entry. The
unit test places a handler in the first extension slot and invokes ID 35.

PMB8876 uses `command_table=D:854E` and `max_command=0x43`. Its table was read
from an EL71 using boot `DREAD` and routes commands as follows:

| ID | Route |
|---|---|
| `1..34` | Directly to Mask ROM handlers |
| `35..66` | To 32 two-word trampolines in Program RAM at `P:1FC0..P:1FFE` |
| `67` | Directly to Mask ROM at `P:2D76` |

Program RAM does not replace the common dispatcher. Mask ROM defines the
command protocol, validates the ID, selects a slot, and produces the
acknowledgement. Loaded firmware defines the contents of trampolines `35..66`:
a slot can jump to a real handler or to the common `P:28D7` rejection stub.

SL98 startup firmware `0x0801` installs these routes:

| ID | Command | Final handler |
|---:|---|---|
| 35 | `READ_PROG` | `P:2BD9` |
| 36 | `MCU_INT` | `P:2BE7` |
| 37 | `VB_I2Sy` | `P:2BEF` |
| 38 | `TTY_CTM` | `P:2CF1` |
| 39 | `VB_SYNC` | `P:2D07` |
| 40 | `UMTS_ON` | `P:2D19` |
| 41 | `MP3` | Reject at `P:28D7` |
| 42 | `SYNTH` | `P:2D28` |
| 43 | Old `RF_ADAPT` ID | Reject at `P:28D7` |
| 44 | `AUDIOPOSTPROC` | `P:2ED1` |
| 45 | `PCMPLAY` | `P:2EF5` |
| `46..48` | `DTW`, `TX_DIG`, `I2S_SWAP` | Reject at `P:28D7` |
| `49..64` | `USER_15..USER_0` | Reject at `P:28D7` |
| `65..66` | Additional extension slots | Reject at `P:28D7` |

In particular, `USER_15..USER_0` are reserved in advance as IDs `49..64` and
PRAM addresses `P:1FDC..P:1FFA`. They are not implemented by the inspected
firmware. Another startup firmware can place jumps to its own handlers in
these slots without changing the Mask ROM dispatcher.

Command 67 is implemented in Mask ROM. Its parameter count and uses match
`RF_ADAPT`: four control values, an update flag, and 13 coefficients. This is
an inference from the handler code; old ID 43 is disabled in startup firmware
`0x0801`.

A startup firmware `0x0800` listing must not be matched with Mask ROM `0x0801`:
trampolines and built-in handler addresses belong to a matched Mask/startup
version pair.

## Testing Mask ROM commands

The `dsp-mask-rom-commands` unit exercises the boot dispatcher that is already
running from Program Mask ROM after DSP reset. It does not load phone DSP
startup firmware. The boot phase covers five of the six boot opcodes:

- `PLOAD` and `PREAD`: write, overwrite, and verify Program RAM;
- `DLOAD` and `DREAD`: write, overwrite, and verify Data RAM;
- `PREAD`: verify fixed Program Mask ROM fingerprints at addresses valid for
  the selected CPU;
- `DREAD`: verify the fixed Data Mask ROM and runtime command table;
- `BRANCH`: transfer control to the minimal test startup in Program RAM.

The test startup installs the three command-pipe vectors, enables all three
pipe interrupt sources, and otherwise waits. Its only custom handler copies
one parameter to Shared RAM. PMB8875 invokes it through ID 35 at `P:0FE0`;
PMB8876 invokes it through `USER_0`, ID 64 at `P:1FFA`. Interrupt handling and
runtime command dispatch remain in Mask ROM.

After `BRANCH`, the test verifies the custom command and these built-in Mask ROM
commands through the normal command pipe and negative-command ACK protocol:

- `FC_INIT` (1), followed by `READ_DSP` of `MODE`, `THRSH`, and `HYST`;
- `MODU_INIT` (2), followed by `READ_DSP` of the stored parameters;
- `LOOP` (10), followed by `READ_DSP` of its state;
- `VB_STOP_TONE` (19) and `VB_READ_DURATION` (20), including the duration result
  in Shared RAM;
- PMB8876 `VB_SET_CBUF_GAIN` (26), including its five stored parameters;
- `READ_DSP` (33) from Data Mask ROM;
- `WRITE_DSP` (32), followed by `READ_DSP` of Data RAM;
- `WRITE_PROG` (34), verified after reset with boot `PREAD`;
- `IQ_SWAP_1` (3), `IQ_SWAP_2` (4), and `DTX_ON` (30), each with states 0 and 1.

The last three handlers use the TEAKLite eight-bit immediate addressing mode.
The Mask ROM interrupt wrapper sets `st1` to `0x005D` on PMB8875 and `0x007D`
on PMB8876, so the same kind of handler stores state on different D-space
pages.

`VB_SET_BIQUAD` (16) is isolated in a fresh runtime session so a stalled
voiceband path cannot invalidate later command checks. It is fully checked on
PMB8876. Mask ID `0602` requires voiceband initialization not supplied by this
minimal startup, so that one invocation is skipped on PMB8875 and the DSP is
reset immediately afterward.

`BRANCH` is last in each boot session because it terminates the Mask ROM boot
loop. `FAST` is not issued: it requires a valid standby-power-down snapshot in
`SM_SBPD_*` and is not safe as an isolated reset-state command.

The source images are `unit/dsp/commands.asm` and `commands-8875.asm`. Their
`.inc` files contain complete DSP1 containers as byte dumps; the test parses
the DSP1 header and segment table at runtime. Regenerate them with:

```sh
unit/dsp/build_dsp_rom.sh
```

Set `MAKEDSP1=/path/to/makedsp1` if the assembler is installed elsewhere.

Run it with:

```sh
cd bsp/unit
BOARD=siemens-el71 TEST_COLOR=OFF ./run.sh dsp-mask-rom-commands
BOARD=siemens-cx75 TEST_COLOR=OFF ./run.sh dsp-mask-rom-commands
```

Mask IDs in the `0x06xx` and `0x08xx` families are accepted. Exact fixed-ROM
fingerprints are known for `0x0602` and `0x0801`; other accepted revisions
still exercise ROM reads and compare runtime `READ_DSP` data with the same
boot-time `DREAD` data.

The built-in handlers are verified by reading their resulting Data RAM state,
not merely by observing the command ACK. Command ID 12 appears to be the
otherwise undocumented `BB_INIT`: it selects a profile through the Data Mask
ROM table at `D:80AA`. It is intentionally not exercised until the valid
profile indices and parameter contract are known.

## DSP service-request interrupts

The five consecutive ARM-side service-request registers at `SCU:00CC..00DC`
are not five DSP sources:

- `SCU_PM_INT_SRC` at `00CC`, VIC input 56, is the external Power ASIC/PMIC
  interrupt. SL98 registers its handler from `pow_int.c` and configures the
  corresponding `PM_INT` filter and edge field at `SCU_EXTI_*[17:16]`;
- `SCU_DSP_SRC(0..3)` at `00D0..00DC` are the four TEAKLite DSP-to-MCU
  interrupt sources. They route to VIC 54..57 on PMB8875 and VIC 57..60 on
  PMB8876.

The primary `scu` unit covers `PM_INT_SRC` as an SCU service-request register.
The primary `dsp` unit uses `SETR` to verify the software request, VIC route,
and `SRR` acknowledgement of all four `DSP_SRC` registers. This is
register-path coverage only; it does not claim that the request originated in
the PMIC or DSP.

The separate `dsp-irqs` unit loads a minimal Program RAM loop and verifies the
real DSP-generated path. A TEAKLite store to `INT_TOMCU`, at `D:E610` on
PMB8875 or `D:DE10` on PMB8876, raises:

| DSP write | ARM service request | PMB8875 VIC | PMB8876 VIC |
|---|---|---:|---:|
| bit 0 | `SCU_DSP_SRC(0)` | 54 | 57 |
| bit 1 | `SCU_DSP_SRC(1)` | 55 | 58 |
| bit 2 | `SCU_DSP_SRC(2)` | 56 | 59 |
| bit 3 | `SCU_DSP_SRC(3)` | 57 | 60 |

Bit 4 raises none of these sources.

The source images are `unit/dsp/irqs.asm` and `irqs-8875.asm`;
`unit/dsp/build_dsp_rom.sh` assembles them and embeds each complete DSP1
container in its `.inc`. The specialized unit only enables DSP CLC; module
identification and reset-value checks remain in the primary `dsp` unit.

Run the hardware test with:

```sh
cd bsp/unit
BOARD=siemens-el71 TEST_COLOR=OFF ./run.sh dsp-irqs
BOARD=siemens-cx75 TEST_COLOR=OFF ./run.sh dsp-irqs
```

## Dumping Program Mask ROM

The `dsp-rom-dump` test stays in the boot loop and uses only `DLOAD` and
`PREAD`. It selects the layout from the Mask ID family, writes the appropriate
page values to `DSP_PAGE`, and reads the fixed ROM followed by every banked
window. Neither `BRANCH` nor code loaded into Program RAM is needed.

Run the test on a supported PMB8875- or PMB8876-based board and save the result
with:

```sh
(cd bsp/unit && BOARD=siemens-el71 TEST_COLOR=OFF ./run.sh dsp-rom-dump) \
  | tee /tmp/dsp-0801-program-rom.log
rg -a '^:' /tmp/dsp-0801-program-rom.log > /tmp/dsp-0801-program-rom.hex
objcopy -I ihex -O binary /tmp/dsp-0801-program-rom.hex \
  bsp/rom/dsp/0801-program-rom.bin
wc -c bsp/rom/dsp/0801-program-rom.bin
sha256sum bsp/rom/dsp/0801-program-rom.bin
```

Use `BOARD=siemens-cx75` for PMB8875 or `BOARD=siemens-el71` for PMB8876.

Verified result for PMB8875 revision 05, DSP Mask ROM ID `0x0602`:

```text
size:   163840 bytes
sha256: f9f38a68034221b5b826936abab9ccf6d8c10e4de25f7c3570ad7558e7159693
fixed:  word-wise FNV-1a 833458ED
bank 0: word-wise FNV-1a 8CF47EA9
bank 1: word-wise FNV-1a 91076EF9
```

Verified result for PMB8876 revision 10, DSP Mask ROM ID `0x0801`:

```text
size:   212992 bytes
sha256: 9504a73f6a602189339a1cc69a321cb594939c6eae89a6afe5d06e85d4c4c1c6
fixed:  word-wise FNV-1a 3B018B00
bank 0: word-wise FNV-1a 61DCDE5D
bank 1: word-wise FNV-1a 041E8769
bank 2: word-wise FNV-1a 436D3807
```

The 212992-byte size is correct: 104K means 104K 16-bit words, not 104K bytes.
Thus `104 * 1024 * 2 = 212992` bytes (`0x34000`, or 208 KiB).

The contents are determined by the DSP silicon mask, identified here by CPU
type/revision and DSP Mask ROM ID. They do not depend on the phone model or on
the startup firmware loaded into DSP RAM. The board name in the run command
only selects a transport and hardware configuration known to the test harness.

## Dumping Data Mask ROM

The separate `dsp-data-rom-dump` test remains in the same Mask ROM boot loop.
It uses `DLOAD` only to select `DSP_PAGE.DATA_PAGE`, then uses `DREAD` to dump
the fixed range and every selectable view of the banked window. It does not
issue `BRANCH`, and data loaded by the phone firmware into Data RAM is not part
of the result.

`bsp/rom/dsp/<mask-id>-data-rom.bin` has this linear layout:

| CPU | Fixed ROM | Bank windows | Total visible dump |
|---|---|---|---:|
| PMB8875 | offset `0x00000`, `D:6000..8FFF`, `0x6000` bytes | offsets `0x06000`, `0x0F000`; each `D:9000..D7FF`, `0x9000` bytes | `0x18000` = 98304 bytes |
| PMB8876 | offset `0x00000`, `D:8000..8FFF`, `0x2000` bytes | offsets `0x02000`, `0x0A000`, `0x12000`, `0x1A000`; each `D:9000..CFFF`, `0x8000` bytes | `0x22000` = 139264 bytes |

Run and convert it with:

```sh
(cd bsp/unit && BOARD=siemens-el71 TEST_COLOR=OFF ./run.sh dsp-data-rom-dump) \
  | tee /tmp/dsp-0801-data-rom.log
rg -a '^:' /tmp/dsp-0801-data-rom.log > /tmp/dsp-0801-data-rom.hex
objcopy -I ihex -O binary /tmp/dsp-0801-data-rom.hex \
  bsp/rom/dsp/0801-data-rom.bin
wc -c bsp/rom/dsp/0801-data-rom.bin
sha256sum bsp/rom/dsp/0801-data-rom.bin
```

Verified result for PMB8875 revision 05, DSP Mask ROM ID `0x0602`:

```text
size:   98304 bytes
sha256: d1043ce1936861f56acc5f097af161b2b8b7f0e6ee23d0c9efb04528c289ac9c
fixed:  word-wise FNV-1a BE0C6441
bank 0: word-wise FNV-1a 034D0BCD
bank 1: word-wise FNV-1a E28E1781
```

Verified result for PMB8876 revision 10, DSP Mask ROM ID `0x0801`:

```text
size:   139264 bytes
sha256: 48d861f10fb5e2b45bd57de4374911694688a068cf9ff96f86dd008132175971
fixed:  word-wise FNV-1a 23B7451F
bank 0: word-wise FNV-1a 12BC63DB
bank 1: word-wise FNV-1a 44F0F202
bank 2: word-wise FNV-1a EC3B8991
bank 3: word-wise FNV-1a 38699DC5
```

The fourth 16K-word view is entirely zero on this mask. Consequently this
139264-byte file is a dump of every address selected by the two-bit page
field, not proof that the complete stated 60K-word physical Data ROM is mapped
as `4K + 4 * 16K`. The three populated bank views plus the fixed range account
for 52K words. The location or accessibility of the remaining stated 8K words
still needs independent confirmation.

## Emulator implementation

- QEMU memory map: `qemu/hw/arm/pmb887x/gen/cpu_meta.c`.
- QEMU DSP model: `qemu/hw/arm/pmb887x/dsp.c`.
- L1 monitor ring parsing and assert `0x4011`: [l1mon.md](l1mon.md).
