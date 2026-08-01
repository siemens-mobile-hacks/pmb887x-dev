# CX75 DSP startup firmware

CX75 selects one of two ARM-side boot streams according to the DSP Mask ROM
revision. These files contain the `PLOAD`, `DLOAD`, and `BRANCH` records sent to
the DSP after reset; they do not contain the factory Mask ROM.

| Mask ID | ARM address | PLOAD | DLOAD | Entry | Container SHA-256 |
|---:|---:|---:|---:|---:|---|
| `0602` | `0xA0D07080` | 3598 words | 441 words | `P:0020` | `6f83d473ee65a5e272440b4f76666703d6a1bac2c9e74f684afd6fc5ccb8db16` |
| `0605` | `0xA0D0908E` | 759 words | 3 words | `P:0020` | `e7811189a890871905d6d5eba407e9eda9e2a6c41847ea9644181b85c01a12b5` |

Each revision directory contains:

- `container.bin`: exact ARM-side boot stream;
- `firmware.dsp1`: PLOAD and DLOAD payloads in DSP1 format;
- `firmware.txt`: Teakra instruction listing;
- `pram.bin` and `dram.bin`: sparse little-endian memory images;
- `records/` and `map.tsv`: individual records and their addresses.

Regenerate an extraction from a raw CX75 image with:

```sh
perl bsp/tools/dsp/extract_dsp_firmware.pl \
  --address 0xA0D07080 \
  --out /tmp/cx75-dsp-0602 \
  /path/to/CX75.bin
```

The legacy flat `0602-container.bin`, `0602.bin`, `0602.dsp1`, and `0602.txt`
files are byte-identical to the corresponding files under `0602/`.

## Runtime-command routes

Mask ROM `0602` routes IDs 35 through 50 through 16 Program RAM trampolines at
`P:0FE0..P:0FFE`. The `0605` Mask ROM has not been dumped independently, but
its targets have the same order and roles as `0602`; the ID labels in that
column therefore follow the shared CX75 ABI.

| ID | Name | 0602 target | 0605 target |
|---:|---|---:|---:|
| 35 | `READ_PROG` | `P:1BC5` | `P:1BF6` |
| 36 | `MCU_INT` | `P:2A32` | `P:2BCE` |
| 37 | `VB_I2Sy` | `P:2A41` | `P:2BED` |
| 38 | `TTY_CTM` | `P:2B04` | `P:2CA3` |
| 39 | `VB_SYNC` | `P:2B7A` | `P:2D19` |
| 40 | `UMTS_ON` | `P:2B89` | `P:2D28` |
| 41 | `MP3` | `P:2BA0` | `P:2D51` |
| 42 | `SYNTH` | `P:2BAC` | `P:2D5D` |
| 43 | `RF_ADAPT` | `P:2BC2` | `P:2D73` |
| 44 | `AUDIOPOSTPROC` | `P:2BC9` | `P:2D7A` |
| 45 | `PCMPLAY` | `P:2C3A` | `P:2DF9` |
| 46 | `DTW` | `P:AD3F` | `P:AB77` |
| 47 | `TX_DIG` | `P:AEE6` | `P:E6E0` |
| 48 | `I2S_SWAP` | Reject at `P:1867` | Reject at `P:1865` |
| 49 | `USER_15` | Reject at `P:1867` | Reject at `P:1865` |
| 50 | `USER_14` | Reject at `P:1867` | Reject at `P:1865` |

All installed targets are Mask ROM addresses. CX75 does not place a private
runtime-command implementation in Program RAM; its startup firmware selects
between alternate implementations already present in each Mask ROM.
