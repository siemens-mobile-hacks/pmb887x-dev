# SL98 clock tree

SL98 uses PMB8876 (S-GOLD2). This document combines the SL98 firmware clock
manager, its CGU initialization, S-GOLD2 `MON_CR` signals, and measurements on
PMB8876 hardware. Firmware addresses refer to the SL98 fullflash loaded at
`0xA0000000`.

## Boot clock tree

```text
26 MHz oscillator
├── fSYS ........................................ 26 MHz
├── fSTM = oscillator / 8 ....................... 3.25 MHz
├── PLL: (NDIV + 1) / (MDIV + 1) = 4 / 1 ....... 104 MHz
│   ├── PLL / 2
│   │   ├── FPI1 / 2 ............................ 26 MHz
│   │   └── AHB_PER ............................. 52 MHz
│   ├── phase 1: 12 / (1 * 6 + 2) ............... 156 MHz
│   │   └── DSP
│   ├── phase 2: 12 / (1 * 6 + 0) ............... 208 MHz
│   │   ├── AHB
│   │   └── ARM ................................. 208 MHz
│   └── phase 4: 12 / (2 * 6 + 1) ............... 96 MHz
│       ├── CLK48M / 2 .......................... 48 MHz
│       └── MMCI/USIF ........................... 96 MHz
├── 6.5 MHz trigger domain
└── low-frequency sources
    ├── 406.25 kHz = 26 MHz / 64
    ├── 203.125 kHz = 26 MHz / 128
    └── 32 kHz
        ├── AFC standby clock, gated by `CGU_CON2.AFC32K_EN`
        └── external CLK32K output, gated by `CGU_CON2.CLK32K_EN`
```

The ARM divider field is programmed to `/2`, but its enable bit remains clear,
so the boot profile runs ARM directly from the 208 MHz AHB clock. Phase 3 is not
configured or powered by `cgu_initialize`.

## Boot register profile

`cgu_initialize` at `0xA05A85F4` builds this profile after waiting for
`CGU_STAT.LOCK`:

| Clock | `CGU.cfg` fields | Source and divider | Frequency |
|---|---|---|---:|
| PLL | `OSC.NDIV=3`, `OSC.MDIV=0` | oscillator × 4 | 104 MHz |
| PHASE1 | `CON0.PHASE1_K1=1`, `PHASE1_K2=2` | PLL × 12 / 8 | 156 MHz |
| PHASE2 | `CON0.PHASE2_K1=1`, `PHASE2_K2=0` | PLL × 12 / 6 | 208 MHz |
| PHASE4 | `CON0.PHASE4_K1=2`, `PHASE4_K2=1` | PLL × 12 / 13 | 96 MHz |
| fSYS | `CON1.FSYS_CLKSEL=BYPASS` | oscillator | 26 MHz |
| fSTM | `CON1.FSTM_DIV_EN=1`, `FSTM_DIV=8` | oscillator / 8 | 3.25 MHz |
| AHB | `CON1.AHB_CLKSEL=PHASE2` | phase 2 | 208 MHz |
| ARM | `CON2.CPU_DIV_EN=0` | AHB | 208 MHz |
| EBU | `CON2.EBU_CLKSEL=PLL` | PLL | 104 MHz |
| DSP | `CON2.DSP_CLKSEL=PHASE1` | phase 1 | 156 MHz |
| FPI1 | `CON1.FPI1_CLKSEL=PLL_DIV_2`, `FPI1_CLKDIV=DIV2` | PLL / 2, then / 2 | 26 MHz |
| AHB_PER | `CON3.AHB_PER_CLKSEL=PLL_DIV_2`, `AHB_PER_CLKDIV=DIV1` | PLL / 2 | 52 MHz |
| CLK48M / USB/FIRDA | `CON2.CLK48M_CLKSEL=PHASE4`, `CON3.CLK48M_CLKDIV=DIV2` | phase 4 / 2 | 48 MHz |
| MMCI/USIF | `CON3.MMCI_CLKSEL=PHASE4`, `MMCI_CLKDIV=DIV1` | phase 4 | 96 MHz |
| DMA | `CON3.DMA_CLK_DISABLE=0` | PLL | 104 MHz |
| Mixed signal (`CLK_MS_O`) | `CON2[29:28]=2` | oscillator / 64 | 406.25 kHz |

`CLKOUT0`, `CLKOUT1`, and `CLKOUT2` are disabled. The 32 kHz output,
mixed-signal clock, and USB/FIRDA clock are enabled.

The corresponding physical monitor names are `CLK_PLL_I`, `CLK_PHS1_I` through
`CLK_PHS4_I`, `CLK_DSP_O`, `CLK_EBU_O`, `CLK_FPI1_O`, `CLK_AHB_PER_O`,
`CLK_48M_O`, `CLK_MMCI_O`, and `CLK_MS_O`. They are the names exposed by the
PMB8876 CGU monitor mux; `CGU.cfg` names the selectors and dividers that feed
those signals.

## Managed clock domains

The firmware represents special CGU outputs as domains `0x19..0x24`.
`clock_client_set_state` (`0xA0555C38`) reference-counts clients and eventually
calls `clock_domain_apply_state` (`0xA0555A8C`).

| ID | Clock | CGU field | Sources or rates |
|---:|---|---|---|
| `0x19` | AHB | `CON1[22:20]` | oscillator, 32 kHz, 406.25 kHz, or phases 1..4 |
| `0x1A` | AFC standby clock | `CON2[13]` | off / 32 kHz |
| `0x1B` | GSM trigger clock | no writable selector found | fixed 6.5 MHz |
| `0x1C` | CLKOUT0 | `CON2[18:16]` | 26 MHz divided by 1, 2, 4, or 8 |
| `0x1D` | CLKOUT1 | `CON2[22:20]` | 26 MHz divided by 1, 2, 4, or 8 |
| `0x1E` | CLKOUT2 | `CON3[21:16]` | phase 4 divided by 1, 2, 4, or 8 |
| `0x1F` | USB/FIRDA | `CON2[15:14]`, `CON3[25:24]` | oscillator or phase 4, divided by 1, 2, 4, or 8 |
| `0x20` | CLK32K output | `CON2[24]` | off / 32 kHz |
| `0x21` | MMCI/USIF | `CON3[13:8]` | oscillator, 32 kHz, or phase 4, divided by 1, 2, 4, or 8 |
| `0x22` | shared low-frequency source | two-bit field selected through a descriptor | 26 MHz, 32 kHz, 406.25 kHz, or off |
| `0x23` | `CLK_MS_O` | `CON2[29:28]` | 26 MHz, 32 kHz, 406.25 kHz, or off |
| `0x24` | unresolved manager-only domain | no direct CGU access found | unresolved |

The AFC block does not calculate the frequency error. DSP software measures the
error and writes `AFC_AFCVAL`; the AFC hardware converts that value into a PNM
output whose low-pass-filtered average controls the external VCXO. Regular PNM
generation uses 26 MHz. `AFC32K_EN` only supplies a slower clock during standby
so the already programmed correction voltage can be maintained while the
normal clock is stopped. In normal mode this bit does not replace the 26 MHz
AFC clock.

Relevant firmware functions:

- `clock_client_set_state(client_id, clock_id, frequency_hz, enabled)` —
  `0xA0555C38`, public reference-counted request path;
- `clock_domain_descriptor_get(clock_id)` — `0xA0554F88`;
- `clock_domain_refresh_state(clock_id, ...)` — `0xA0554FCC`;
- `clock_domain_apply_state(clock_id, enabled, frequency_hz, ...)` —
  `0xA0555A8C`;
- `clock_configure_mmc_usif` — `0xA0555590`;
- `clock_configure_usb_firda` — `0xA0556578`;
- `clock_configure_clkout2` — `0xA0556674`;
- `clock_configure_shared_source` — `0xA05564A0`.

## Hardware confirmation

- FPI1 and AHB_PER selector 2 track `fPLL / 2`, confirmed through peripheral
  timing while changing the PLL frequency.
- Phase 3 and phase 4 are independent working phase outputs, confirmed through
  EBU timing.
- `CON2[13]` has no visible effect in normal operation. In `TCXO_OFF`, setting
  it leaves `CLK_AFC_O` active at the standby rate; clearing it stops the
  signal. It is therefore `AFC32K_EN`, not a general peripheral clock selector.
- `CON2[29:28]` controls `CLK_MS_O`. The SL98 rate table is selector 0 = 26 MHz,
  1 = 32 kHz, 2 = 406.25 kHz, 3 = off.
- `CLK_6M5_TRIG_O` produces half as many sampled transitions as `CLK_AFC_O` in
  normal mode, consistent with a 6.5 MHz trigger clock derived from 13 MHz.

Absolute high-frequency values above come from the firmware's divider setup
and clock tables. `MON_CR` GPIO sampling confirms presence, gating, and large
ratios, but aliases high-frequency clocks and is not an absolute frequency
counter.
