# VS7 DSP startup firmware

VS7 accepts DSP Mask IDs `0603` and `0604` and selects a separate ARM-side boot
stream for each. These files contain the `PLOAD`, `DLOAD`, and `BRANCH` records
sent after reset; they do not contain the factory Mask ROM.

| Mask ID | ARM address | PLOAD | DLOAD | Entry | Container SHA-256 |
|---:|---:|---:|---:|---:|---|
| `0603` | `0xA06F450C` | 3723 words | 593 words | `P:0020` | `b671fb794504faccec1a154f2e36f09f8d01d11d78091e608c0492d781ce3dd8` |
| `0604` | `0xA06F342A` | 1506 words | 617 words | `P:0020` | `9ee6d8b158072a70ef2f4ec59ce2f60da43837315482c45c6533c086cf40ab6e` |

Each revision directory contains the exact container, DSP1 conversion, Teakra
listing, sparse P/D images, individual record payloads, and `map.tsv`.

Regenerate an extraction from a raw VS7 image with:

```sh
perl bsp/tools/dsp/extract_dsp_firmware.pl \
  --address 0xA06F450C \
  --out /tmp/vs7-dsp-0603 \
  /path/to/vs7.bin
```

## Runtime-command routes

Mask ROM `0604` routes IDs 46 through 61 through the 16 Program RAM
trampolines at `P:0FE0..P:0FFE`. VS7 points every `0604` slot at the common
rejection handler `P:16BB`.

The `0603` Mask ROM has not been dumped independently, so its command IDs must
not be inferred from the `0604` dispatcher. The startup image installs these
slot targets without revealing which command-table entries reach them:

| PRAM slot | 0603 target | 0604 target and known ID |
|---:|---:|---|
| `P:0FE0` | `P:01F2` | Reject at `P:16BB`, ID 46 |
| `P:0FE2` | `P:0296` | Reject at `P:16BB`, ID 47 |
| `P:0FE4` | `P:02B5` | Reject at `P:16BB`, ID 48 |
| `P:0FE6..P:0FFE` | Reject at `P:16C5` | Reject at `P:16BB`, IDs 49..61 |

The `0603` implementations are loaded into Program RAM:

- Slot `P:0FE0` consumes four fixed control words and an enable word. When enabled it
  invokes the Mask ROM block-copy routine with count register `r5=13`.
- Slot `P:0FE2` consumes one control word and conditionally
  copies blocks selected from `D:FF68`/`D:FF6C` and `D:FF74`/`D:FF82` with
  count-register values 4 and 10.
- Slot `P:0FE4` accepts switch values 0 or 1. The enabled path consumes three additional
  words, copies a mode-dependent block, and calls Mask ROM `P:131D`; other
  switch values reject.

These descriptions come from the extracted Teak code. Assigning public names
or command IDs to the three `0603` slots requires a real `0603` Mask ROM dump.
