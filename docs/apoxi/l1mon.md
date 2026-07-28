# L1 monitoring: failure at `l1co_mon.c:0x415`

## Symptom

```text
A01C87F8: __tpu_mon_assert(0x4011, 0x2, 0x0, 0x415,
                          "../../../gems_system/ms-gprs-l1-src/text/l1co_mon.c")
A01C87BC: __tpu_assert3(0x415,
                       "../../../gems_system/ms-gprs-l1-src/text/l1co_mon.c",
                       0x4011)
```

In `l1co_mon_process_results`, assert `0x4011` fires when the next monitoring
result is `0xFFFF`. This value is an empty-slot sentinel, not a valid
measurement result.

## Result location

`0xF600117E` belongs to DSP Shared RAM, not TPU RAM:

- DSP Shared RAM: `0xF6001000..0xF6001FFF` in the current emulator map;
- TPU RAM: `0xF6401000..0xF6401FFF`.

L1 initialization uses base address `0xF6001000` and places the monitor
structure at `0xF600117E`.

In this DSP ABI, `SM_MON_INDEX` has word offset 191 and `SM_MON_VALS` occupies
offsets 192..199. Since `191 * 2 = 0x17E`, a Shared RAM base of `0xF6001000`
produces the same address, `0xF600117E`.

```c
typedef struct {
	int16_t index;       /* 0xF600117E: SM_MON_INDEX */
	int16_t values[8];   /* 0xF6001180: SM_MON_VALS */
} dsp_monitor_results_t;
```

## Measurement producer

The TPU and DSP perform different parts of the operation:

1. The TPU/GSM timer creates the `MONON` window.
2. The baseband receive unit writes filtered I/Q samples into internal DSP
   memory.
3. The DSP receives an interrupt on the falling edge of `MONON`.
4. The DSP calculates RMS, normalizes it by the window length, and converts it
   to a logarithmic value in units of 1/16 dB.
5. The DSP writes the result into the `SM_MON_VALS` ring and advances
   `SM_MON_INDEX`.

The TPU therefore controls measurement timing, baseband hardware collects
samples, and DSP firmware calculates and publishes the result through Shared
RAM.

Monitoring has these constraints:

- monitoring window: 7..190 symbols;
- at least 60 symbols are needed for a useful RMS value;
- adjacent windows must be separated by at least 70 us;
- calculation for the longest window takes less than 25 us at 104 MHz.

## DSP ring algorithm

For a valid `SM_MON_INDEX` value:

```c
int index = results->index;

if (index >= 0 && index < 8) {
	results->values[index] = rms_db_x16;
	results->index = (index + 1) & 7;
}
```

The ARM reads only completed slots and then marks every consumed slot with
`0xFFFF`.

## ARM-side result conversion

The DSP writes RMS in units of 1/16 dB, not a final RXLEV value. The ARM first
rounds it to whole dB:

```c
q = (raw + 8) >> 4;
```

It then applies calibration and saturation:

```c
if (q < 21)
	rxlev = 0;
else
	rxlev = clamp(q - 0x4C - correction[path] + 0x6E + band_bias,
	              0, 0x49);
```

The firmware uses:

```c
correction[0..3] = { 0x17, 0x2B, 0x3F, 0x4B };
band_bias = band == 1 || band == 2;
```

The final range is `0..73`.

To generate a raw value for a requested, unclipped RXLEV, use the inverse:

```c
q = rxlev + 0x4C + correction[path] - 0x6E - band_bias;
raw = q << 4;
```

The `q >= 21` threshold and final RXLEV saturation still apply.

## Cause of the QEMU failure

The current DSP model only provides a passive backing array for Shared RAM.
Memory is cleared on reset, and `dsp_update_state()` is not implemented.
Consequently, nothing handles the falling edge of `MONON`, writes
`SM_MON_VALS`, or advances `SM_MON_INDEX`.

After the ARM marks a consumed slot as `0xFFFF`, the next monitor job observes
the unfilled slot and raises assert `0x4011`.

A fix must implement this chain:

```text
TPU MONON falling edge -> DSP monitor calculation -> SM_MON_VALS write
                       -> SM_MON_INDEX advance
```

For an initial no-RF implementation, publishing a deterministic valid raw
value below the threshold and advancing the index correctly is sufficient. A
complete implementation should derive the raw value from the selected ARFCN's
signal level and RF path parameters.

The DSP architecture, firmware, and memory map are documented in
[dsp.md](dsp.md).
