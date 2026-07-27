# PASIC PMIC families

PASIC is the common register superset for the Dialog Mozart and ST Twigo4 PMIC families. The register map was
reconstructed from the EL71 firmware and the reset dumps in `qemu/hw/arm/pmb887x/i2c/d1094xx.c`.

The common model is intentionally a superset. A register present in `PASIC.cfg` is not necessarily implemented by
every physical chip.

## Identification

`pasic_init` at `0xA04FA504` reads register `0x00`. The completion path decodes it as follows before
`pasic_init_registers` at `0xA057C960` applies family-specific initialization:

```c
model = id & 0x07;
revision = id >> 3 & 0x0f;
vendor = id >> 7;
signature = id & 0x87;
```

| Signature | Vendor | Model | Firmware family |
|---:|---|---:|---:|
| `0x84` | Dialog Mozart | 4 | 0 |
| `0x04` | ST Twigo4 | 4 | 1 |
| `0x85` | Dialog Mozart | 5 | 2 |
| `0x05` | ST Twigo4 | 5 | 3 |
| `0x86` | Dialog Mozart | 6 | 4 |
| `0x06` | ST Twigo4 | 6 | 5 |
| Other | Unknown | - | 15 |

The firmware uses the signature to select behavior. Revision remains a separate value and affects some D1094xx
register locations.

## Firmware API

The reconstructed high-level API names are kept in the Ghidra project and `lib/data/trace/EL71_errors.json`.

| Area | Functions |
|---|---|
| Initialization | `pasic_init` (`0xA04FA504`), `pasic_init_registers` (`0xA057C960`) |
| Generic access | `PASIC_SetRegister` (`0xA04FA434`), `PASIC_ReadReg` (`0xA04FCF44`) |
| Power | `PASIC_SetSupply` (`0xA04F9274`), `PASIC_SetVoltage` (`0xA04FB3E4`), `PASIC_PowerOff` (`0xA04FD51C`) |
| Status | `PASIC_ReadTurnoffReason` (`0xA04FD4E8`), `PASIC_ReadIrqStatus` (`0xA04FD6D4`), `PASIC_ReadChargeStatus` (`0xA01ABD78`) |
| Charger | `PASIC_SetChargeEnabled` (`0xA04FB79C`), `PASIC_SetChargeCurrent` (`0xA04FB884`) |
| Light and vibra | `PASIC_SetLightControl` (`0xA04FB628`), `PASIC_SetLight` (`0xA04FBA84`), `PASIC_SetSliMode` (`0xA04FB458`), `PASIC_SetVibra` (`0xA04FBC84`) |
| Audio levels | `PASIC_SetAmplifierGain` (`0xA04FBE78`), `PASIC_SetAdcSampleRate` (`0xA04FC7C0`), `PASIC_SetDacSampleRate` (`0xA04FC8D0`) |
| Audio routing | `PASIC_ControlPushPullAmplifier` (`0xA04FC06C`), `PASIC_SwitchMux` (`0xA04FC6B8`), `PASIC_SetMuxInput` (`0xA04FCB10`) |
| Audio paths | `PASIC_MonoControl` (`0xA04FCBA8`), `PASIC_StereoControl` (`0xA04FCCD0`), `PASIC_AdcControl` (`0xA04FCE14`), `PASIC_DacControl` (`0xA04FCEAC`) |
| Audio interfaces | `PASIC_ConfigureStereoMode` (`0xA04FC9E0`), `PASIC_I2sRxControl` (`0xA04FBD48`), `PASIC_I2sTxControl` (`0xA04FBDE0`), `PASIC_SetAuxAudioControl` (`0xA04FCA78`) |
| Tone | `PASIC_ClickControl` (`0xA04FC42C`) |
| Other blocks | `PASIC_DdpsControl` (`0xA04FCFCC`), `PASIC_SetOutportMode` (`0xA04FD084`), `PASIC_ControlResource` (`0xA04FD31C`) |

The direct register accesses group as follows:

| API path | Registers |
|---|---|
| Initialization | `00-0C`, `10`, `14-19`, `40-5A` |
| Generic register access | `00-19`, `3F-5D` |
| Supply control | `06-0B`, `14`, `4D` |
| Charger control and status | `10`, `11` |
| Interrupt handling | `01-04` |
| Light and vibra | `0C`, `12-19`, `47` |
| Audio | `40-5D` |
| DDPS | `20` |
| OUTPORT | `58` |

`PASIC_ReadReg` returns the firmware shadow byte without starting an I2C read. `pasic_write_reg` performs masked
updates where the firmware must preserve protected bits, notably in registers `0x03`, `0x0A`, and `0x10`.

## Known parts and dumps

| Part | ID | Model | Revision | Vendor | Firmware family |
|---|---:|---:|---:|---|---:|
| D1094BB | `0x9C` | 4 | 3 | Dialog Mozart | 0 |
| D1094DB | `0xFC` | 4 | 15 | Dialog Mozart | 0 |
| D1094EC | `0x94` | 4 | 2 | Dialog Mozart | 0 |
| D1094ED | `0x85` | 5 | 0 | Dialog Mozart | 2 |
| D1094ED | `0x25` | 5 | 4 | ST Twigo4 | 3 |
| D1601AA | `0x86` | 6 | 0 | Dialog Mozart | 4 |

IDs `0x04`, `0x05`, and `0x06` are recognized by the firmware, but no complete reset dumps for these exact IDs
are currently available. ID `0x25` was observed on hardware; it is not one of the saved emulator dumps.

Only registers which differ between the available 128-byte reset dumps are listed below.

| Register | D1094EC | D1094ED | D1601AA | D1094DB | D1094BB |
|---:|---:|---:|---:|---:|---:|
| `0x00` | `0x94` | `0x85` | `0x86` | `0xFC` | `0x9C` |
| `0x01` | `0x00` | `0x00` | `0x00` | `0x00` | `0x04` |
| `0x05` | `0x08` | `0x20` | `0x04` | `0x01` | `0x00` |
| `0x0A` | `0x0E` | `0x06` | `0x06` | `0x0E` | `0x0E` |
| `0x10` | `0x00` | `0x00` | `0x00` | `0x00` | `0x20` |
| `0x14` | `0x22` | `0x22` | `0x22` | `0x22` | `0x02` |
| `0x1D` | `0xA5` | `0xB3` | `0x82` | `0x77` | `0x7A` |
| `0x1E` | `0x44` | `0x80` | `0x45` | `0xD1` | `0x4A` |
| `0x1F` | `0x0C` | `0x0C` | `0x0C` | `0x04` | `0x00` |

## Interrupts

`pasic_irq_init` at `0xA04FA29C` installs a rising-edge handler on the PMIC interrupt input. The handler schedules
a high-level callback, and `PASIC_ReadIrqStatus` at `0xA04FD6D4` reads both status bytes:

| Register | Bit | Firmware interpretation |
|---:|---:|---|
| `IRQ_STATUS_1` (`0x01`) | 0 | Charger event, state 1 |
| | 2 | PMIC overtemperature |
| | 3 | Read `CHARGE_STATUS` |
| | 5 | Audio regulator A undervoltage |
| `IRQ_STATUS_2` (`0x02`) | 0 | SIM regulator A undervoltage |
| | 1 | Supply short circuit |
| | 2 | Unexpected charging |
| | 3 | Unknown event 4 |
| | 4 | Charger event, state 2 |
| | 5 | Battery overvoltage |

`IRQ_MASK_1` and `IRQ_MASK_2` use one mask bit per status bit; a set bit masks the corresponding interrupt.
`PASIC_ReadChargeStatus` temporarily unmasks `IRQ_STATUS_1.3`, reads register `0x11`, then restores the mask and
clears the cached status bit.

Both status registers are read for each interrupt. A set bit may therefore be a previously latched event rather
than the source of the current interrupt.

## Family differences

The D1094xx and D1601xx maps share the regulator, charger, light, audio, OUTPORT, and DDPS registers currently
known to the firmware. Confirmed differences are:

| Feature | D1094xx | D1601xx |
|---|---|---|
| VBOOST | Supply control at `0x07.2` | Integrated converter controlled at `0x07.2` |
| LED pattern bytes `0x15-0x19` | Not used by the firmware | Available |
| VLPREG enable | `0x07.4` on legacy revisions, otherwise `0x4D.6` | `0x4D.6` |

The EL71 firmware selects the legacy D1094xx VLPREG location for:

- family 0, revisions 9 and 15;
- family 1, revisions below 3.

All other recognized variants use `SWITCH_MUX_2.VLPREG_EN` at `0x4D.6`.

Families 4 and 5 support the five-byte LED1/LED2 pattern block. The observed firmware modes are:

| Mode | `LIGHT_CONTROL[1:0]` | `15 16 17 18 19` |
|---:|---:|---|
| 0 | `0` | `00 00 00 00 00` |
| 1 | `1` | `00 00 00 00 00` |
| 2 | `3` | `01 00 00 00 00` |
| 3 | `3` | `00 01 00 01 00` |

The bit order and timing of the pattern bytes are unknown. Dialog family 4 writes the pattern block during
initialization; ST family 5 initializes the shadow bytes without sending that write.

## Charge status

The firmware treats `CHARGE_STATUS` (`0x11`) as one raw byte:

```c
current_ma = raw < 0x1f ? 0 : ((raw >> 5) + 1) * 200;
```

This conversion does not establish a hardware bit-field boundary. Values `0x00-0x1E` and the unused low bits for
higher values remain undocumented.

## Metadata

- `D1094XX.cfg` describes the D1094xx family and generates its public header.
- `D1601XX.cfg` describes the D1601xx family and generates its public header.
- `PASIC.cfg` is the common superset used by the emulator and generic PASIC tools.

Use `PASIC.h` only when code must work with multiple PASIC families. Code targeting one known family should use
the matching family-specific header.
