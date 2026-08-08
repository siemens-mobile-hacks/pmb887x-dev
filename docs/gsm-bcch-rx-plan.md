# RX-only BCCH plan

## Constraints

- Keep the complete path RX-only: no `TXON`, `CODON`, transmit DSP commands,
  power-amplifier control, or uplink RF output.
- Preserve the current RSSI -> FCCH -> SCH pipeline and expose BCCH as the
  next independent L1 operation.

## Known baseline

- FCCH supplies the frequency correction and SCH supplies BSIC plus the GSM
  frame number.
- EL71 v45 starts control-channel reception with DSP Mask ROM 0801 command 7
  (`CCH_RX`). Its seven parameters are `CIPH`, `TSC`, `EARLY`, and four
  command-specific values.
- The EL71 v45 wrapper is at `0xA0A74EAC`; its default caller wrapper is at
  `0xA0A74EEE`.

## Implementation

1. Recover the exact EL71 v45 `CCH_RX` parameters, shared-memory result layout,
   TPU TS0 normal-burst schedule, and completion flags.
2. Add a reusable `gsm_l1_decode_bcch()` operation and a result structure. Keep
   TAP output and System Information parsing outside the L1 core.
3. Receive the complete four-burst BCCH block and accept it only after the DSP
   channel decoder reports a valid FIRE/CRC result.
4. Parse the common System Information header and SI3 fields needed to identify
   the cell: MCC, MNC, LAC, Cell ID, and control-channel description.
5. Run BCCH only for candidates with valid SCH, retry within a bounded number
   of multiframes, and report only CRC-valid blocks.

## Verification

- Audit every new TPU event table and DSP command path for the RX-only
  constraint.
- Validate on real PMB8876 hardware against cells already confirmed by SCH.
- Add emulator coverage only after the hardware sequence and result layout are
  fixed; QEMU output is not the hardware specification.
