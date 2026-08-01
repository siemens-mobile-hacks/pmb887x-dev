# EL71 DSP startup firmware

EL71 selects one of two ARM-side boot streams according to the DSP Mask ROM
revision. These files contain the `PLOAD`, `DLOAD`, and `BRANCH` records sent to
the DSP after reset; they do not contain the factory Mask ROM.

| Mask ID | ARM address | PLOAD | DLOAD | Entry | Container SHA-256 |
|---:|---:|---:|---:|---:|---|
| `0800` | `0xA0C82E44` | 1646 words | 93 words | `P:0020` | `9008ecfbb0339b28c35fecdbccb9f78100a22710c5de4d7b00eb12ea337a80e7` |
| `0801` | `0xA0C83C18` | 6809 words | 452 words | `P:0020` | `e8c1e3dd24bc5814349e15d35405751c5e9f6bfe6b5ae7b935859e7fcb50dc7b` |

Each revision directory contains:

- `container.bin`: exact ARM-side boot stream;
- `firmware.dsp1`: PLOAD and DLOAD payloads in DSP1 format;
- `firmware.txt`: Teakra instruction listing;
- `pram.bin` and `dram.bin`: sparse little-endian memory images;
- `records/` and `map.tsv`: individual records and their addresses.

Regenerate an extraction from a raw EL71 ARM image with:

```sh
perl bsp/tools/dsp/extract_dsp_firmware.pl \
  --address 0xA0C83C18 \
  --out /tmp/el71-dsp-0801 \
  /path/to/EL71.bin
```

## Runtime-command routes

Mask ROM commands 35 through 66 first enter the Program RAM trampolines at
`P:1FC0..P:1FFE`. The startup firmware therefore determines whether these
commands run, reject, or call a different Mask ROM implementation.

| ID | Public name | 0800 target | 0801 target |
|---:|---|---:|---:|
| 35 | `READ_PROG` | `P:2B6A` | `P:2BD9` |
| 36 | `MCU_INT` | `P:4176` | `P:44FC` |
| 37 | `VB_I2Sy` | `P:419F` | `P:4525` |
| 38 | `TTY_CTM` | `P:4280` | `P:460C` |
| 39 | `VB_SYNC` | `P:435A` | `P:46E6` |
| 40 | `UMTS_ON` | `P:4369` | `P:46F5` |
| 41 | `MP3` | `P:4382` | `P:471C` |
| 42 | `SYNTH` | `P:438E` | `P:4728` |
| 43 | legacy `RF_ADAPT` | `P:43A4` | `P:4738` |
| 44 | `AUDIOPOSTPROC` | `P:43E7` | `P:4789` |
| 45 | `PCMPLAY` | `P:4467` | `P:4813` |
| 46 | `DTW` | `P:008B` | `P:00DD` |
| 47 | `TX_DIG` | `P:0097` | `P:00E9` |
| 48 | `I2S_SWAP` | `P:431A` | `P:46A6` |
| 49 | `USER_15` | `P:44CF` | `P:4829` |
| 50 | `USER_14` | Reject | `P:4845` |
| 51 | `USER_13` | Reject | `P:13AE` |
| 52 | `USER_12` / `[AFE_REG_UPDATE]` | Reject | `P:13C0` |
| 53..66 | user extensions | Reject | Reject |

The public names come from the PMB7870 firmware manual. The target addresses
come from the extracted EL71 trampolines and are authoritative for this
startup-firmware pair.

## EL71/0801 private handlers

The four handlers below are either loaded into Program RAM or exposed only by
the EL71 route table:

- ID 46, `P:00DD`: select DSP page value 5, call banked Mask ROM `P:CA48`,
  restore page 0, and return. EL71 calls it for DTW start and stop operations.
- ID 47, `P:00E9`: return without consuming parameters. The 0800 firmware
  instead calls banked Mask ROM `P:C7F0`.
- ID 49, `P:4829`: consume four words and update the alternate PCM-player
  state at `D:5214..D:5219`. The ARM audio-routing path submits this as command
  49.
- ID 50, `P:4845`: clear 64 words at `D:F9CF`, consume one control word, and
  select one of the Mask ROM routines at `P:42D5` or `P:42EE` through bits 3
  and 2. The ARM audio-routing path sends two words, although the handler
  consumes only the first.
- ID 51, `P:13AE`: copy 16 words to `D:7ECB..D:7EDA`, clear 12 words at
  `D:7EBF..D:7ECA`, and write either 0 or 4 to `D:4A84` according to
  `D:7ECF`.
- ID 52, `P:13C0`: perform a masked AFE-register update. With parameters
  `OFFSET, VALUE, MASK`, it writes
  `(old & ~MASK) | (VALUE & MASK)` to `D:DE70 + OFFSET`. EL71 uses offsets
  3 through 6 for `AFE_VRX1`, `AFE_VRX2`, `AFE_VTX`, and `AFE_RING`.

IDs 49 through 51 have no recovered symbolic Siemens names yet.
`AFE_REG_UPDATE` is a descriptive name recovered from both sides of the ABI.
The behavior above comes from the Teak listing and EL71 ARM callers, not from
emulator code.
