# PMB8876 SIM tests

`sim` covers reset values, the PrimeCell-style interrupt block, and VIC routing.
It does not require a card.

`sim-dma` covers DMA request 8 in the verified PER2MEM direction. A software-set `OK` event
produces a request only when `DMAE.OK` is already enabled; enabling DMA afterwards does not
replay the old event.

`sim-card` requires an inserted SIM card and a Siemens board with the PMIC at SMBus address
`0x31`. It enables the SIM regulator through PMIC register `0x06`, bit 4, and restores the
previous value on exit. The test is read-only: it receives ATR, selects MF and EF_ICCID, reads
their responses, records the observed clear `STAT.SIMDET` state, and reads ten bytes from
EF_ICCID without logging the identifier.

`sim-card-dma` requires the same card and PMIC setup. After receiving ATR in character mode, it
switches to hardware T=0 and performs SELECT MF through a two-item DMA chain. It verifies the
five-byte header, the procedure-byte pause, the two-byte data phase, `STAT.T0END`, and `SW1/SW2`.

`sim-card-timers` verifies that `CHTIMER=1` raises `STAT.CHTIMEOUT` while hardware T=0 waits for
the next character. On the tested C81 it expired in 129–218 microseconds across repeated runs.
The BWT case is skipped because the available SIM advertises only T=0; it requires a T=1 card or
card emulator.

`sim-no-card` requires an empty SIM slot. It verifies that `STAT.SIMDET` remains clear and card
activation produces no ATR. On the tested Siemens C81, `SIMDET` stays clear both with and without
a card. EL71 firmware likewise does not inspect this bit; it identifies a missing card when ATR
activation times out.

`sim-card-info` builds the same read-only exchange with explicit identity diagnostics enabled.
It logs every APDU and response in raw hex with `SW1/SW2`, including dynamic-length MF and DF_GSM
GET RESPONSE commands and DF_GSM STATUS. It also dumps and decodes ICCID, verifies its Luhn check
digit, reads IMSI and splits it into MCC/MNC/MSIN, reads SPN, and tries the first populated MSISDN
record. Its output contains SIM identifiers and a phone number, so it should only be used for
intentional local diagnostics.

## Hardware T=0 notes

EL71 firmware uses the following sequence:

1. Set `CON.UARTON` and `CON.SIMT0`.
2. Enable `IRQEN.ENT0END`, disable per-character `ENOKINT`, and enable `DMAE.OK`.
3. Transfer the five-byte APDU header to `TXB` through DMA request 8.
4. Write `INS` and `P3` from the DMA start callback immediately before enabling the channel.
5. Run the data phase through the same request; `INS.INSDIR` selects its direction.
6. Read the final status from `SW1` and `SW2` after `STAT.T0END`.

The firmware DMA resource table confirms request 8 for both `TXB` and `RXB`. EL71, VS7, and SL98
all use the same hardware T=0 layout. On a Siemens C81 with PMB8876 rev. 10, synthetic `OK` events
reliably generate PER2MEM requests. A standalone interface without card activation does not emit
the initial MEM2PER request. After power-on and ATR, `sim-card-dma` confirms that the activated
interface emits request 8 and completes the five-byte MEM2PER header without a DMA bus error.
