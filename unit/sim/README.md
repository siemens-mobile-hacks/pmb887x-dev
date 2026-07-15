# PMB8876 SIM tests

`sim` covers reset values, write masks, the PrimeCell-style interrupt block, and VIC routing.
It does not require a card.

`sim-dma` covers DMA request 8 in the verified PER2MEM direction. A software-set `OK` event
produces a request only when `DMAE.OK` is already enabled; enabling DMA afterwards does not
replay the old event.

`sim-card` requires an inserted SIM card and a Siemens board with the PMIC at SMBus address
`0x31`. It enables the SIM regulator through PMIC register `0x06`, bit 4, and restores the
previous value on exit. The test is read-only: it receives ATR, selects MF and EF_ICCID, reads
their responses, and reads ten bytes from EF_ICCID without logging the identifier.

`sim-card-info` builds the same read-only exchange with explicit identity diagnostics enabled.
It dumps and decodes ICCID, verifies its Luhn check digit, reads IMSI and splits it into
MCC/MNC/MSIN, reads SPN, and tries the first populated MSISDN record. Its output contains SIM
identifiers and a phone number, so it should only be used for intentional local diagnostics.

## Hardware T=0 notes

EL71 firmware uses the following sequence:

1. Set `CON.UARTON` and `CON.SIMT0`.
2. Enable `IRQEN.ENT0END`, disable per-character `ENOKINT`, and enable `DMAE.OK`.
3. Transfer the five-byte APDU header to `TXB` through DMA request 8.
4. In the header DMA completion callback, write `INS` and `P3`.
5. Run the data phase through the same request; `INS.INSDIR` selects its direction.
6. Read the final status from `SW1` and `SW2` after `STAT.T0END`.

The firmware DMA resource table confirms request 8 for both `TXB` and `RXB`. On a Siemens C81
with PMB8876 rev. 10, synthetic `OK` events reliably generate PER2MEM requests. Direct unit-test
attempts did not observe the MEM2PER request during the hardware T=0 header phase, even though
character-mode APDUs work. Keep hardware T=0 TX out of mandatory tests until its missing start
condition is identified on real hardware.
