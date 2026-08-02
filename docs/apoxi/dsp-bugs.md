# DSP bugs

## Missing CIPH completion event in QEMU

Status: fixed.

CX75 startup firmware for Mask ROM `0602` hangs after the boot loader executes
`BRANCH P:0020`. The ARM has set `DSP_COM_SET.bit0` and waits for the DSP to
acknowledge the command by clearing the communication flag through
`MCS_CFR.bit0`.

The DSP reaches this loop in `bsp/rom/dsp/cx75/0602.txt`:

```text
P:0081  load D:E608 into r0
P:0083  tstb [r0], 0
P:0084  brr P:0083, neq
```

`D:E608` is `INT_FINT1`; bit 0 is the CIPH interrupt flag. TEAKLite `TSTB`
copies the tested bit into `Z`, as specified by the CEVA TEAKLite Architecture
Specification. Therefore `brr neq` deliberately waits while CIPH is zero. This
differs from instructions which set `Z` when their result is zero and must not
be changed in the TCG implementation.

The startup firmware writes `CIPH_CSTAT=0x11` and then `CIPH_CSTAT=0x19`. The
second value sets `INIT`, which starts cipher initialization. QEMU previously
routed CIPH through the unknown peripheral, so initialization never completed
and `INT_FINT1.CIPH` remained zero forever. Execution consequently never
reached the `MCS_CFR` acknowledgement at `P:00B4`.

The basic CIPH model stores its registers and immediately raises the CIPH flag
when `CSTAT.INIT` is written. This models completion timing without
implementing the cipher algorithm. CX75 then leaves the wait loop, acknowledges
`BRANCH`, and starts processing runtime DSP commands.

The CX75 DSP artifacts used to reproduce the problem are in
`bsp/rom/dsp/cx75`: `0602-container.bin`, `0602.bin`, `0602.dsp1`, and
`0602.txt`.

## SL98 DSP acknowledgements miss the ARM polling deadline

Status: fixed.

SL98 submits every DSP boot command by setting `DSP_COM_SET.bit0`, pulsing
`SCU_DSP_INT.bit0`, and reading `DSP_COM_STATUS` at most 1000 times. The ARM
asserts at `dwddrv/DSP/src/dsp.c:0x9B` if the DSP has not cleared the flag.
After boot, the startup code also expects runtime command `0x21` to be
acknowledged within two short ARM delay loops. Repeated failures eventually
assert at `dsp.c:0x119`.

GDB showed two distinct scheduling races. The first `PLOAD P:0006` could be
submitted while the worker was still resetting. The ARM then observed the old
runtime state instead of the queued communication flag. After that was fixed,
all boot commands completed but runtime command `0x21` could still miss its
deadline. PLOAD and DLOAD acknowledge from Mask ROM at `P:204B`. After BRANCH,
the loaded SL98 user ROM acknowledges runtime commands through `MCS_CFR`.

Tracing changes host scheduling enough to hide both races. Steady-state DSP
throughput also does not measure the cross-thread wake latency seen by a short
ARM polling loop.

The reset path now keeps communication flags and interrupt requests in a reset
queue until the worker has installed the new runtime state. A busy
`DSP_COM_STATUS` read waits for that one-time handoff instead of reading the
old state.

For normal commands, writing a request wakes the DSP worker immediately. A
busy status read gives the already runnable worker one bounded scheduling
handoff. Clearing `MCS_CFR` notifies that waiter immediately, so the ARM does
not wait for the rest of the DSP execution slice. This synchronization depends
only on the externally visible communication flag, not on command numbers,
firmware addresses, polling counts, or instruction offsets.

PLOAD only copies words while the Mask ROM dispatcher remains active; it does
not execute the modified P-space. P-space writes therefore skip TCG cache
invalidation until mutable P-space has actually started executing. Once it is
active, later writes invalidate translated mutable code normally. QEMU logs
the transition once as `cold program start` with the entry PC and current
communication flags.

The `dsp-l1mon-functional` test covers the boot and runtime command paths. A
full SL98 run without DSP tracing now passes both DSP assertions and reaches
the separate `l1co_mon.c:0x415` monitor-vector assertion.
