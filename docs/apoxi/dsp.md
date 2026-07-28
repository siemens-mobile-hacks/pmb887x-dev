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

## Address spaces

TEAKLite uses a Harvard architecture: P-space and D-space are separate address
spaces. Identical numerical addresses such as `P:0020` and `D:0020` identify
different physical words. DSP addresses are measured in 16-bit words, while
ARM addresses and file offsets are measured in bytes.

Physical PMB8876 DSP memory sizes:

| Memory | Size |
|---|---:|
| Program Mask ROM | 104K x 16 bits |
| Program RAM | 8K x 16 bits |
| Data Mask ROM | 60K x 16 bits |
| Data RAM | 37K x 16 bits |
| Incremental Redundancy memory | 35904 x 16 bits |
| Shared RAM | 1.5K x 32 bits, or 3K x 16 bits as seen by the DSP |

### P-space

| Word address | Size | Purpose |
|---|---:|---|
| `P:0000..1FFF` | 8K words | Program RAM: startup code, vectors, and trampolines |
| `P:2000..9FFF` | 32K words | Fixed Program Mask ROM |
| `P:A000..FFFF` | 24K words | Banked Program Mask ROM window |

The `P:A000..FFFF` window exposes one of three physical pages. The page is
selected by `DSP_PAGE.PROG_PAGE`, bits 3:2 of register `D:DEA3`:

| `PROG_PAGE` | `D:DEA3` value when the data page is 0 | Physical ROM |
|---:|---:|---|
| 0 | `0x0000` | Program bank 0 |
| 1 | `0x0004` | Program bank 1 |
| 2 | `0x0008` | Program bank 2 |

The physical Program Mask ROM therefore contains
`32K + 3 * 24K = 104K` 16-bit words. Changing the bank does not affect the
fixed ROM or Program RAM.

### D-space

The following working map is supported by the stated memory sizes, startup
image destinations, and observed code accesses:

| Word address | Size | Purpose |
|---|---:|---|
| `D:0000..7FFF` | 32K words | Main Data RAM, XRAM |
| `D:8000..8FFF` | 4K words | Fixed Data Mask ROM; the command table is at `D:854E` |
| `D:9000..CFFF` | 16K words | Banked Data Mask ROM window |
| `D:D000..DBFF` | 3K words | Shared RAM, equivalent to 1.5K x 32 bits |
| `D:DC00..DDFF` | 512 words | Unavailable or reserved range |
| `D:DE00..EBFF` | 3.5K words | DSP registers and specialized baseband/audio memory windows |
| `D:EC00..FFFF` | 5K words | Upper Data RAM, YRAM |

The lower and upper RAM ranges account for the stated
`32K + 5K = 37K` words of Data RAM. The upper RAM is also visible in a startup
image `DLOAD` targeting `D:F350`.

`DSP_PAGE.DATA_PAGE` occupies bits 1:0 of `D:DEA3` and selects the contents of
the `D:9000..CFFF` window. The physical Data Mask ROM contains 60K words, but
the occupancy of the last pages within this window has not yet been dumped
completely. The populated boundary of each data page therefore requires a
separate full `DREAD`; it cannot be recovered by reading one linear 64K
D-space image.

The 35904-word Incremental Redundancy memory belongs to a separate channel
codec memory. Its direct mapping into ordinary P-space or D-space is not yet
known.

### Shared RAM from the ARM side

The DSP accesses Shared RAM as `D:D000..DBFF`. For example:

| DSP address | Purpose |
|---|---|
| `D:D005` | Command pipe 0 |
| `D:D021` | Command pipe 1 |
| `D:D03D` | Command pipe 2 |
| `D:D0BF` | `SM_MON_INDEX` |
| `D:D0C0..D0C7` | `SM_MON_VALS[0..7]` |

With ARM byte addressing, the monitor index is at
`0xF6001000 + 0xBF * 2 = 0xF600117E`. The current QEMU model exposes a 4K-byte
ARM window at `0xF6001000..0xF6001FFF`. This is smaller than the physical
Shared RAM size of 1.5K x 32 bits, so the model's ARM aperture must not be used
as evidence of the complete hardware size.

### Program Mask ROM dump layout

`/tmp/el71-dsp-mask-rom.bin` linearizes the banked P-space as follows:

| Byte offset | Size | Contents |
|---:|---:|---|
| `0x00000` | `0x10000` | Fixed ROM `P:2000..9FFF` |
| `0x10000` | `0x0C000` | Bank 0 `P:A000..FFFF` |
| `0x1C000` | `0x0C000` | Bank 1 `P:A000..FFFF` |
| `0x28000` | `0x0C000` | Bank 2 `P:A000..FFFF` |
| | `0x34000` | Total: 212992 bytes |

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

The DSP has three equivalent command pipes. The Mask ROM dispatcher reads the
command word and parameters from `D:D005`, `D:D021`, or `D:D03D`, validates the
ID, and invokes a handler from the Data Mask ROM table:

```c
command = pipe[0];
if (command >= 1 && command <= 0x43) {
	ack = -command;
	handler = data_mask_rom[0x854E + command];
	handler(&pipe[1]);
	pipe[0] = ack; /* The handler sets this to 0 on error. */
}
```

The `D:854E..D:8591` table was read from an EL71 using boot `DREAD`. It routes
commands as follows:

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

## Dumping Program Mask ROM

The Program Mask ROM layout and `DSP_PAGE` register are described above.
Before calls into the banked window, startup firmware writes values such as
`0x5` (data page 1, program page 1) and `0x8` (data page 0, program page 2).

The `dsp-rom-dump` test stays in the boot loop and uses only `DLOAD` and
`PREAD`: it writes `0`, `4`, and `8` to `D:DEA3`, then reads the corresponding
window. Neither `BRANCH` nor code loaded into PRAM is needed for the dump. The
output file contains the fixed ROM followed by Program ROM windows 0, 1, and 2.

Run the test on EL71 and save the result with:

```sh
cd bsp/unit
BOARD=siemens-el71 TEST_COLOR=OFF ./run.sh dsp-rom-dump \
  | tee /tmp/el71-dsp-mask-rom.log
rg -a '^:' /tmp/el71-dsp-mask-rom.log > /tmp/el71-dsp-mask-rom.hex
objcopy -I ihex -O binary \
  /tmp/el71-dsp-mask-rom.hex /tmp/el71-dsp-mask-rom.bin
wc -c /tmp/el71-dsp-mask-rom.bin
sha256sum /tmp/el71-dsp-mask-rom.bin
```

Verified result for an EL71 with PMB8876 revision 10:

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

## Emulator implementation

- QEMU memory map: `qemu/hw/arm/pmb887x/gen/cpu_meta.c`.
- QEMU DSP model: `qemu/hw/arm/pmb887x/dsp.c`.
- L1 monitor ring parsing and assert `0x4011`: [l1mon.md](l1mon.md).
