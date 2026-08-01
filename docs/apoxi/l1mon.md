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

The DSP core, Mask ROM, phone user ROM, TPU signal routing, Baseband status,
and DSP interrupts are all active. The failure was caused by independent
problems in their execution path:

- the Baseband model did not reset `BB_WR_POINTER` at the start of each
  `EQON`, `FCON`, `MONON`, or `SCON` job;
- the TCG block cache avoided recompilation but still decoded the same DSP
  block before every execution, so the DSP worker did not keep up with ARM;
- a later threaded model queued TPU transitions and consumed only one before
  each long DSP execution slice. A monitoring burst accumulated 42 events and
  reached 6.8 ms of delivery backlog in emulated time.

The PMB7870 design specification defines `BB_WR_POINTER` as an absolute sample
RAM address. It resets at the beginning of each Baseband job, increments while
samples are written, and holds the last written position after the falling
edge. `RXON` is only a status input and does not start a job. The sample area is
`0x000..0x3BF`; `0x3C0..0x3FF` contains coefficients.

A narrow TPU/Baseband/interrupt trace confirmed that:

- TPU generates repeated `RXON` and `MONON` transitions;
- DSP firmware reads and clears `BBHI`, `BBLO`, and `BB_FULL`;
- DSP firmware enters the Mask ROM Baseband handlers;
- the old cumulative `BB_WR_POINTER` generated false `BB_FULL` events and gave
  the Mask ROM handler the wrong end position;
- fixing the pointer and translation cache removed two delays, but did not
  guarantee that the Mask ROM handler finishes before the ARM consumes the
  result.

The ARM and DSP run in independent host threads. TPU signals are inputs of the
DSP peripheral model: a transition immediately updates Baseband state and
interrupt flags, independently of the DSP execution backend. It also wakes the
DSP thread if the core is idle. There is no TPU-event queue whose latency
depends on the DSP execution slice.

This removes the queue backlog, but it does not provide virtual-time
co-scheduling between the two cores. `dsp-baseband-functional` demonstrates the
remaining limitation: ARM/TPU can advance to `BB_FULL` or `BBLO` before the
host DSP thread has serviced `BBHI`. Accurate ordering requires a common
virtual-time scheduler for ARM and DSP that is independent of the DSP execution
backend; it must not be implemented as a translation-block hook, an ARM wait,
or firmware-specific synchronization.

A host/virtual-time trace makes the missed deadline explicit. Times below are
relative to the first `MONON` falling edge of the burst:

| Event | Virtual time | Host time |
|---|---:|---:|
| first `MONON` falling edge | 0 | 0 |
| sixth `MONON` falling edge | 2.620 ms | 0.783 ms |
| next `TPU_INT1` result collection | 3.914 ms | 2.153 ms |
| first `SM_MON_VALS` write at `P:20E6` | 7.713 ms | 3.373 ms |

The I/Q acquisition windows therefore finish before ARM collects the results,
but the DSP firmware does not publish even the first result in time. During the
burst, falling edges arrive every 122..129 us of host time while consecutive
DSP result writes take approximately 199..351 us of host time. The first DSP
interrupt status read also observes `BBHI | BBLO` together, proving that both
edges of a 230.8-us virtual window elapsed before the DSP serviced the first
edge. Since Baseband interrupt flags are latches rather than counters, later
edges can coalesce and cannot be recovered by processing them afterward.

The 4.615-ms GSM frame budget is sufficient on hardware: even the longest
documented monitor calculation takes less than 25 us. It is insufficient in
the current emulator because ARM advances QEMU virtual time independently of
the DSP host thread. Waking that thread delivers the input promptly but does
not guarantee that it executes before ARM reaches the virtual-time deadline.
The dense MAC benchmark measures steady-state DSP host throughput and does not
measure this cross-thread interrupt latency or event ordering.

The repeated-state idle detector was removed. It could mistake a firmware poll
loop for sleep and had no hardware basis. The DSP now stops only when the core
is disabled by hardware state. ARM MMIO and Shared RAM paths do not wait for DSP
startup or completion.

The current Baseband model advances the sample pointer over QEMU virtual time
while a receive job is active and schedules `BB_FULL` when the pointer crosses
`BB_INT_POINTER`. Its exact sample rate and interrupt timing still need the
hardware measurements listed in [tpu-dsp-tests.md](tpu-dsp-tests.md).

The complete emulated chain remains:

```text
TPU MONON falling edge -> DSP monitor calculation -> SM_MON_VALS write
                       -> SM_MON_INDEX advance
```

A direct write to `SM_MON_VALS`, a firmware-PC hook, or waiting for the DSP from
an ARM MMIO/shared-memory access would hide the bug and is not a hardware
model. The current implementation uses none of these shortcuts.

Extended SL98 smoke runs remain alive after direct TPU signal delivery, but the
firmware assert is not written to QEMU stderr. They therefore do not replace a
firmware-visible l1mon check. Earlier runs confirmed that synchronous DSP
startup waits are not required on the ARM access path.

The DSP architecture, firmware, and memory map are documented in
[dsp.md](dsp.md).
