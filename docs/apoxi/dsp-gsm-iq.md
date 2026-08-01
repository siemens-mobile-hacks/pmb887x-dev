# DSP HLE for minimal GSM emulation

For minimal GSM emulation, where verifying the TeakLite core is not the goal,
emulating every DSP instruction and the analog I/Q path is unnecessary. The
practical boundary is a high-level emulation of the complete DSP subsystem ABI
visible to the ARM firmware.

Mocking only Program Mask ROM is insufficient. After the `BRANCH` boot command,
the phone runs a loaded DSP startup firmware which implements the phone-specific
Shared RAM ABI, channel scheduling, interleaving, and result handling. Without a
DSP core, the HLE must replace this firmware behavior as well.

```text
ARM firmware
    |
    | Shared RAM, MCS, DSP_SRC
    v
DSP HLE
    |
    | decoded blocks or GSM bursts
    v
Virtual BTS
```

## DSP boot interface

Before `BRANCH`, the HLE must reproduce the externally observable DSP boot
contract:

- reset state and boot-ready indication;
- `PLOAD`, `DLOAD`, `PREAD`, and `DREAD`;
- `BRANCH` acknowledgement;
- runtime command pipes and their acknowledgements;
- MCS communication flags and semaphores;
- DSP-to-ARM interrupt sources.

The HLE may accept and store the uploaded DSP1 image without executing it. The
Mask ROM ID and DSP1 hash select a firmware ABI profile. Start with one exact
combination, such as PMB8876, Mask ROM `0801`, and the C81 DSP startup image.

## Runtime interface

After `BRANCH`, the HLE replaces the loaded DSP startup firmware and must:

- consume commands and parameters from Shared RAM;
- observe the TPU `FCON`, `SCON`, `MONON`, and `EQON` signals;
- update the firmware-specific result structures in Shared RAM;
- deliver the same MCS flags and DSP interrupts as hardware;
- schedule completion using QEMU virtual time rather than host sleeps;
- handle receive results and transmit requests asynchronously.

The HLE must not intercept DSP program counters or fabricate values in Shared
RAM read handlers. It should behave as an independent device which updates
memory and raises hardware signals at scheduled times.

## Virtual air-interface boundary

The preferred low-overhead interface is a GSM burst rather than analog I/Q:

```c
struct gsm_burst {
	uint32_t fn;
	uint16_t arfcn;
	uint8_t tn;
	uint8_t type;
	int8_t soft_bits[114];
	int16_t rssi;
	int16_t toa;
	int16_t cfo;
};
```

At this boundary the HLE implements:

- A5/x deciphering and ciphering;
- interleaving and deinterleaving;
- channel encoding and decoding;
- puncturing and depuncturing where required;
- CRC and FIRE checks;
- collection of logical blocks spanning four or eight bursts;
- conversion between GSM bursts and the selected Shared RAM ABI.

BB, ADC, CORDIC, FIR filtering, Equalizer processing, and analog I/Q generation
are omitted in this mode.

An even higher-level backend can exchange decoded 23-byte xCCH blocks. This is
the smallest implementation, but the HLE must then synthesize the timing and
metadata of the corresponding four bursts, including stealing flags, BER, RSSI,
timing offset, and completion events.

## Minimal network acquisition

The minimum flow required for the phone to find and camp on a virtual cell is:

1. On `FCON`, return a successful frequency-search result.
2. On `SCON`, return the virtual cell BSIC and GSM frame number.
3. On scheduled `EQON` windows, collect four virtual BCCH bursts.
4. Decode or directly supply a System Information block through the DSP receive
   result structure.
5. Raise the normal DSP-to-ARM completion interrupt.
6. On `MONON`, provide stable RXLEV and RXQUAL measurements.
7. Supply an otherwise empty CCCH.

Location Update requires bidirectional support:

1. Receive an uplink RACH request from the phone.
2. Return Immediate Assignment on AGCH.
3. Exchange SDCCH bursts and LAPDm blocks.
4. Implement the required MM authentication, cipher-mode, and location-update
   exchange.

Calls, SMS, and packet data add TCH, SACCH/FACCH, or GPRS/EDGE channel handling
on top of the same burst and Shared RAM interfaces.

## Receive and transmit paths

The downlink path is:

```text
Virtual BTS burst
    -> DSP HLE decode/deinterleave
    -> firmware-specific RX result in Shared RAM
    -> MCS flag or DSP_SRC interrupt
    -> ARM L1 firmware
```

The uplink path is the reverse:

```text
ARM L1 transmit request in Shared RAM
    -> DSP HLE channel encode/interleave/cipher
    -> uplink burst
    -> Virtual BTS
```

In core-emulation mode, uplink data would instead pass through DSP firmware and
Modulator RAM. HLE mode should consume the ARM-visible transmit ABI directly;
it does not need to generate modulator I/Q.

## Emulator modes

Keep accurate DSP emulation and fast GSM emulation as separate modes:

```text
-dsp-mode=core
    TeakLite core, Mask ROM, startup firmware, and DSP peripherals
    used for hardware verification and DSP regression tests

-dsp-mode=hle
    firmware ABI profile selected by Mask ROM ID and DSP1 hash
    used for fast phone boot and communication with a Virtual BTS
```

Shared RAM, MCS, SCU interrupt routing, and TPU signals remain common to both
modes. This keeps the ARM-visible hardware contract intact while avoiding DSP
instruction execution and high-rate analog I/Q simulation in HLE mode.

## Limitations

- Each supported DSP startup firmware needs its own verified ABI profile.
- Shared RAM layouts and command IDs must be recovered from hardware, firmware,
  and matching Mask ROM listings.
- Completion latency and interrupt ordering must use hardware measurements.
- ARM, TPU, and HLE events must share QEMU virtual time. Delays in Shared RAM or
  MMIO handlers are not a substitute for correct scheduling.
- HLE cannot validate TeakLite instructions, DSP peripherals, BB filtering,
  Equalizer behavior, or the original DSP firmware algorithms. Those remain the
  responsibility of `-dsp-mode=core` and hardware tests.
