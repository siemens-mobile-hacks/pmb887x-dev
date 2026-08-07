# MonSGold monitor signal tables

Source: `Siemens/QCOM/qcom_x85_tools/x85/monsgold.exe`, SHA-256
`03b86763d63e347d844acbdeb41c9bed5457508c5155a5d0a03aaae273571b3f`.

The executable calls the older SGOLD family `S-GOLD Lite`; this document keeps
that exact family name. The tables below preserve the source order and contain
every block, subgroup, signal, and exact 16-bit value written to `MON_CR`.

For normal signal selections, `MON_CR[11:8]` is the block ID and
`MON_CR[7:0]` is the signal selector. `INPUT-MODE` is the special value
`0x8000`. For example, both families contain `CGU / N/A / CLK_32K_I ->
MON_CR=0x0802`.

S-GOLD2 contains one exact alias in the source table: `MON_CR=0x0079` is
assigned to both `mon_gclk_mix_gmsk` and `mon_gclk_mix_psk` in
`DSP Block1 / DSP Miscellaneous`. No other duplicate `MON_CR` values occur
within either family.

## Supported chip revisions

| Family | Chip ID | Version |
|---|---:|---|
| S-GOLD2 | `0x10` | S-Gold2 V1.0 |
| S-GOLD Lite | `0x0D` | S-Gold Lite V1.1 |
| S-GOLD Lite | `0x0E` | S-Gold Lite V1.1a |
| S-GOLD Lite | `0x0F` | S-Gold Lite V1.1b |

## Block summary

| Family | Block | Block ID | Subgroups | Signals |
|---|---|---:|---:|---:|
| S-GOLD2 | AHB_PER 1 | `0x05` | 8 | 70 |
| S-GOLD2 | AHB_PER 2 | `0x06` | 5 | 115 |
| S-GOLD2 | CGU | `0x08` | 1 | 29 |
| S-GOLD2 | DSP Block1 | `0x00` | 10 | 117 |
| S-GOLD2 | DSP Block2 | `0x02` | 9 | 113 |
| S-GOLD2 | FPI1 | `0x04` | 6 | 66 |
| S-GOLD2 | FPI21 | `0x0A` | 9 | 86 |
| S-GOLD2 | FPI22 | `0x0B` | 5 | 122 |
| S-GOLD2 | FPI3 | `0x07` | 5 | 93 |
| S-GOLD2 | LMU | `0x0F` | 5 | 47 |
| S-GOLD2 | ML-AHB | `0x0E` | 10 | 111 |
| S-GOLD2 | SCU | `0x09` | 3 | 102 |
| S-GOLD2 | INPUT-MODE | `0x00` | 1 | 1 |
| S-GOLD Lite | CGU | `0x08` | 1 | 24 |
| S-GOLD Lite | DSP | `0x00` | 5 | 58 |
| S-GOLD Lite | DSP_PER | `0x02` | 13 | 125 |
| S-GOLD Lite | FPI1 | `0x04` | 7 | 75 |
| S-GOLD Lite | FPI21 | `0x0A` | 8 | 91 |
| S-GOLD Lite | FPI22 | `0x0B` | 1 | 77 |
| S-GOLD Lite | FPI3 | `0x07` | 5 | 83 |
| S-GOLD Lite | LMU | `0x0F` | 2 | 47 |
| S-GOLD Lite | ML-AHB | `0x0E` | 1 | 33 |
| S-GOLD Lite | SCU | `0x09` | 3 | 85 |
| S-GOLD Lite | INPUT-MODE | `0x00` | 1 | 1 |

## PMB8876 CGU findings

Verified on a C81 with `unit/cgu` and `unit/gpio/monitor`. High-frequency
`MON_CR` transition counts are aliases of the GPIO sampling loop and are useful
for detecting clock presence, gating, and large ratios, but not for measuring
an absolute frequency. Functional peripheral timing is used where available.

| Field or signal | Result |
|---|---|
| `PHASE3` | A real PLL phase output. With `fPLL=104 MHz` and `K1=4, K2=3`, EBU reads through this source took 2,580,806 STM ticks and returned the same checksum as every other EBU source. The divider formula gives 46.222 MHz. |
| `PHASE4` | A real PLL phase output. With `fPLL=104 MHz` and `K1=5, K2=0`, EBU reads through this source took 2,792,117 STM ticks and returned the same checksum as every other EBU source. The divider formula gives 41.6 MHz. |
| `FPI1_CLKSEL=PLL_DIV_2` | A separate `fPLL / 2` source, not `fSYS`. With CPU, AHB, and `fSYS` held on the 26 MHz oscillator, FPI1 loopback took 19,938, 15,926, and 13,270 STM ticks at PLL frequencies of 104, 130, and 156 MHz. These ratios track `fPLL` precisely. At 104 MHz PLL, SL98's source table maps selector 2 to 52,000,000 Hz and its CGU initialization selects it for PDBUS/FPI1. |
| `AHB_PER_CLKSEL=PLL_DIV_2` | The same selector layout as FPI1. At PLL frequencies of 78, 91, and 104 MHz, 32,768 MMICIF reads took 655,411, 622,643, and 589,875 STM ticks. Each 13 MHz PLL step removed exactly one STM wait-state per read, directly confirming the PLL dependency despite quantized bridge latency. EL71 names the normal 104 MHz PLL configuration 52 MHz, and SL98 selects source 2 for XPER/AHB_PER. |
| `CGU_CTL5[13]` (`AFC32K_EN`) | SL98 exposes this bit as clock-domain ID `0x1A` and reports 32 kHz while it is set. On C81 hardware it had no visible effect in normal operation, but in `TCXO_OFF` it gated `CLK_AFC_O`: set produced transitions and clear produced none. It enables the AFC standby clock, not a general peripheral source mux. |
| `CGU_CTL5[29:28]` (`MS_CLKSEL`) | `MS` means mixed signal. SL98 exposes this field as clock-domain ID `0x23`, and `MON_CR` identifies its output as `CLK_MS_O`. The firmware rate table maps selector 0 to 26 MHz, 1 to 32 kHz, 2 to 406.25 kHz, and 3 to off. EL71 writes `CGU_CTL5=0x10004127` during early CGU initialization, selecting 32 kHz, but has no dedicated runtime client for this field. PMB7870 documents `clk_ms` as the analog master clock from which peripherals derive clocks including `clk_meas`, `clk_kernel`, `clk_bbrx`, and `clk_bbtx`.
| `CLK_AFC_O` | An upstream CGU clock. In normal mode it remained active with `AFC_CLC` disabled and did not change after enabling `ENAFC`; in `TCXO_OFF`, `AFC32K_EN` selected whether it continued from the 32 kHz standby source. |
| `CLK_6M5_TRIG_O` | An upstream CGU clock, not the TPU module-gated clock: it remained active with `TPU_CLC` disabled and did not change after programming `TPU_GSMCLK1/2/3`. The observed transition count was consistently half of `CLK_AFC_O`, matching the signal's 6.5 MHz name but not constituting an absolute frequency measurement. |
| `CLK_CON_O` | Remained active and unchanged while selecting PLL-backed `fSYS` and toggling `CGU_CTL5[13]`; its exact consumer and source are not yet identified. |
| `EN_CERB_O` | A level signal rather than a clock. It stayed high throughout the same source-selection probes; its owning enable path is not yet identified. |

Firmware corroboration comes from EL71 `clock_initialize_domains`,
`clock_enable_fpi1_52mhz`, and `clock_enable_ahb_per_52mhz`, plus the SL98 clock
dispatcher. The hardware observations above take precedence where a firmware
name and measured behavior do not yet agree.

## S-GOLD2

Total: 13 blocks, 1072 signals.

### AHB_PER 1 (`block_id=0x05`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Camera IF | CIF_ERR_IRQ | `0x0510` |
| Camera IF | CIF_FRAME_IRQ | `0x050F` |
| Camera IF | CIF_HBUSREQ_CIF | `0x0579` |
| Camera IF | CIF_HGRANT | `0x057A` |
| Camera IF | CIF_HREADYOUT | `0x055A` |
| Camera IF | CIF_HRESP(0) | `0x055B` |
| Camera IF | CIF_HRESP(1) | `0x055C` |
| Camera IF | CIF_HSEL | `0x0559` |
| Camera IF | CIF_HTRANS_CIF(0) | `0x057C` |
| Camera IF | CIF_HTRANS_CIF(1) | `0x057D` |
| Camera IF | CIF_HWRITE_CIF | `0x057B` |
| Camera IF | CIF_MBL_IRQ | `0x050E` |
| Camera IF | CIF_VSYNC_STAT | `0x050D` |
| Display IF | DIF_HREADYOUT | `0x055E` |
| Display IF | DIF_HRESP(0) | `0x055F` |
| Display IF | DIF_HRESP(1) | `0x0560` |
| Display IF | DIF_HSEL | `0x055D` |
| FCDP | FCDP_DATCMD_IRQ | `0x0541` |
| FCDP | FCDP_ECC_IRQ | `0x0542` |
| FCDP | FCDP_ERR_IRQ | `0x0544` |
| FCDP | FCDP_HBURSTM(0) | `0x053B` |
| FCDP | FCDP_HBURSTM(1) | `0x053C` |
| FCDP | FCDP_HBURSTM(2) | `0x053D` |
| FCDP | FCDP_HPROTM(0) | `0x0534` |
| FCDP | FCDP_HPROTM(1) | `0x0535` |
| FCDP | FCDP_HPROTM(2) | `0x0536` |
| FCDP | FCDP_HPROTM(3) | `0x0537` |
| FCDP | FCDP_HREADY | `0x0531` |
| FCDP | FCDP_HREADY | `0x0540` |
| FCDP | FCDP_HREADYOUT | `0x056A` |
| FCDP | FCDP_HRESP(0) | `0x053F` |
| FCDP | FCDP_HRESP(0) | `0x056B` |
| FCDP | FCDP_HRESP(1) | `0x056C` |
| FCDP | FCDP_HSEL | `0x0569` |
| FCDP | FCDP_HSIZEM(0) | `0x0538` |
| FCDP | FCDP_HSIZEM(1) | `0x0539` |
| FCDP | FCDP_HSIZEM(2) | `0x053A` |
| FCDP | FCDP_HTRANSM(0) | `0x0532` |
| FCDP | FCDP_HTRANSM(1) | `0x0533` |
| FCDP | FCDP_HWRITEM | `0x053E` |
| FCDP | FCDP_KERNEL_CLK_MON | `0x052F` |
| FCDP | FCDP_ST_IRQ | `0x0543` |
| Fast IrDA | FIRDA_HREADYOUT | `0x056E` |
| Fast IrDA | FIRDA_HRESP(0) | `0x056F` |
| Fast IrDA | FIRDA_HRESP(1) | `0x0570` |
| Fast IrDA | FIRDA_HSEL | `0x056D` |
| I2C | I2C_HREADYOUT | `0x0576` |
| I2C | I2C_HRESP(0) | `0x0577` |
| I2C | I2C_HRESP(1) | `0x0578` |
| I2C | I2C_HSEL | `0x0575` |
| MMC | MMCI_INT0_INT | `0x0551` |
| MMC | MMCI_INT1_INT | `0x0552` |
| MMC | MMCI_SDIO_INT | `0x0553` |
| MMC | MMC_HREADYOUT | `0x0562` |
| MMC | MMC_HRESP(0) | `0x0563` |
| MMC | MMC_HRESP(1) | `0x0564` |
| MMC | MMC_HSEL | `0x0561` |
| MMIF | MMIF_BREQ_IRQ | `0x051F` |
| MMIF | MMIF_ERR_IRQ | `0x0520` |
| MMIF | MMIF_HREADYOUT | `0x0566` |
| MMIF | MMIF_HRESP(0) | `0x0567` |
| MMIF | MMIF_HRESP(1) | `0x0568` |
| MMIF | MMIF_HSEL | `0x0565` |
| MMIF | MMIF_LBREQ_IRQ | `0x051E` |
| MMIF | MMIF_MMIF_LSREQ_IRQ | `0x051C` |
| MMIF | MMIF_MMIF_SREQ_IRQ | `0x051D` |
| USIF | USIF_HREADYOUT | `0x0572` |
| USIF | USIF_HRESP(0) | `0x0573` |
| USIF | USIF_HRESP(1) | `0x0574` |
| USIF | USIF_HSEL | `0x0571` |

### AHB_PER 2 (`block_id=0x06`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| AHB | AHB_PER_BIF_HRESP(0) | `0x060B` |
| AHB | AHB_PER_BIF_HRESP(1) | `0x060C` |
| AHB | AHB_PER_HBURST(0) | `0x0606` |
| AHB | AHB_PER_HBURST(1) | `0x0607` |
| AHB | AHB_PER_HBURST(2) | `0x0608` |
| AHB | AHB_PER_HREADY | `0x060A` |
| AHB | AHB_PER_HSIZE(0) | `0x0603` |
| AHB | AHB_PER_HSIZE(1) | `0x0604` |
| AHB | AHB_PER_HSIZE(2) | `0x0605` |
| AHB | AHB_PER_HTRANS(0) | `0x0601` |
| AHB | AHB_PER_HTRANS(1) | `0x0602` |
| AHB | AHB_PER_HWRITE | `0x0609` |
| Display IF | DIF_IRQ(0) | `0x0611` |
| Display IF | DIF_IRQ(1) | `0x0612` |
| Display IF | DIF_IRQ(2) | `0x0613` |
| Display IF | DIF_IRQ(3) | `0x0614` |
| Display IF | DIF_IRQ(4) | `0x0615` |
| Display IF | DIF_IRQ(5) | `0x0616` |
| Display IF | DIF_IRQ(6) | `0x0617` |
| Display IF | DIF_IRQ(7) | `0x0618` |
| Display IF | DIF_IRQ(8) | `0x0619` |
| Fast IrDA | FIRDA_EORXP_ACK | `0x0628` |
| Fast IrDA | FIRDA_EORXP_IND | `0x0627` |
| Fast IrDA | FIRDA_EOXTP_IND | `0x0626` |
| Fast IrDA | FIRDA_FD_IRQ | `0x062F` |
| Fast IrDA | FIRDA_FIFO_FLUSH | `0x0638` |
| Fast IrDA | FIRDA_FI_IRQ | `0x0633` |
| Fast IrDA | FIRDA_FT_IRQ | `0x0630` |
| Fast IrDA | FIRDA_IRQ(0) | `0x0629` |
| Fast IrDA | FIRDA_IRQ(1) | `0x062A` |
| Fast IrDA | FIRDA_IRQ(2) | `0x062B` |
| Fast IrDA | FIRDA_IRQ(3) | `0x062C` |
| Fast IrDA | FIRDA_KERNEL_CLOCK | `0x063B` |
| Fast IrDA | FIRDA_MBT_IRQ | `0x0631` |
| Fast IrDA | FIRDA_MTAT_IRQ | `0x0632` |
| Fast IrDA | FIRDA_REQCLR | `0x062D` |
| Fast IrDA | FIRDA_RUN_ACK | `0x063A` |
| Fast IrDA | FIRDA_RUN_REQ | `0x0639` |
| Fast IrDA | FIRDA_RXF_OFL_IRQ | `0x0635` |
| Fast IrDA | FIRDA_RXF_UFL_IRQ | `0x0634` |
| Fast IrDA | FIRDA_RXS_SET | `0x0625` |
| Fast IrDA | FIRDA_RX_DATA_IND | `0x0624` |
| Fast IrDA | FIRDA_RX_FIFO_RDY | `0x0623` |
| Fast IrDA | FIRDA_SD_IRQ | `0x062E` |
| Fast IrDA | FIRDA_TXF_OFL_IRQ | `0x0637` |
| Fast IrDA | FIRDA_TXF_UFL_IRQ | `0x0636` |
| Fast IrDA | FIRDA_TX_DATA_ACK | `0x0622` |
| Fast IrDA | FIRDA_TX_DATA_IND | `0x0621` |
| I2C | I2C_AL_INT | `0x0676` |
| I2C | I2C_AM_INT | `0x0673` |
| I2C | I2C_DATA_IND | `0x0669` |
| I2C | I2C_EORXP_ACK | `0x066D` |
| I2C | I2C_EORXP_IND | `0x066C` |
| I2C | I2C_EOXTP_IND | `0x066B` |
| I2C | I2C_FIFO_FLUSH | `0x067F` |
| I2C | I2C_GCINT | `0x0674` |
| I2C | I2C_IRQ(0) | `0x066E` |
| I2C | I2C_IRQ(1) | `0x066F` |
| I2C | I2C_IRQ(2) | `0x0670` |
| I2C | I2C_IRQ(3) | `0x0671` |
| I2C | I2C_MC_INT | `0x0675` |
| I2C | I2C_NACK_INT | `0x0677` |
| I2C | I2C_REQCLR(0) | `0x0672` |
| I2C | I2C_RUN_ACK | `0x067A` |
| I2C | I2C_RUN_REQ | `0x0679` |
| I2C | I2C_RXS_SET | `0x066A` |
| I2C | I2C_RX_FIFO_RDY | `0x0668` |
| I2C | I2C_RX_OFL_IRQ | `0x067C` |
| I2C | I2C_RX_UFL_IRQ | `0x067B` |
| I2C | I2C_SCL_IN | `0x0662` |
| I2C | I2C_SCL_OUT | `0x0664` |
| I2C | I2C_SDA_IN | `0x0663` |
| I2C | I2C_SDA_OUT | `0x0665` |
| I2C | I2C_TC_END_INT | `0x0678` |
| I2C | I2C_TX_DATA_ACK | `0x0667` |
| I2C | I2C_TX_DATA_IND | `0x0666` |
| I2C | I2C_TX_OFL_IRQ | `0x067E` |
| I2C | I2C_TX_UFL_IRQ | `0x067D` |
| USIF | USIF_AB_IRQ | `0x065C` |
| USIF | USIF_EORXP_ACK | `0x0644` |
| USIF | USIF_EORXP_IND | `0x0643` |
| USIF | USIF_EOXTP_IND | `0x0642` |
| USIF | USIF_ERR_BRE_IRQ | `0x065D` |
| USIF | USIF_ERR_FE_IRQ | `0x065F` |
| USIF | USIF_ERR_PE_IRQ | `0x0660` |
| USIF | USIF_ERR_PHE_IRQ | `0x065E` |
| USIF | USIF_IRQ(0) | `0x0645` |
| USIF | USIF_IRQ(1) | `0x0646` |
| USIF | USIF_IRQ(10) | `0x064F` |
| USIF | USIF_IRQ(2) | `0x0647` |
| USIF | USIF_IRQ(3) | `0x0648` |
| USIF | USIF_IRQ(4) | `0x0649` |
| USIF | USIF_IRQ(5) | `0x064A` |
| USIF | USIF_IRQ(6) | `0x064B` |
| USIF | USIF_IRQ(7) | `0x064C` |
| USIF | USIF_IRQ(8) | `0x064D` |
| USIF | USIF_IRQ(9) | `0x064E` |
| USIF | USIF_KERNEL_CLOCK | `0x0661` |
| USIF | USIF_MRPS_START | `0x0658` |
| USIF | USIF_MRPS_STOP | `0x0659` |
| USIF | USIF_REQCLR(0) | `0x0650` |
| USIF | USIF_REQCLR(1) | `0x0651` |
| USIF | USIF_RUN_ACK | `0x0657` |
| USIF | USIF_RUN_REQ | `0x0656` |
| USIF | USIF_RXF_OFL | `0x0654` |
| USIF | USIF_RXF_UFL | `0x0655` |
| USIF | USIF_RXS | `0x065B` |
| USIF | USIF_RXS_SET | `0x0641` |
| USIF | USIF_RX_DATA_IND | `0x0640` |
| USIF | USIF_RX_FIFO_RDY | `0x063F` |
| USIF | USIF_TXF_OFL | `0x0652` |
| USIF | USIF_TXF_UFL | `0x0653` |
| USIF | USIF_TXS_TXACT | `0x065A` |
| USIF | USIF_TX_DATA_ACK | `0x063E` |
| USIF | USIF_TX_DATA_IND | `0x063D` |

### CGU (`block_id=0x08`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| N/A | CLK32KOUT_O | `0x0817` |
| N/A | CLK_104M_O | `0x081C` |
| N/A | CLK_32K_I | `0x0802` |
| N/A | CLK_48M_O | `0x080D` |
| N/A | CLK_6M5_TRIG_O | `0x081A` |
| N/A | CLK_AFC_O | `0x0813` |
| N/A | CLK_AHB_PER_O | `0x081B` |
| N/A | CLK_CLKOUT0_O | `0x0815` |
| N/A | CLK_CLKOUT1_O | `0x0816` |
| N/A | CLK_CLKOUT2_O | `0x081D` |
| N/A | CLK_CON_O | `0x0809` |
| N/A | CLK_DSP_O | `0x0808` |
| N/A | CLK_EBU_O | `0x080C` |
| N/A | CLK_FPI1_O | `0x080E` |
| N/A | CLK_FPI2_O | `0x0812` |
| N/A | CLK_FPI3_O | `0x0810` |
| N/A | CLK_IN_1 | `0x0801` |
| N/A | CLK_MMCI_O | `0x0819` |
| N/A | CLK_MS_O | `0x0814` |
| N/A | CLK_PHS1_I | `0x0804` |
| N/A | CLK_PHS2_I | `0x0805` |
| N/A | CLK_PHS3_I | `0x0806` |
| N/A | CLK_PHS4_I | `0x0807` |
| N/A | CLK_PLL_I | `0x0803` |
| N/A | EN_AHB_O | `0x080B` |
| N/A | EN_ARM_O | `0x080A` |
| N/A | EN_CERB_O | `0x081E` |
| N/A | EN_FPI3_O | `0x0811` |
| N/A | F32KON_O | `0x0818` |

### DSP Block1 (`block_id=0x00`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| DSP Interrupt 1 | ciph_int | `0x0024` |
| DSP Interrupt 1 | dspin0_int | `0x0028` |
| DSP Interrupt 1 | dspin1_int | `0x0029` |
| DSP Interrupt 1 | dtmr10_int | `0x0025` |
| DSP Interrupt 1 | dtmr11_int | `0x0026` |
| DSP Interrupt 1 | dtmr2_int | `0x0027` |
| DSP Interrupt 1 | monin1_int | `0x002A` |
| DSP Interrupt 1 | monin2_int | `0x002B` |
| DSP Interrupt 1 | monin3_int | `0x002C` |
| DSP Interrupt 1 | monin4_int | `0x002D` |
| DSP Interrupt 2 | intfw0 | `0x002E` |
| DSP Interrupt 2 | intfw1 | `0x002F` |
| DSP Interrupt 2 | intfw2 | `0x0030` |
| DSP Interrupt 2 | intfw3 | `0x0031` |
| DSP Interrupt 2 | intfw4 | `0x0032` |
| DSP Interrupt 2 | intfw5 | `0x0033` |
| DSP Interrupt 2 | intfw6 | `0x0034` |
| DSP Interrupt 2 | intfw7 | `0x0035` |
| DSP Interrupt 2 | intfw8 | `0x0036` |
| DSP Interrupt 2 | intfw9 | `0x0037` |
| DSP Interrupt 2 | intfw10 | `0x0038` |
| DSP Interrupt 2 | intfw11 | `0x0039` |
| DSP Interrupt 2 | intfw12 | `0x003A` |
| DSP Interrupt 2 | intfw13 | `0x003B` |
| DSP Interrupt 2 | intfw14 | `0x003C` |
| DSP Interrupt 2 | intfw15 | `0x003D` |
| DSP Interrupt A0 | bb_full_int | `0x0013` |
| DSP Interrupt A0 | chadec_int | `0x000D` |
| DSP Interrupt A0 | codon_int | `0x000B` |
| DSP Interrupt A0 | eq_int | `0x000E` |
| DSP Interrupt A0 | eqon_int | `0x000F` |
| DSP Interrupt A0 | fcon_int | `0x0010` |
| DSP Interrupt A0 | frame_int | `0x0005` |
| DSP Interrupt A0 | mcu0_int | `0x0001` |
| DSP Interrupt A0 | mcu1_int | `0x0002` |
| DSP Interrupt A0 | mcu2_int | `0x0003` |
| DSP Interrupt A0 | mcu3_int | `0x0004` |
| DSP Interrupt A0 | modu_int | `0x000C` |
| DSP Interrupt A0 | monon_int | `0x0011` |
| DSP Interrupt A0 | scon_int | `0x0012` |
| DSP Interrupt B0 | i2s1rx_int | `0x0014` |
| DSP Interrupt B0 | i2s1tx_int | `0x0015` |
| DSP Interrupt B0 | i2s2rx_int | `0x0016` |
| DSP Interrupt B0 | i2s2tx_int | `0x0017` |
| DSP Interrupt B0 | i2s3rx_int | `0x0018` |
| DSP Interrupt B0 | i2s3tx_int | `0x0019` |
| DSP Interrupt B0 | ssc2_err_int | `0x0022` |
| DSP Interrupt B0 | ssc2_rx_int | `0x0020` |
| DSP Interrupt B0 | ssc2_tx_int | `0x0021` |
| DSP Interrupt B0 | sysmcu_int | `0x0023` |
| DSP Interrupt B0 | vbrx_int | `0x001B` |
| DSP Interrupt B0 | vbtx_int | `0x001A` |
| DSP | mon_dspdis | `0x0045` |
| DSP | mon_dxap_page(0) | `0x0043` |
| DSP | mon_dxap_page(1) | `0x0044` |
| DSP | mon_en_reg_read | `0x0046` |
| DSP | mon_ppap_page(0) | `0x0041` |
| DSP | mon_ppap_page(1) | `0x0042` |
| DSP | mon_rst_dsp_mem_n | `0x005E` |
| DSP | mon_rst_dsp_n | `0x005C` |
| DSP | mon_rst_dsp_per_n | `0x005D` |
| DSP | mon_rst_pll_n | `0x005B` |
| DSP BB-Transmit | mon_clk_bbtx | `0x007A` |
| DSP BB-Transmit | mon_dpram_data_o(0) | `0x0072` |
| DSP BB-Transmit | mon_dpram_data_o(1) | `0x0073` |
| DSP BB-Transmit | mon_dpram_data_o(2) | `0x0074` |
| DSP BB-Transmit | mon_dpram_data_o(3) | `0x0075` |
| DSP BB-Transmit | mon_dpram_req | `0x0077` |
| DSP Modulat.to DAC | mon_daci_data_o(0) | `0x0060` |
| DSP Modulat.to DAC | mon_daci_data_o(1) | `0x0061` |
| DSP Modulat.to DAC | mon_daci_data_o(2) | `0x0062` |
| DSP Modulat.to DAC | mon_daci_data_o(3) | `0x0063` |
| DSP Modulat.to DAC | mon_daci_data_o(4) | `0x0064` |
| DSP Modulat.to DAC | mon_daci_data_o(5) | `0x0065` |
| DSP Modulat.to DAC | mon_daci_data_o(6) | `0x0066` |
| DSP Modulat.to DAC | mon_daci_data_o(7) | `0x0067` |
| DSP Modulat.to DAC | mon_daci_data_o(8) | `0x0068` |
| DSP Modulat.to DAC | mon_dacq_data_o(0) | `0x0069` |
| DSP Modulat.to DAC | mon_dacq_data_o(1) | `0x006A` |
| DSP Modulat.to DAC | mon_dacq_data_o(2) | `0x006B` |
| DSP Modulat.to DAC | mon_dacq_data_o(3) | `0x006C` |
| DSP Modulat.to DAC | mon_dacq_data_o(4) | `0x006D` |
| DSP Modulat.to DAC | mon_dacq_data_o(5) | `0x006E` |
| DSP Modulat.to DAC | mon_dacq_data_o(6) | `0x006F` |
| DSP Modulat.to DAC | mon_dacq_data_o(7) | `0x0070` |
| DSP Modulat.to DAC | mon_dacq_data_o(8) | `0x0071` |
| DSP Ocem | abortn | `0x0059` |
| DSP Ocem | btrapreq | `0x005A` |
| DSP Ocem | debugp | `0x0057` |
| DSP Ocem | dtvmp | `0x0056` |
| DSP Ocem | obootp | `0x0058` |
| DSP Ocem | pdummyp | `0x0055` |
| DSP Ocem | pprp | `0x0052` |
| DSP Ocem | ppwp | `0x0051` |
| DSP Ocem | psftp | `0x0050` |
| DSP Ocem | pstatusp(0) | `0x004C` |
| DSP Ocem | pstatusp(1) | `0x004D` |
| DSP Ocem | pstatusp(2) | `0x004E` |
| DSP Ocem | pstatusp(3) | `0x004F` |
| DSP Seib | enocemn | `0x004B` |
| DSP Seib | seib_j_abort_out_n | `0x0049` |
| DSP Seib | seib_j_tdo_p | `0x0048` |
| DSP Seib | waitp | `0x004A` |
| DSP Miscellaneous | int0 | `0x001C` |
| DSP Miscellaneous | int1 | `0x001D` |
| DSP Miscellaneous | int2 | `0x001E` |
| DSP Miscellaneous | inttomcu0 | `0x0006` |
| DSP Miscellaneous | inttomcu1 | `0x0007` |
| DSP Miscellaneous | inttomcu2 | `0x0008` |
| DSP Miscellaneous | inttomcu3 | `0x0009` |
| DSP Miscellaneous | mon_dspout0 | `0x007C` |
| DSP Miscellaneous | mon_dspout1 | `0x007D` |
| DSP Miscellaneous | mon_dspout2 | `0x007E` |
| DSP Miscellaneous | mon_gclk_mix_gmsk | `0x0079` |
| DSP Miscellaneous | mon_gclk_mix_psk | `0x0079` |
| DSP Miscellaneous | mon_gclk_pll_mod | `0x0078` |
| DSP Miscellaneous | mon_gmsk_sel_n | `0x007B` |

### DSP Block2 (`block_id=0x02`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Audio-FE | mon_clk_vbrx | `0x020D` |
| Audio-FE | mon_clk_vbtx | `0x0210` |
| Audio-FE | mon_clk_vbtx_dith | `0x020F` |
| Audio-FE | mon_data_rrq_rx | `0x0203` |
| Audio-FE | mon_data_vbrx_left | `0x0208` |
| Audio-FE | mon_data_vbrx_lsb4(0) | `0x0209` |
| Audio-FE | mon_data_vbrx_lsb4(1) | `0x020A` |
| Audio-FE | mon_data_vbrx_lsb4(2) | `0x020B` |
| Audio-FE | mon_data_vbrx_lsb4(3) | `0x020C` |
| Audio-FE | mon_data_vbtx | `0x020E` |
| Audio-FE | mon_data_wrq_tx | `0x0204` |
| Audio-FE | mon_powerup_int_asmd_rx | `0x0206` |
| Audio-FE | mon_powerup_int_asmd_tx | `0x0205` |
| Audio-FE | mon_rxclear | `0x0201` |
| Audio-FE | mon_txclear | `0x0202` |
| Audio-FE | mon_zero_detect | `0x0207` |
| BB-Receive | mon_clk_bbrx | `0x0218` |
| BB-Receive | mon_clk_dither | `0x0212` |
| BB-Receive | mon_gclock_mix | `0x0213` |
| BB-Receive | mon_gclock_pll | `0x0211` |
| BB-Receive | mon_pdm1i | `0x0216` |
| BB-Receive | mon_pdm1q | `0x0214` |
| BB-Receive | mon_pdm2i | `0x0217` |
| BB-Receive | mon_pdm2q | `0x0215` |
| Cipher A53 | mon_a53_ciphering_ready | `0x0266` |
| Cipher A53 | mon_a53_dcry_valid | `0x0269` |
| Cipher A53 | mon_a53_ecry_dcry_data | `0x0268` |
| Cipher A53 | mon_a53_ecry_valid | `0x0263` |
| Cipher A53 | mon_a53_gclock_cipher | `0x0265` |
| Cipher A53 | mon_a53_gclock_dsp | `0x0262` |
| Cipher A53 | mon_a53_gclock_ram | `0x0264` |
| Cipher A53 | mon_a53_start_ciphering | `0x0267` |
| Cipher A53 | mon_block_count(0) | `0x026F` |
| Cipher A53 | mon_block_count(1) | `0x0270` |
| Cipher A53 | mon_block_count(2) | `0x0271` |
| Cipher A53 | mon_block_count(3) | `0x0272` |
| Cipher A53 | mon_clear_init | `0x0273` |
| Cipher A53 | mon_init_kgcore | `0x026A` |
| Cipher A53 | mon_state_kgcore(0) | `0x026B` |
| Cipher A53 | mon_state_kgcore(1) | `0x026C` |
| Cipher A53 | mon_state_kgcore(2) | `0x026D` |
| Cipher A53 | mon_state_kgcore(3) | `0x026E` |
| GSM-Cipher | mon_ciphering_ready | `0x0219` |
| GSM-Cipher | mon_dcry_valid | `0x021C` |
| GSM-Cipher | mon_ecry_dcry_data | `0x021B` |
| GSM-Cipher | mon_ecry_valid | `0x021D` |
| GSM-Cipher | mon_gclock_cipher | `0x0220` |
| GSM-Cipher | mon_gclock_dsp | `0x021E` |
| GSM-Cipher | mon_gclock_ram | `0x021F` |
| GSM-Cipher | mon_start_ciphering | `0x021A` |
| GSM-Cipher | mon_state_ctrl(0) | `0x0253` |
| GSM-Cipher | mon_state_ctrl(1) | `0x0254` |
| GSM-Cipher | mon_state_ctrl(2) | `0x0255` |
| I2S 1 | mon_cl1_state(0) | `0x022C` |
| I2S 1 | mon_cl1_state(1) | `0x022D` |
| I2S 1 | mon_cl1_state(2) | `0x022E` |
| I2S 1 | mon_cl2_state(0) | `0x0229` |
| I2S 1 | mon_cl2_state(1) | `0x022A` |
| I2S 1 | mon_cl2_state(2) | `0x022B` |
| I2S 1 | mon_i2s_data_rrq | `0x0230` |
| I2S 1 | mon_i2s_data_wrq | `0x022F` |
| I2S 1 | mon_rx_state(0) | `0x0221` |
| I2S 1 | mon_rx_state(1) | `0x0222` |
| I2S 1 | mon_rx_state(2) | `0x0223` |
| I2S 1 | mon_rx_state(3) | `0x0224` |
| I2S 1 | mon_tx_state(0) | `0x0225` |
| I2S 1 | mon_tx_state(1) | `0x0226` |
| I2S 1 | mon_tx_state(2) | `0x0227` |
| I2S 1 | mon_tx_state(3) | `0x0228` |
| I2S 2 | mon_cl1_state(0) | `0x023C` |
| I2S 2 | mon_cl1_state(1) | `0x023D` |
| I2S 2 | mon_cl1_state(2) | `0x023E` |
| I2S 2 | mon_cl2_state(0) | `0x0239` |
| I2S 2 | mon_cl2_state(1) | `0x023A` |
| I2S 2 | mon_cl2_state(2) | `0x023B` |
| I2S 2 | mon_i2s_data_rrq | `0x0240` |
| I2S 2 | mon_i2s_data_wrq | `0x023F` |
| I2S 2 | mon_rx_state(0) | `0x0231` |
| I2S 2 | mon_rx_state(1) | `0x0232` |
| I2S 2 | mon_rx_state(2) | `0x0233` |
| I2S 2 | mon_rx_state(3) | `0x0234` |
| I2S 2 | mon_tx_state(0) | `0x0235` |
| I2S 2 | mon_tx_state(1) | `0x0236` |
| I2S 2 | mon_tx_state(2) | `0x0237` |
| I2S 2 | mon_tx_state(3) | `0x0238` |
| I2S 3 | mon_cl1_state(0) | `0x024C` |
| I2S 3 | mon_cl1_state(1) | `0x024D` |
| I2S 3 | mon_cl1_state(2) | `0x024E` |
| I2S 3 | mon_cl2_state(0) | `0x0249` |
| I2S 3 | mon_cl2_state(1) | `0x024A` |
| I2S 3 | mon_cl2_state(2) | `0x024B` |
| I2S 3 | mon_i2s_data_rrq | `0x0250` |
| I2S 3 | mon_i2s_data_wrq | `0x024F` |
| I2S 3 | mon_rx_state(0) | `0x0241` |
| I2S 3 | mon_rx_state(1) | `0x0242` |
| I2S 3 | mon_rx_state(2) | `0x0243` |
| I2S 3 | mon_rx_state(3) | `0x0244` |
| I2S 3 | mon_tx_state(0) | `0x0245` |
| I2S 3 | mon_tx_state(1) | `0x0246` |
| I2S 3 | mon_tx_state(2) | `0x0247` |
| I2S 3 | mon_tx_state(3) | `0x0248` |
| Viterbi-Decoder | mon_dec_active | `0x0258` |
| Viterbi-Decoder | mon_dec_busy | `0x0257` |
| Viterbi-Decoder | mon_pointer_dec_new | `0x0259` |
| Viterbi-Decoder | mon_res_dec | `0x025A` |
| Viterbi-Decoder | mon_res_dsp_int | `0x025B` |
| Viterbi-Decoder | mon_vh_status | `0x0256` |
| Viterbi-Equalizer | mon_eq_busy | `0x025D` |
| Viterbi-Equalizer | mon_pointer_left_hs_new | `0x025F` |
| Viterbi-Equalizer | mon_pointer_right_hs_new | `0x025E` |
| Viterbi-Equalizer | mon_res_dsp_int | `0x0261` |
| Viterbi-Equalizer | mon_res_eq | `0x0260` |
| Viterbi-Equalizer | mon_vh_status | `0x025C` |

### FPI1 (`block_id=0x04`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| FPI | cgu_clk_fpi1_rg | `0x0453` |
| FPI | fpi_ack_s[0] | `0x0458` |
| FPI | fpi_ack_s[1] | `0x0459` |
| FPI | fpi_endinit_s | `0x045A` |
| FPI | fpi_opc_s[0] | `0x0454` |
| FPI | fpi_opc_s[1] | `0x0455` |
| FPI | fpi_opc_s[2] | `0x0456` |
| FPI | fpi_opc_s[3] | `0x0457` |
| FPI | fpi_rd_n_s | `0x0450` |
| FPI | fpi_rdy_s | `0x0452` |
| FPI | fpi_sleep_n_s | `0x045B` |
| FPI | FPI_SSC_SEL_N | `0x045E` |
| FPI | FPI_USART0_SEL_N | `0x0460` |
| FPI | FPI_USART1_SEL_N | `0x045D` |
| FPI | FPI_USB_SEL_N | `0x0462` |
| FPI | FPI_USIM_SEL_N | `0x0461` |
| FPI | fpi_wr_n_s | `0x0451` |
| FPI | ocds_p_suspend_s | `0x045C` |
| SSC0 | SSC0_E_IRQ | `0x040B` |
| SSC0 | SSC0_R_DMACLR | `0x041F` |
| SSC0 | SSC0_R_DMAREQ | `0x041D` |
| SSC0 | SSC0_R_IRQ | `0x040A` |
| SSC0 | SSC0_T_DMACLR | `0x041E` |
| SSC0 | SSC0_T_DMAREQ | `0x041C` |
| SSC0 | SSC0_T_IRQ | `0x0409` |
| SSC0 | SSC0_TMO_IRQ | `0x040C` |
| USART0 | USART0_ABDET_IRQ | `0x042F` |
| USART0 | USART0_ABST_IRQ | `0x042E` |
| USART0 | USART0_B_IRQ | `0x042B` |
| USART0 | USART0_CTS_IRQ | `0x0430` |
| USART0 | USART0_E_IRQ | `0x042D` |
| USART0 | USART0_R_DMACLR | `0x0427` |
| USART0 | USART0_R_DMAREQ | `0x0425` |
| USART0 | USART0_R_IRQ | `0x042C` |
| USART0 | USART0_T_DMACLR | `0x0426` |
| USART0 | USART0_T_DMAREQ | `0x0424` |
| USART0 | USART0_T_IRQ | `0x042A` |
| USART0 | USART0_TMO_IRQ | `0x0431` |
| USART1 | USART1_ABDET_IRQ | `0x0406` |
| USART1 | USART1_B_IRQ | `0x0402` |
| USART1 | USART1_CTS_IRQ | `0x0407` |
| USART1 | USART1_R_DMACLR | `0x041B` |
| USART1 | USART1_R_DMAREQ | `0x0419` |
| USART1 | USART1_R_IRQ | `0x0403` |
| USART1 | USART1_T_DMACLR | `0x041A` |
| USART1 | USART1_T_DMAREQ | `0x0418` |
| USART1 | USART1_T_IRQ | `0x0401` |
| USART1 | USART1_TMO_IRQ | `0x0408` |
| USART1 | USART1ABST_IRQ | `0x0405` |
| USART1 | USART1E_IRQ | `0x0404` |
| USB | usb_clk_rg | `0x044D` |
| USB | USB_DIF | `0x0464` |
| USB | USB_DMACLR[0] | `0x0441` |
| USB | USB_DMACLR[1] | `0x0442` |
| USB | USB_DMACLR[2] | `0x0443` |
| USB | USB_DMACLR[3] | `0x0444` |
| USB | USB_DMAREQ[0] | `0x0432` |
| USB | USB_DMAREQ[1] | `0x0433` |
| USB | USB_DMAREQ[2] | `0x0434` |
| USB | USB_DMAREQ[3] | `0x0435` |
| USB | USB_INT | `0x0416` |
| USIM | USIM_E_IRQ | `0x0413` |
| USIM | USIM_IN_IRQ | `0x0414` |
| USIM | USIM_OK_IRQ | `0x0415` |
| USIM | USIM_TXRX_DMACLR | `0x0429` |
| USIM | USIM_TXRX_DMAREQ | `0x0428` |

### FPI21 (`block_id=0x0A`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| CapCom 0 | CC0T0SRC | `0x0A28` |
| CapCom 0 | CC0T1SRC | `0x0A29` |
| CapCom 0 | CC0SRC0 | `0x0A2A` |
| CapCom 0 | CC0SRC1 | `0x0A2B` |
| CapCom 0 | CC0SRC2 | `0x0A2C` |
| CapCom 0 | CC0SRC3 | `0x0A2D` |
| CapCom 0 | CC0SRC4 | `0x0A2E` |
| CapCom 0 | CC0SRC5 | `0x0A2F` |
| CapCom 0 | CC0SRC6 | `0x0A30` |
| CapCom 0 | CC0SRC7 | `0x0A31` |
| CapCom 1 | CC1T0SRC | `0x0A32` |
| CapCom 1 | CC1T1SRC | `0x0A33` |
| CapCom 1 | CC1SRC0 | `0x0A34` |
| CapCom 1 | CC1SRC1 | `0x0A35` |
| CapCom 1 | CC1SRC2 | `0x0A36` |
| CapCom 1 | CC1SRC3 | `0x0A37` |
| CapCom 1 | CC1SRC4 | `0x0A38` |
| CapCom 1 | CC1SRC5 | `0x0A39` |
| CapCom 1 | CC1SRC6 | `0x0A3A` |
| CapCom 1 | CC1SRC7 | `0x0A3B` |
| CGU | CGU_LC | `0x0A77` |
| CGU | CGU_PH1PU | `0x0A72` |
| CGU | CGU_PH2PU | `0x0A73` |
| CGU | CGU_PH3PU | `0x0A74` |
| CGU | CGU_PH4PU | `0x0A75` |
| CGU | CGU_PLLPU | `0x0A71` |
| CGU | CGU_PLL_LOCKED_I | `0x0A70` |
| CGU | CSC_CLK_RG | `0x0A76` |
| FPI | FPI_A0 | `0x0A12` |
| FPI | FPI_A31 | `0x0A14` |
| FPI | FPI_A7 | `0x0A13` |
| FPI | FPI_ACK0 | `0x0A0D` |
| FPI | FPI_ACK1 | `0x0A0E` |
| FPI | FPI_D0 | `0x0A15` |
| FPI | FPI_D7 | `0x0A16` |
| FPI | FPI_ENDINIT | `0x0A0B` |
| FPI | FPI_OCDS_SUSPEND | `0x0A0C` |
| FPI | FPI_OPC0 | `0x0A06` |
| FPI | FPI_OPC1 | `0x0A07` |
| FPI | FPI_OPC2 | `0x0A08` |
| FPI | FPI_OPC3 | `0x0A09` |
| FPI | FPI_RD | `0x0A03` |
| FPI | FPI_RDY | `0x0A0F` |
| FPI | FPI_SLEEP_n | `0x0A0A` |
| FPI | FPI_SVM | `0x0A11` |
| FPI | FPI_TOUT | `0x0A10` |
| FPI | FPI_WR | `0x0A04` |
| GPTU 0 | GPTU0_DMAREQ | `0x0A01` |
| GPTU 0 | GT0SRC0 | `0x0A46` |
| GPTU 0 | GT0SRC1 | `0x0A47` |
| GPTU 0 | GT0SRC2 | `0x0A48` |
| GPTU 0 | GT0SRC3 | `0x0A49` |
| GPTU 0 | GT0SRC4 | `0x0A4A` |
| GPTU 0 | GT0SRC5 | `0x0A4B` |
| GPTU 0 | GT0SRC6 | `0x0A4C` |
| GPTU 0 | GT0SRC7 | `0x0A4D` |
| GPTU 1 | GPTU1_DMAREQ | `0x0A02` |
| GPTU 1 | GT1SRC0 | `0x0A4E` |
| GPTU 1 | GT1SRC1 | `0x0A4F` |
| GPTU 1 | GT1SRC2 | `0x0A50` |
| GPTU 1 | GT1SRC3 | `0x0A51` |
| GPTU 1 | GT1SRC4 | `0x0A52` |
| GPTU 1 | GT1SRC5 | `0x0A53` |
| GPTU 1 | GT1SRC6 | `0x0A54` |
| GPTU 1 | GT1SRC7 | `0x0A55` |
| Keypad | KEYPAD_FSM_STATE0 | `0x0A58` |
| Keypad | KEYPAD_FSM_STATE1 | `0x0A59` |
| Keypad | KEYPAD_INT0 | `0x0A5C` |
| Keypad | KEYPAD_INT1 | `0x0A5D` |
| Keypad | KEYPAD_INT2 | `0x0A5E` |
| Keypad | KEYPAD_INT3 | `0x0A5F` |
| Keypad | KEYPAD_KEY_PRESSED | `0x0A5A` |
| Meas./Ana. | ANA_EOC | `0x0A22` |
| Meas./Ana. | GSM_ADCTRIG | `0x0A24` |
| Meas./Ana. | MEAS_CLK | `0x0A20` |
| Meas./Ana. | MEAS_SOC | `0x0A21` |
| RTC | RTC0IR | `0x0A62` |
| RTC | RTC1IR | `0x0A63` |
| RTC | RTC2IR | `0x0A64` |
| RTC | RTC3IR | `0x0A65` |
| RTC | RTC_CLK | `0x0A68` |
| RTC | RTC_INT | `0x0A60` |
| RTC | RTC_REF_CLK | `0x0A69` |
| RTC | RTC_T14INT | `0x0A61` |
| RTC | RTCALARM | `0x0A66` |
| RTC | RTCBAD | `0x0A67` |

### FPI22 (`block_id=0x0B`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| GPIO 00..31 | gpio_pad_00 | `0x0B4D` |
| GPIO 00..31 | gpio_pad_01 | `0x0B01` |
| GPIO 00..31 | gpio_pad_02 | `0x0B02` |
| GPIO 00..31 | gpio_pad_03 | `0x0B03` |
| GPIO 00..31 | gpio_pad_04 | `0x0B04` |
| GPIO 00..31 | gpio_pad_05 | `0x0B05` |
| GPIO 00..31 | gpio_pad_06 | `0x0B06` |
| GPIO 00..31 | gpio_pad_07 | `0x0B07` |
| GPIO 00..31 | gpio_pad_08 | `0x0B08` |
| GPIO 00..31 | gpio_pad_09 | `0x0B09` |
| GPIO 00..31 | gpio_pad_10 | `0x0B0A` |
| GPIO 00..31 | gpio_pad_11 | `0x0B0B` |
| GPIO 00..31 | gpio_pad_12 | `0x0B0C` |
| GPIO 00..31 | gpio_pad_13 | `0x0B0D` |
| GPIO 00..31 | gpio_pad_14 | `0x0B0E` |
| GPIO 00..31 | gpio_pad_15 | `0x0B0F` |
| GPIO 00..31 | gpio_pad_16 | `0x0B10` |
| GPIO 00..31 | gpio_pad_17 | `0x0B11` |
| GPIO 00..31 | gpio_pad_18 | `0x0B12` |
| GPIO 00..31 | gpio_pad_19 | `0x0B13` |
| GPIO 00..31 | gpio_pad_20 | `0x0B14` |
| GPIO 00..31 | gpio_pad_21 | `0x0B15` |
| GPIO 00..31 | gpio_pad_22 | `0x0B16` |
| GPIO 00..31 | gpio_pad_23 | `0x0B17` |
| GPIO 00..31 | gpio_pad_24 | `0x0B18` |
| GPIO 00..31 | gpio_pad_25 | `0x0B19` |
| GPIO 00..31 | gpio_pad_26 | `0x0B1A` |
| GPIO 00..31 | gpio_pad_27 | `0x0B1B` |
| GPIO 00..31 | gpio_pad_28 | `0x0B1C` |
| GPIO 00..31 | gpio_pad_29 | `0x0B1D` |
| GPIO 00..31 | gpio_pad_30 | `0x0B1E` |
| GPIO 00..31 | gpio_pad_31 | `0x0B1F` |
| GPIO 32..63 | gpio_pad_32 | `0x0B20` |
| GPIO 32..63 | gpio_pad_33 | `0x0B21` |
| GPIO 32..63 | gpio_pad_34 | `0x0B22` |
| GPIO 32..63 | gpio_pad_35 | `0x0B23` |
| GPIO 32..63 | gpio_pad_36 | `0x0B24` |
| GPIO 32..63 | gpio_pad_37 | `0x0B25` |
| GPIO 32..63 | gpio_pad_38 | `0x0B26` |
| GPIO 32..63 | gpio_pad_39 | `0x0B27` |
| GPIO 32..63 | gpio_pad_40 | `0x0B28` |
| GPIO 32..63 | gpio_pad_41 | `0x0B29` |
| GPIO 32..63 | gpio_pad_42 | `0x0B2A` |
| GPIO 32..63 | gpio_pad_43 | `0x0B2B` |
| GPIO 32..63 | gpio_pad_44 | `0x0B2C` |
| GPIO 32..63 | gpio_pad_45 | `0x0B2D` |
| GPIO 32..63 | gpio_pad_46 | `0x0B2E` |
| GPIO 32..63 | gpio_pad_47 | `0x0B2F` |
| GPIO 32..63 | gpio_pad_48 | `0x0B30` |
| GPIO 32..63 | gpio_pad_49 | `0x0B31` |
| GPIO 32..63 | gpio_pad_50 | `0x0B32` |
| GPIO 32..63 | gpio_pad_51 | `0x0B33` |
| GPIO 32..63 | gpio_pad_52 | `0x0B34` |
| GPIO 32..63 | gpio_pad_53 | `0x0B35` |
| GPIO 32..63 | gpio_pad_54 | `0x0B36` |
| GPIO 32..63 | gpio_pad_55 | `0x0B37` |
| GPIO 32..63 | gpio_pad_56 | `0x0B38` |
| GPIO 32..63 | gpio_pad_57 | `0x0B39` |
| GPIO 32..63 | gpio_pad_58 | `0x0B3A` |
| GPIO 32..63 | gpio_pad_59 | `0x0B3B` |
| GPIO 32..63 | gpio_pad_60 | `0x0B3C` |
| GPIO 32..63 | gpio_pad_61 | `0x0B3D` |
| GPIO 32..63 | gpio_pad_62 | `0x0B3E` |
| GPIO 32..63 | gpio_pad_63 | `0x0B3F` |
| GPIO 64..95 | gpio_pad_64 | `0x0B40` |
| GPIO 64..95 | gpio_pad_65 | `0x0B41` |
| GPIO 64..95 | gpio_pad_66 | `0x0B42` |
| GPIO 64..95 | gpio_pad_67 | `0x0B43` |
| GPIO 64..95 | gpio_pad_68 | `0x0B44` |
| GPIO 64..95 | gpio_pad_69 | `0x0B45` |
| GPIO 64..95 | gpio_pad_70 | `0x0B46` |
| GPIO 64..95 | gpio_pad_71 | `0x0B47` |
| GPIO 64..95 | gpio_pad_72 | `0x0B48` |
| GPIO 64..95 | gpio_pad_73 | `0x0B49` |
| GPIO 64..95 | gpio_pad_74 | `0x0B4A` |
| GPIO 64..95 | gpio_pad_75 | `0x0B4B` |
| GPIO 64..95 | gpio_pad_76 | `0x0B4C` |
| GPIO 64..95 | gpio_pad_77 | `0x0B6D` |
| GPIO 64..95 | gpio_pad_78 | `0x0B4E` |
| GPIO 64..95 | gpio_pad_79 | `0x0B4F` |
| GPIO 64..95 | gpio_pad_80 | `0x0B50` |
| GPIO 64..95 | gpio_pad_81 | `0x0B51` |
| GPIO 64..95 | gpio_pad_82 | `0x0B52` |
| GPIO 64..95 | gpio_pad_83 | `0x0B53` |
| GPIO 64..95 | gpio_pad_84 | `0x0B54` |
| GPIO 64..95 | gpio_pad_85 | `0x0B55` |
| GPIO 64..95 | gpio_pad_86 | `0x0B56` |
| GPIO 64..95 | gpio_pad_87 | `0x0B57` |
| GPIO 64..95 | gpio_pad_88 | `0x0B58` |
| GPIO 64..95 | gpio_pad_89 | `0x0B59` |
| GPIO 64..95 | gpio_pad_90 | `0x0B5A` |
| GPIO 64..95 | gpio_pad_91 | `0x0B5B` |
| GPIO 64..95 | gpio_pad_92 | `0x0B5C` |
| GPIO 64..95 | gpio_pad_93 | `0x0B5D` |
| GPIO 64..95 | gpio_pad_94 | `0x0B5E` |
| GPIO 64..95 | gpio_pad_95 | `0x0B5F` |
| GPIO 96..113 | gpio_pad_96 | `0x0B60` |
| GPIO 96..113 | gpio_pad_97 | `0x0B61` |
| GPIO 96..113 | gpio_pad_98 | `0x0B62` |
| GPIO 96..113 | gpio_pad_99 | `0x0B63` |
| GPIO 96..113 | gpio_pad_100 | `0x0B6E` |
| GPIO 96..113 | gpio_pad_101 | `0x0B6F` |
| GPIO 96..113 | gpio_pad_102 | `0x0B70` |
| GPIO 96..113 | gpio_pad_103 | `0x0B71` |
| GPIO 96..113 | gpio_pad_104 | `0x0B72` |
| GPIO 96..113 | gpio_pad_105 | `0x0B73` |
| GPIO 96..113 | gpio_pad_106 | `0x0B74` |
| GPIO 96..113 | gpio_pad_107 | `0x0B75` |
| GPIO 96..113 | gpio_pad_108 | `0x0B76` |
| GPIO 96..113 | gpio_pad_109 | `0x0B77` |
| GPIO 96..113 | gpio_pad_110 | `0x0B78` |
| GPIO 96..113 | gpio_pad_111 | `0x0B79` |
| GPIO 96..113 | gpio_pad_112 | `0x0B7A` |
| GPIO 96..113 | gpio_pad_113 | `0x0B7B` |
| Pad | REST_PAD0 | `0x0B64` |
| Pad | REST_PAD1 | `0x0B65` |
| Pad | REST_PAD2 | `0x0B66` |
| Pad | REST_PAD3 | `0x0B67` |
| Pad | REST_PAD4 | `0x0B68` |
| Pad | REST_PAD5 | `0x0B69` |
| Pad | REST_PAD6 | `0x0B6A` |
| Pad | REST_PAD7 | `0x0B6B` |

### FPI3 (`block_id=0x07`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| FPI | AREADY_TGL | `0x0779` |
| FPI | ARESP(0) | `0x077A` |
| FPI | ARESP(1) | `0x077B` |
| FPI | FPI3_A(0) | `0x070F` |
| FPI | FPI3_DATA_RD(0) | `0x0714` |
| FPI | FPI3_DATA_RD(7) | `0x0715` |
| FPI | FPI3_DATA_WR(0) | `0x0712` |
| FPI | FPI3_DATA_WR(7) | `0x0713` |
| FPI | FPI3_OPC(0) | `0x070B` |
| FPI | FPI3_OPC(1) | `0x070C` |
| FPI | FPI3_OPC(2) | `0x070D` |
| FPI | FPI3_OPC(3) | `0x070E` |
| FPI | FPI3_RD_n | `0x0706` |
| FPI | FPI3_RDY | `0x0708` |
| FPI | FPI3_WR_n | `0x0707` |
| FPI | FPI_A(7) | `0x0710` |
| FPI | FPI_A(31) | `0x0711` |
| FPI | FPI_ACK(0) | `0x0709` |
| FPI | FPI_ACK(1) | `0x070A` |
| FPI | FPI_ENDINIT | `0x0702` |
| FPI | FPI_SLEEP_n | `0x0701` |
| FPI | FPI_SVM | `0x0705` |
| FPI | FPI_TOUT | `0x0703` |
| FPI | OCDS_P_SUSPEND | `0x0704` |
| GPRS | GPRS_DMACLR(0) | `0x077C` |
| GPRS | GPRS_DMACLR(1) | `0x077D` |
| GPRS | GPRS_DMAREQ(0) | `0x0750` |
| GPRS | GPRS_DMAREQ(1) | `0x0751` |
| GPRS | GPRS_IRQ(0) | `0x074E` |
| GPRS | GPRS_IRQ(1) | `0x074F` |
| GPRS | gprs_kernel_irq (1E) | `0x071E` |
| GPRS | gprs_kernel_irq (1F) | `0x071F` |
| GSM | GSM_DSP_CODON | `0x0730` |
| GSM | GSM_DSP_FCON | `0x072D` |
| GSM | GSM_DSP_IQRAMP | `0x0732` |
| GSM | GSM_DSP_RXON | `0x072F` |
| GSM | GSM_DSP_SCON | `0x072E` |
| GSM | GSM_EQON | `0x072B` |
| GSM | GSM_INT_GP0 | `0x0721` |
| GSM | GSM_INT_GP1 | `0x0722` |
| GSM | GSM_INT_GP2 | `0x0723` |
| GSM | GSM_INT_GP3 | `0x0724` |
| GSM | GSM_INT_GP4 | `0x0725` |
| GSM | GSM_INT_GP5 | `0x0726` |
| GSM | GSM_INT_GP6 | `0x0727` |
| GSM | GSM_MEAS_ADCTRIG | `0x0734` |
| GSM | GSM_MONON | `0x072C` |
| GSM | GSM_RF_CLK | `0x0777` |
| GSM | GSM_RF_DATA | `0x0778` |
| GSM | GSM_RFSSCTINT | `0x0720` |
| GSM | GSM_SPCU_SLPSTART | `0x0731` |
| GSM | GSM_T_INT1 | `0x0728` |
| GSM | GSM_T_INT2 | `0x0729` |
| GSM | GSM_TXON | `0x072A` |
| GSM | MODESW | `0x0776` |
| GSM | PASW | `0x0733` |
| PA Ramping | DCPA_DATA0 | `0x0736` |
| PA Ramping | DCPA_DATA1 | `0x0737` |
| PA Ramping | DCPA_DATA2 | `0x0738` |
| PA Ramping | DCPA_DATA3 | `0x0739` |
| PA Ramping | DCPA_DATA4 | `0x073A` |
| PA Ramping | DCPA_DATA5 | `0x073B` |
| PA Ramping | DCPA_DATA6 | `0x073C` |
| PA Ramping | DCPA_DATA7 | `0x073D` |
| PA Ramping | DCPA_DATA8 | `0x073E` |
| PA Ramping | DCPA_DATA9 | `0x073F` |
| PA Ramping | DCPA_DATA10 | `0x0740` |
| PA Ramping | DCPA_DATAVALID | `0x0741` |
| PA Ramping | PAR_CLR_INT_FSM | `0x0735` |
| PA Ramping | PAR_DAC0 | `0x0743` |
| PA Ramping | PAR_DAC1 | `0x0744` |
| PA Ramping | PAR_DAC2 | `0x0745` |
| PA Ramping | PAR_DAC3 | `0x0746` |
| PA Ramping | PAR_DAC4 | `0x0747` |
| PA Ramping | PAR_DAC5 | `0x0748` |
| PA Ramping | PAR_DAC6 | `0x0749` |
| PA Ramping | PAR_DAC7 | `0x074A` |
| PA Ramping | PAR_DAC8 | `0x074B` |
| PA Ramping | PAR_DAC9 | `0x074C` |
| PA Ramping | PAR_DAC10 | `0x074D` |
| PA Ramping | PAR_DAC_CLK | `0x0742` |
| SHM | CFS2 | `0x0761` |
| SHM | CSBA | `0x0754` |
| SHM | CSBB | `0x0755` |
| SHM | DSPCOMR2 | `0x0760` |
| SHM | DSPCOMS2 | `0x075F` |
| SHM | DSPSEM_INT4 | `0x075B` |
| SHM | DSPSEMSR4 | `0x0759` |
| SHM | RWBB | `0x0756` |
| SHM | UCCOMR2 | `0x075E` |
| SHM | UCCOMS2 | `0x075D` |
| SHM | UCSEM_INT4 | `0x075C` |
| SHM | UCSEMSR4 | `0x075A` |

### LMU (`block_id=0x0F`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| A[20:2] | A(02) | `0x0F22` |
| A[20:2] | A(03) | `0x0F23` |
| A[20:2] | A(04) | `0x0F24` |
| A[20:2] | A(05) | `0x0F25` |
| A[20:2] | A(06) | `0x0F26` |
| A[20:2] | A(07) | `0x0F27` |
| A[20:2] | A(08) | `0x0F28` |
| A[20:2] | A(09) | `0x0F29` |
| A[20:2] | A(10) | `0x0F2A` |
| A[20:2] | A(11) | `0x0F2B` |
| A[20:2] | A(12) | `0x0F2C` |
| A[20:2] | A(13) | `0x0F2D` |
| A[20:2] | A(14) | `0x0F2E` |
| A[20:2] | A(15) | `0x0F2F` |
| A[20:2] | A(16) | `0x0F30` |
| A[20:2] | A(17) | `0x0F31` |
| A[20:2] | A(18) | `0x0F32` |
| A[20:2] | A(19) | `0x0F33` |
| A[20:2] | A(20) | `0x0F34` |
| BE[3:0] | BE(0) | `0x0F1E` |
| BE[3:0] | BE(1) | `0x0F1F` |
| BE[3:0] | BE(2) | `0x0F20` |
| BE[3:0] | BE(3) | `0x0F21` |
| WIB[3:0] | WIB(0) | `0x0F1A` |
| WIB[3:0] | WIB(1) | `0x0F1B` |
| WIB[3:0] | WIB(2) | `0x0F1C` |
| WIB[3:0] | WIB(3) | `0x0F1D` |
| Built In Self Test | MBIST_CONTROL | `0x0F0C` |
| Built In Self Test | MBIST_DEBUG | `0x0F09` |
| Built In Self Test | MBIST_DONE | `0x0F06` |
| Built In Self Test | MBIST_FAIL | `0x0F07` |
| Built In Self Test | MBIST_NOGO | `0x0F08` |
| Built In Self Test | MBIST_TDO0 | `0x0F0A` |
| Built In Self Test | MBIST_TDO1 | `0x0F0B` |
| Miscellaneous | BOOT_ROM | `0x0F0F` |
| Miscellaneous | CLK_EN | `0x0F03` |
| Miscellaneous | CLK_RAM | `0x0F13` |
| Miscellaneous | CLK_ROM | `0x0F12` |
| Miscellaneous | CMD0 | `0x0F18` |
| Miscellaneous | CMD1 | `0x0F19` |
| Miscellaneous | CMDVAL | `0x0F10` |
| Miscellaneous | HCLK | `0x0F11` |
| Miscellaneous | LMU_CMDACK | `0x0F02` |
| Miscellaneous | LMU_RSPVAL | `0x0F05` |
| Miscellaneous | MEM_FUSE_EN | `0x0F0D` |
| Miscellaneous | PDFT_SCAN_MODE_I_MUX | `0x0F04` |
| Miscellaneous | SCCU_VRAIL_EN | `0x0F01` |

### ML-AHB (`block_id=0x0E`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Cerberus | ADAT_CERB_HBURST(0) | `0x0E05` |
| Cerberus | ADAT_CERB_HBURST(1) | `0x0E06` |
| Cerberus | ADAT_CERB_HBURST(2) | `0x0E07` |
| Cerberus | ADAT_CERB_HGRANT | `0x0E10` |
| Cerberus | ADAT_CERB_HPROT(0) | `0x0E0B` |
| Cerberus | ADAT_CERB_HPROT(1) | `0x0E0C` |
| Cerberus | ADAT_CERB_HPROT(2) | `0x0E0D` |
| Cerberus | ADAT_CERB_HPROT(3) | `0x0E0E` |
| Cerberus | ADAT_CERB_HREADY | `0x0E60` |
| Cerberus | ADAT_CERB_HRESP(0) | `0x0E63` |
| Cerberus | ADAT_CERB_HRESP(1) | `0x0E64` |
| Cerberus | ADAT_CERB_HSIZE(0) | `0x0E08` |
| Cerberus | ADAT_CERB_HSIZE(1) | `0x0E09` |
| Cerberus | ADAT_CERB_HSIZE(2) | `0x0E0A` |
| Cerberus | ADAT_CERB_HTRANS(0) | `0x0E61` |
| Cerberus | ADAT_CERB_HTRANS(1) | `0x0E62` |
| Cerberus | ADAT_CERB_WRITE | `0x0E18` |
| ARM | ARM_FIQ_N | `0x0E02` |
| ARM | ARM_IRQ_N | `0x0E01` |
| ARM | ARM_ONLY_ACK | `0x0E33` |
| ARM | ARM_ONLY_REQ | `0x0E32` |
| Camera I/F | CIF_HGRANTM | `0x0E14` |
| Camera I/F | CIF_HREADY | `0x0E74` |
| Camera I/F | CIF_HRESP(0) | `0x0E77` |
| Camera I/F | CIF_HRESP(1) | `0x0E78` |
| Camera I/F | CIF_HTRANS(0) | `0x0E75` |
| Camera I/F | CIF_HTRANS(1) | `0x0E76` |
| Camera I/F | CIF_HWRITE | `0x0E1C` |
| DMA | DMA_CLR_DIF_RX | `0x0E44` |
| DMA | DMA_CLR_DIF_TX | `0x0E45` |
| DMA | DMA_CLR_FCDP_CMD | `0x0E48` |
| DMA | DMA_CLR_FCDP_DAT | `0x0E49` |
| DMA | DMA_CLR_FIRDA | `0x0E46` |
| DMA | DMA_CLR_GPRS_BUFIN | `0x0E56` |
| DMA | DMA_CLR_GPRS_BUFOUT | `0x0E57` |
| DMA | DMA_CLR_GPTU | `0x0E55` |
| DMA | DMA_CLR_I2C | `0x0E47` |
| DMA | DMA_CLR_MMC | `0x0E43` |
| DMA | DMA_CLR_MMIF | `0x0E40` |
| DMA | DMA_CLR_SSC0_RX | `0x0E4E` |
| DMA | DMA_CLR_SSC0_TX | `0x0E4F` |
| DMA | DMA_CLR_USART0_RX | `0x0E4A` |
| DMA | DMA_CLR_USART0_TX | `0x0E4B` |
| DMA | DMA_CLR_USART1_RX | `0x0E4C` |
| DMA | DMA_CLR_USART1_TX | `0x0E4D` |
| DMA | DMA_CLR_USB_EP1 | `0x0E50` |
| DMA | DMA_CLR_USB_EP2 | `0x0E51` |
| DMA | DMA_CLR_USB_EP3 | `0x0E52` |
| DMA | DMA_CLR_USB_EP4 | `0x0E53` |
| DMA | DMA_CLR_USIF_RX | `0x0E41` |
| DMA | DMA_CLR_USIF_TX | `0x0E42` |
| DMA | DMA_CLR_USIM_OK | `0x0E54` |
| DMA | DMA_M1_HWRITE | `0x0E1A` |
| DMA | DMA_M2_HWRITE | `0x0E1B` |
| DMA 1 | DMA1_CH0_INT | `0x0E38` |
| DMA 1 | DMA1_CH1_INT | `0x0E39` |
| DMA 1 | DMA1_CH2_INT | `0x0E3A` |
| DMA 1 | DMA1_CH3_INT | `0x0E3B` |
| DMA 1 | DMA1_CH4_INT | `0x0E3C` |
| DMA 1 | DMA1_CH5_INT | `0x0E3D` |
| DMA 1 | DMA1_CH6_INT | `0x0E3E` |
| DMA 1 | DMA1_CH7_INT | `0x0E3F` |
| DMA 1 | DMA1_HGRANTM | `0x0E12` |
| DMA 1 | DMA1_HREADY | `0x0E6A` |
| DMA 1 | DMA1_HRESP(0) | `0x0E6D` |
| DMA 1 | DMA1_HRESP(1) | `0x0E6E` |
| DMA 1 | DMA1_HSEL | `0x0E26` |
| DMA 1 | DMA1_HTRANS(0) | `0x0E6B` |
| DMA 1 | DMA1_HTRANS(1) | `0x0E6C` |
| DMA 1 | DMAC1_ERR_INT | `0x0E37` |
| DMA 2 | DMA2_HGRANTM | `0x0E13` |
| DMA 2 | DMA2_HREADY | `0x0E6F` |
| DMA 2 | DMA2_HRESP(0) | `0x0E72` |
| DMA 2 | DMA2_HRESP(1) | `0x0E73` |
| DMA 2 | DMA2_HTRANS(0) | `0x0E70` |
| DMA 2 | DMA2_HTRANS(1) | `0x0E71` |
| AINS | AINS_HGRANT | `0x0E11` |
| AINS | AINS_HREADY | `0x0E65` |
| AINS | AINS_HRESP(0) | `0x0E68` |
| AINS | AINS_HRESP(1) | `0x0E69` |
| AINS | AINS_HTRANS(0) | `0x0E66` |
| AINS | AINS_HTRANS(1) | `0x0E67` |
| AINS | AINS_HWRITE | `0x0E19` |
| FCDP | FCDP_HGRANTM | `0x0E15` |
| FCDP | FCDP_HREADY | `0x0E79` |
| FCDP | FCDP_HRESP(0) | `0x0E7C` |
| FCDP | FCDP_HRESP(1) | `0x0E7D` |
| FCDP | FCDP_HTRANS(0) | `0x0E7A` |
| FCDP | FCDP_HTRANS(1) | `0x0E7B` |
| FCDP | FCDP_HWRITE | `0x0E1D` |
| FPI | FPI1_EMPTY | `0x0E34` |
| FPI | FPI1_HREADYOUT | `0x0E2B` |
| FPI | FPI1_HSEL | `0x0E22` |
| FPI | FPI2_EMPTY | `0x0E36` |
| FPI | FPI2_HSEL | `0x0E23` |
| FPI | FPI3_EMPTY | `0x0E35` |
| FPI | FPI3_HSEL | `0x0E24` |
| FPI | FPI4_HREADYOUT | `0x0E2D` |
| FPI | FPI5_HREADYOUT | `0x0E2C` |
| Miscellaneous | ABW_HSEL | `0x0E28` |
| Miscellaneous | AHB_HREADYOUT | `0x0E31` |
| Miscellaneous | AHB_PER_HSEL | `0x0E25` |
| Miscellaneous | EBU_HREADYOUT | `0x0E29` |
| Miscellaneous | EBU_HSEL | `0x0E20` |
| Miscellaneous | HCLK | `0x0E04` |
| Miscellaneous | HREADY_ABW | `0x0E2F` |
| Miscellaneous | HREADY_DMA | `0x0E30` |
| Miscellaneous | ICU_HREADYOUT | `0x0E2E` |
| Miscellaneous | ICU_HSEL | `0x0E27` |
| Miscellaneous | LMU_HREADYOUT | `0x0E2A` |
| Miscellaneous | LMU_HSEL | `0x0E21` |

### SCU (`block_id=0x09`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Interrupt | DSP_TOMCU0 | `0x0913` |
| Interrupt | DSP_TOMCU1 | `0x0914` |
| Interrupt | DSP_TOMCU2 | `0x0915` |
| Interrupt | DSP_TOMCU3 | `0x0916` |
| Interrupt | DSP_TOMCU4 | `0x0917` |
| Interrupt | DSP_TOMCU5 | `0x0918` |
| Interrupt | EBU_INT_REQ | `0x091B` |
| Interrupt | EBU_IRQ | `0x091C` |
| Interrupt | EIRQ0 | `0x090D` |
| Interrupt | EIRQ1 | `0x090E` |
| Interrupt | EIRQ2 | `0x090F` |
| Interrupt | EIRQ3 | `0x0910` |
| Interrupt | EIRQ4 | `0x0911` |
| Interrupt | EIRQ5 | `0x0969` |
| Interrupt | EIRQ6 | `0x096A` |
| Interrupt | EIRQ7 | `0x096B` |
| Interrupt | EIRQ8 | `0x0912` |
| Interrupt | PDSPIRQ0 | `0x0901` |
| Interrupt | PDSPIRQ1 | `0x0902` |
| Interrupt | PDSPIRQ2 | `0x0903` |
| Interrupt | PDSP_INT0 | `0x0907` |
| Interrupt | PDSP_INT1 | `0x0908` |
| Interrupt | PDSP_INT2 | `0x0909` |
| Interrupt | SDSPIRQ0 | `0x0904` |
| Interrupt | SDSPIRQ1 | `0x0905` |
| Interrupt | SDSPIRQ2 | `0x0906` |
| Interrupt | SDSP_INT0 | `0x090A` |
| Interrupt | SDSP_INT1 | `0x090B` |
| Interrupt | SDSP_INT2 | `0x090C` |
| Interrupt | WDT_IRQ0 | `0x0919` |
| Interrupt | WDT_IRQ1 | `0x091A` |
| Reset | AUX_RES | `0x091F` |
| Reset | RST_AHB_PER_N | `0x0968` |
| Reset | RST_ANA_N | `0x092F` |
| Reset | RST_BOOT_N | `0x092C` |
| Reset | RST_CGU_N | `0x092E` |
| Reset | RST_CON_N | `0x0923` |
| Reset | RST_DBG_N | `0x0925` |
| Reset | RST_DISP_N | `0x0937` |
| Reset | RST_DMA1_N | `0x0938` |
| Reset | RST_DMA2_N | `0x0939` |
| Reset | RST_DMA3_N | `0x093A` |
| Reset | RST_DSP_N | `0x0926` |
| Reset | RST_FIRDA_N | `0x0966` |
| Reset | RST_FPI_N | `0x0928` |
| Reset | RST_I2C_N | `0x0967` |
| Reset | RST_MMCI_N | `0x0936` |
| Reset | RST_PADCTL_N | `0x092A` |
| Reset | RST_PADCTL_OPT_N | `0x092B` |
| Reset | RST_PLL_N | `0x0929` |
| Reset | RST_RTC_N | `0x092D` |
| Reset | RST_SIM_N | `0x0927` |
| Reset | RST_SSC0_N | `0x0932` |
| Reset | RST_SSC1_N | `0x0933` |
| Reset | RST_STM_N | `0x0924` |
| Reset | RST_USART0_N | `0x0934` |
| Reset | RST_USART1_N | `0x0935` |
| Reset | RST_USB_N | `0x0930` |
| Reset | SCCU_RESA | `0x0921` |
| Reset | SCCU_RESD | `0x0920` |
| Reset | SFT_RES_REQ | `0x0922` |
| Miscellaneous | AHB_AOM_ACK | `0x0945` |
| Miscellaneous | BOOTOPT0 | `0x093F` |
| Miscellaneous | BOOTOPT1 | `0x0940` |
| Miscellaneous | BOOT_ROM | `0x0941` |
| Miscellaneous | CLK_CPU_EN_DEL_O | `0x0958` |
| Miscellaneous | CLK_GSM_ON | `0x0959` |
| Miscellaneous | EBU_ST | `0x0943` |
| Miscellaneous | EBU_SY_REQ | `0x0942` |
| Miscellaneous | ENDINIT | `0x093E` |
| Miscellaneous | FPI_SLEEP_N | `0x093D` |
| Miscellaneous | FR_ANA_OUT | `0x094F` |
| Miscellaneous | FR_CORE_OUT | `0x094E` |
| Miscellaneous | HWWUP | `0x094C` |
| Miscellaneous | OCDS_E_N | `0x0947` |
| Miscellaneous | OCDS_P_SUSPEND | `0x0946` |
| Miscellaneous | PMST0 | `0x093B` |
| Miscellaneous | PMST1 | `0x093C` |
| Miscellaneous | PREWUP_INT_O | `0x0965` |
| Miscellaneous | PRE_WAKEUP | `0x094B` |
| Miscellaneous | RESET_13 | `0x0953` |
| Miscellaneous | RESET_32 | `0x0952` |
| Miscellaneous | RES_UCSLP | `0x0956` |
| Miscellaneous | RES_UCWUP | `0x0957` |
| Miscellaneous | SCCU_RESA_O | `0x0949` |
| Miscellaneous | SCCU_RESD_O | `0x0948` |
| Miscellaneous | SCCU_SHAP_EN_DEL | `0x095A` |
| Miscellaneous | SCCU_VCXO_EN_DEL | `0x094D` |
| Miscellaneous | SLPRST | `0x0951` |
| Miscellaneous | SLP_VCXO_OFF | `0x094A` |
| Miscellaneous | STATUS(0) | `0x0960` |
| Miscellaneous | STATUS(1) | `0x0961` |
| Miscellaneous | STATUS(2) | `0x0962` |
| Miscellaneous | STATUS(3) | `0x0963` |
| Miscellaneous | STATUS(4) | `0x0964` |
| Miscellaneous | SW_AOM_REQ | `0x0944` |
| Miscellaneous | TSTSTATE13M(0) | `0x095C` |
| Miscellaneous | TSTSTATE13M(1) | `0x095D` |
| Miscellaneous | TSTSTATE32K(0) | `0x095E` |
| Miscellaneous | TSTSTATE32K(1) | `0x095F` |
| Miscellaneous | WD_DOUBLE_ERR | `0x091E` |
| Miscellaneous | WD_ERR | `0x091D` |

### INPUT-MODE (`block_id=0x00`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| N/A | N/A | `0x8000` |
## S-GOLD Lite

Total: 11 blocks, 699 signals.

### CGU (`block_id=0x08`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| N/A | CLK32KOUT_O | `0x0817` |
| N/A | CLK_32K_I | `0x0802` |
| N/A | CLK_48M_O | `0x080D` |
| N/A | CLK_6M5_TRIG_O | `0x081A` |
| N/A | CLK_AFC_O | `0x0813` |
| N/A | CLK_ARM_O | `0x080A` |
| N/A | CLK_CLKOUT0_O | `0x0815` |
| N/A | CLK_CLKOUT1_O | `0x0816` |
| N/A | CLK_CON_O | `0x0809` |
| N/A | CLK_DSP_O | `0x0808` |
| N/A | CLK_EBU_O | `0x080C` |
| N/A | CLK_FPI1_O | `0x080E` |
| N/A | CLK_FPI2_O | `0x0812` |
| N/A | CLK_FPI3_O | `0x0810` |
| N/A | CLK_IN_1 | `0x0801` |
| N/A | CLK_MS_O | `0x0814` |
| N/A | CLK_PHS1_I | `0x0804` |
| N/A | CLK_PHS2_I | `0x0805` |
| N/A | CLK_PHS3_I | `0x0806` |
| N/A | CLK_PHS4_I | `0x0807` |
| N/A | CLK_PLL_I | `0x0803` |
| N/A | EN_AHB_O | `0x080B` |
| N/A | EN_FPI3_O | `0x0811` |
| N/A | F32KON_O | `0x0818` |

### DSP (`block_id=0x00`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| DSP Interrupt 1 | ciph_int | `0x0024` |
| DSP Interrupt 1 | dspin0_int | `0x0028` |
| DSP Interrupt 1 | dspin1_int | `0x0029` |
| DSP Interrupt 1 | dtmr10_int | `0x0025` |
| DSP Interrupt 1 | dtmr11_int | `0x0026` |
| DSP Interrupt 1 | dtmr2_int | `0x0027` |
| DSP Interrupt 1 | monin1_int | `0x002A` |
| DSP Interrupt 1 | monin2_int | `0x002B` |
| DSP Interrupt 1 | monin3_int | `0x002C` |
| DSP Interrupt 1 | monin4_int | `0x002D` |
| DSP Interrupt 2 | intfw0 | `0x002E` |
| DSP Interrupt 2 | intfw1 | `0x002F` |
| DSP Interrupt 2 | intfw2 | `0x0030` |
| DSP Interrupt 2 | intfw3 | `0x0031` |
| DSP Interrupt 2 | intfw4 | `0x0032` |
| DSP Interrupt 2 | intfw5 | `0x0033` |
| DSP Interrupt 2 | intfw6 | `0x0034` |
| DSP Interrupt 2 | intfw7 | `0x0035` |
| DSP Interrupt 2 | intfw8 | `0x0036` |
| DSP Interrupt 2 | intfw9 | `0x0037` |
| DSP Interrupt 2 | intfw10 | `0x0038` |
| DSP Interrupt 2 | intfw11 | `0x0039` |
| DSP Interrupt 2 | intfw12 | `0x003A` |
| DSP Interrupt 2 | intfw13 | `0x003B` |
| DSP Interrupt 2 | intfw14 | `0x003C` |
| DSP Interrupt 2 | intfw15 | `0x003D` |
| DSP Interrupt A0 | bb_full_int | `0x0013` |
| DSP Interrupt A0 | chadec_int | `0x000D` |
| DSP Interrupt A0 | codon_int | `0x000B` |
| DSP Interrupt A0 | eq_int | `0x000E` |
| DSP Interrupt A0 | eqon_int | `0x000F` |
| DSP Interrupt A0 | fcon_int | `0x0010` |
| DSP Interrupt A0 | frame_int | `0x0005` |
| DSP Interrupt A0 | mcu0_int | `0x0001` |
| DSP Interrupt A0 | mcu1_int | `0x0002` |
| DSP Interrupt A0 | mcu2_int | `0x0003` |
| DSP Interrupt A0 | mcu3_int | `0x0004` |
| DSP Interrupt A0 | modu_int | `0x000C` |
| DSP Interrupt A0 | monon_int | `0x0011` |
| DSP Interrupt A0 | scon_int | `0x0012` |
| DSP Interrupt B0 | i2s1rx_int | `0x0014` |
| DSP Interrupt B0 | i2s1tx_int | `0x0015` |
| DSP Interrupt B0 | i2s2rx_int | `0x0016` |
| DSP Interrupt B0 | i2s2tx_int | `0x0017` |
| DSP Interrupt B0 | i2s3tx_int | `0x0019` |
| DSP Interrupt B0 | ssc1_eir_int | `0x0022` |
| DSP Interrupt B0 | ssc1_rir_int | `0x0020` |
| DSP Interrupt B0 | ssc1_tir_int | `0x0021` |
| DSP Interrupt B0 | sysmcu_int | `0x0023` |
| DSP Interrupt B0 | vbrx_int | `0x001B` |
| DSP Interrupt B0 | vbtx_int | `0x001A` |
| DSP Miscellaneous | int0 | `0x003E` |
| DSP Miscellaneous | int1 | `0x003F` |
| DSP Miscellaneous | int2 | `0x0040` |
| DSP Miscellaneous | tomcu0_int | `0x0041` |
| DSP Miscellaneous | tomcu1_int | `0x0042` |
| DSP Miscellaneous | tomcu2_int | `0x0043` |
| DSP Miscellaneous | tomcu3_int | `0x0044` |

### DSP_PER (`block_id=0x02`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Audio-FE | mon_clk_vbrx | `0x020D` |
| Audio-FE | mon_clk_vbtx | `0x0210` |
| Audio-FE | mon_clk_vbtx_dith | `0x020F` |
| Audio-FE | mon_data_rrq_rx | `0x0203` |
| Audio-FE | mon_data_vbrx_left | `0x0208` |
| Audio-FE | mon_data_vbrx_lsb4(0) | `0x0209` |
| Audio-FE | mon_data_vbrx_lsb4(1) | `0x020A` |
| Audio-FE | mon_data_vbrx_lsb4(2) | `0x020B` |
| Audio-FE | mon_data_vbrx_lsb4(3) | `0x020C` |
| Audio-FE | mon_data_vbtx | `0x020E` |
| Audio-FE | mon_data_wrq_tx | `0x0204` |
| Audio-FE | mon_powerup_int_asmd_rx | `0x0206` |
| Audio-FE | mon_powerup_int_asmd_tx | `0x0205` |
| Audio-FE | mon_rxclear | `0x0201` |
| Audio-FE | mon_txclear | `0x0202` |
| Audio-FE | mon_zero_detect | `0x0207` |
| BB-Receive | data_bbrx_i_(0) | `0x0216` |
| BB-Receive | data_bbrx_i_(1) | `0x0217` |
| BB-Receive | data_bbrx_q_(0) | `0x0214` |
| BB-Receive | data_bbrx_q_(1) | `0x0215` |
| BB-Receive | mon_clk_bbrx | `0x0218` |
| BB-Receive | mon_clk_dither | `0x0212` |
| BB-Receive | mon_gclock_mix | `0x0213` |
| BB-Receive | mon_gclock_pll | `0x0211` |
| BB-Transmit | mon_clk_bbtx | `0x0255` |
| BB-Transmit | mon_clk_ms_mod | `0x0252` |
| BB-Transmit | mon_dpram_data(0) | `0x024D` |
| BB-Transmit | mon_dpram_req | `0x0251` |
| Cipher A53 | mon_a53_ciphering_ready | `0x024F` |
| Cipher A53 | mon_a53_dcry_valid | `0x0254` |
| Cipher A53 | mon_a53_ecry_dcry_data | `0x0253` |
| Cipher A53 | mon_a53_ecry_valid | `0x0263` |
| Cipher A53 | mon_a53_gclock_cipher | `0x024E` |
| Cipher A53 | mon_a53_gclock_dsp | `0x027E` |
| Cipher A53 | mon_a53_gclock_ram | `0x027F` |
| Cipher A53 | mon_a53_start_ciphering | `0x0250` |
| Cipher A53 | mon_init_kgcore | `0x024B` |
| Cipher A53 | mon_state_ctrl(0) | `0x0245` |
| Cipher A53 | mon_state_ctrl(1) | `0x0246` |
| Cipher A53 | mon_state_ctrl(2) | `0x0247` |
| DSP | mon_dspdis | `0x0266` |
| DSP | mon_dxap_page | `0x0265` |
| DSP | mon_en_reg_read | `0x0262` |
| DSP | mon_ppap_page | `0x0264` |
| DSP | mon_rst_dsp_mem_n | `0x0279` |
| DSP | mon_rst_dsp_n | `0x0277` |
| DSP | mon_rst_dsp_per_n | `0x0278` |
| DSP | mon_rst_ms_n | `0x0276` |
| GSM-Cipher | mon_ciphering_ready | `0x0219` |
| GSM-Cipher | mon_dcry_valid | `0x021C` |
| GSM-Cipher | mon_ecry_dcry_data | `0x021B` |
| GSM-Cipher | mon_ecry_valid | `0x021D` |
| GSM-Cipher | mon_gclock_cipher | `0x0220` |
| GSM-Cipher | mon_gclock_dsp | `0x021E` |
| GSM-Cipher | mon_gclock_ram | `0x021F` |
| GSM-Cipher | mon_start_ciphering | `0x021A` |
| I2S 1 | mon_cl1_state(0) | `0x022C` |
| I2S 1 | mon_cl1_state(1) | `0x022D` |
| I2S 1 | mon_cl1_state(2) | `0x022E` |
| I2S 1 | mon_cl2_state(0) | `0x0229` |
| I2S 1 | mon_cl2_state(1) | `0x022A` |
| I2S 1 | mon_cl2_state(2) | `0x022B` |
| I2S 1 | mon_i2s_data_rrq | `0x0230` |
| I2S 1 | mon_i2s_data_wrq | `0x022F` |
| I2S 1 | mon_rx_state(0) | `0x0221` |
| I2S 1 | mon_rx_state(1) | `0x0222` |
| I2S 1 | mon_rx_state(2) | `0x0223` |
| I2S 1 | mon_rx_state(3) | `0x0224` |
| I2S 1 | mon_tx_state(0) | `0x0225` |
| I2S 1 | mon_tx_state(1) | `0x0226` |
| I2S 1 | mon_tx_state(2) | `0x0227` |
| I2S 1 | mon_tx_state(3) | `0x0228` |
| I2S 2 | mon_cl1_state(0) | `0x023C` |
| I2S 2 | mon_cl1_state(1) | `0x023D` |
| I2S 2 | mon_cl1_state(2) | `0x023E` |
| I2S 2 | mon_cl2_state(0) | `0x0239` |
| I2S 2 | mon_cl2_state(1) | `0x023A` |
| I2S 2 | mon_cl2_state(2) | `0x023B` |
| I2S 2 | mon_i2s_data_rrq | `0x0240` |
| I2S 2 | mon_i2s_data_wrq | `0x023F` |
| I2S 2 | mon_rx_state(0) | `0x0231` |
| I2S 2 | mon_rx_state(1) | `0x0232` |
| I2S 2 | mon_rx_state(2) | `0x0233` |
| I2S 2 | mon_rx_state(3) | `0x0234` |
| I2S 2 | mon_tx_state(0) | `0x0235` |
| I2S 2 | mon_tx_state(1) | `0x0236` |
| I2S 2 | mon_tx_state(2) | `0x0237` |
| I2S 2 | mon_tx_state(3) | `0x0238` |
| I2S 3 | mon_cl1_state(0) | `0x0248` |
| I2S 3 | mon_cl1_state(1) | `0x0249` |
| I2S 3 | mon_cl1_state(2) | `0x024A` |
| I2S 3 | mon_i2s_data_rrq | `0x024C` |
| I2S 3 | mon_tx_state(0) | `0x0241` |
| I2S 3 | mon_tx_state(1) | `0x0242` |
| I2S 3 | mon_tx_state(2) | `0x0243` |
| I2S 3 | mon_tx_state(3) | `0x0244` |
| Ocem | abortn | `0x0274` |
| Ocem | btrapreqp | `0x0275` |
| Ocem | debugp | `0x0272` |
| Ocem | dtvmp | `0x0271` |
| Ocem | obootp | `0x0273` |
| Ocem | pdummyp | `0x0270` |
| Ocem | pprp | `0x026D` |
| Ocem | ppwp | `0x026C` |
| Ocem | psftp | `0x026B` |
| Ocem | pstatusp(0) | `0x0267` |
| Ocem | pstatusp(1) | `0x0268` |
| Ocem | pstatusp(2) | `0x0269` |
| Ocem | pstatusp(3) | `0x026A` |
| Seib | enocemm | `0x027D` |
| Seib | seib_j_abort_out_n | `0x027B` |
| Seib | seib_j_tdo_p | `0x027A` |
| Seib | waitp | `0x027C` |
| Viterbi-Decoder | mon_dec_active | `0x0258` |
| Viterbi-Decoder | mon_dec_busy | `0x0257` |
| Viterbi-Decoder | mon_pointer_dec_new | `0x0259` |
| Viterbi-Decoder | mon_res_dec | `0x025A` |
| Viterbi-Decoder | mon_res_dsp_int | `0x025B` |
| Viterbi-Decoder | mon_vh_status | `0x0256` |
| Viterbi-Equalizer | mon_eq_busy | `0x025D` |
| Viterbi-Equalizer | mon_pointer_left_hs_new | `0x025F` |
| Viterbi-Equalizer | mon_pointer_right_hs_new | `0x025E` |
| Viterbi-Equalizer | mon_res_dsp_int | `0x0261` |
| Viterbi-Equalizer | mon_res_eq | `0x0260` |
| Viterbi-Equalizer | mon_vh_status | `0x025C` |

### FPI1 (`block_id=0x04`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| DIF | DIF_E_IRQ | `0x040F` |
| DIF | DIF_R_DMACLR | `0x0423` |
| DIF | DIF_R_DMAREQ | `0x0421` |
| DIF | DIF_R_IRQ | `0x040E` |
| DIF | DIF_T_DMACLR | `0x0422` |
| DIF | DIF_T_DMAREQ | `0x0420` |
| DIF | DIF_T_IRQ | `0x040D` |
| DIF | DIF_TMO_IRQ | `0x0410` |
| FPI | cgu_clk_fpi1_rg | `0x0453` |
| FPI | fpi_ack_s[0] | `0x0458` |
| FPI | fpi_ack_s[1] | `0x0459` |
| FPI | FPI_DIF_SEL_N | `0x045F` |
| FPI | fpi_endinit_s | `0x045A` |
| FPI | fpi_opc_s[0] | `0x0454` |
| FPI | fpi_opc_s[1] | `0x0455` |
| FPI | fpi_opc_s[2] | `0x0456` |
| FPI | fpi_opc_s[3] | `0x0457` |
| FPI | fpi_rd_n_s | `0x0450` |
| FPI | fpi_rdy_s | `0x0452` |
| FPI | fpi_sleep_n_s | `0x045B` |
| FPI | FPI_SSC_SEL_N | `0x045E` |
| FPI | FPI_USART0_SEL_N | `0x0460` |
| FPI | FPI_USART1_SEL_N | `0x045D` |
| FPI | FPI_USB_SEL_N | `0x0462` |
| FPI | FPI_USIM_SEL_N | `0x0461` |
| FPI | fpi_wr_n_s | `0x0451` |
| FPI | ocds_p_suspend_s | `0x045C` |
| SSC0 | SSC0_E_IRQ | `0x040B` |
| SSC0 | SSC0_R_DMACLR | `0x041F` |
| SSC0 | SSC0_R_DMAREQ | `0x041D` |
| SSC0 | SSC0_R_IRQ | `0x040A` |
| SSC0 | SSC0_T_DMACLR | `0x041E` |
| SSC0 | SSC0_T_DMAREQ | `0x041C` |
| SSC0 | SSC0_T_IRQ | `0x0409` |
| SSC0 | SSC0_TMO_IRQ | `0x040C` |
| USART0 | USART0_ABDET_IRQ | `0x042F` |
| USART0 | USART0_ABST_IRQ | `0x042E` |
| USART0 | USART0_B_IRQ | `0x042B` |
| USART0 | USART0_CTS_IRQ | `0x0430` |
| USART0 | USART0_E_IRQ | `0x042D` |
| USART0 | USART0_R_DMACLR | `0x0427` |
| USART0 | USART0_R_DMAREQ | `0x0425` |
| USART0 | USART0_R_IRQ | `0x042C` |
| USART0 | USART0_T_DMACLR | `0x0426` |
| USART0 | USART0_T_DMAREQ | `0x0424` |
| USART0 | USART0_T_IRQ | `0x042A` |
| USART0 | USART0_TMO_IRQ | `0x0431` |
| USART1 | USART1_ABDET_IRQ | `0x0406` |
| USART1 | USART1_B_IRQ | `0x0402` |
| USART1 | USART1_CTS_IRQ | `0x0407` |
| USART1 | USART1_R_DMACLR | `0x041B` |
| USART1 | USART1_R_DMAREQ | `0x0419` |
| USART1 | USART1_R_IRQ | `0x0403` |
| USART1 | USART1_T_DMACLR | `0x041A` |
| USART1 | USART1_T_DMAREQ | `0x0418` |
| USART1 | USART1_T_IRQ | `0x0401` |
| USART1 | USART1_TMO_IRQ | `0x0408` |
| USART1 | USART1ABST_IRQ | `0x0405` |
| USART1 | USART1E_IRQ | `0x0404` |
| USB | usb_clk_rg | `0x044D` |
| USB | USB_DIF | `0x0464` |
| USB | USB_DMACLR[0] | `0x0441` |
| USB | USB_DMACLR[1] | `0x0442` |
| USB | USB_DMACLR[2] | `0x0443` |
| USB | USB_DMACLR[3] | `0x0444` |
| USB | USB_DMAREQ[0] | `0x0432` |
| USB | USB_DMAREQ[1] | `0x0433` |
| USB | USB_DMAREQ[2] | `0x0434` |
| USB | USB_DMAREQ[3] | `0x0435` |
| USB | USB_INT | `0x0416` |
| USIM | USIM_E_IRQ | `0x0413` |
| USIM | USIM_IN_IRQ | `0x0414` |
| USIM | USIM_OK_IRQ | `0x0415` |
| USIM | USIM_TXRX_DMACLR | `0x0429` |
| USIM | USIM_TXRX_DMAREQ | `0x0428` |

### FPI21 (`block_id=0x0A`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| CapCom | CC0T0SRC | `0x0A28` |
| CapCom | CC0T1SRC | `0x0A29` |
| CapCom | CC0SRC0 | `0x0A2A` |
| CapCom | CC0SRC1 | `0x0A2B` |
| CapCom | CC0SRC2 | `0x0A2C` |
| CapCom | CC0SRC3 | `0x0A2D` |
| CapCom | CC0SRC4 | `0x0A2E` |
| CapCom | CC0SRC5 | `0x0A2F` |
| CapCom | CC0SRC6 | `0x0A30` |
| CapCom | CC0SRC7 | `0x0A31` |
| CapCom | CC1T0SRC | `0x0A32` |
| CapCom | CC1T1SRC | `0x0A33` |
| CapCom | CC1SRC0 | `0x0A34` |
| CapCom | CC1SRC1 | `0x0A35` |
| CapCom | CC1SRC2 | `0x0A36` |
| CapCom | CC1SRC3 | `0x0A37` |
| CapCom | CC1SRC4 | `0x0A38` |
| CapCom | CC1SRC5 | `0x0A39` |
| CapCom | CC1SRC6 | `0x0A3A` |
| CapCom | CC1SRC7 | `0x0A3B` |
| CGU | sig4mon_cgu_fpi(0) | `0x0A70` |
| CGU | sig4mon_cgu_fpi(1) | `0x0A71` |
| CGU | sig4mon_cgu_fpi(2) | `0x0A72` |
| CGU | sig4mon_cgu_fpi(3) | `0x0A73` |
| CGU | sig4mon_cgu_fpi(4) | `0x0A74` |
| CGU | sig4mon_cgu_fpi(5) | `0x0A75` |
| CGU | sig4mon_cgu_fpi(6) | `0x0A76` |
| CGU | sig4mon_cgu_fpi(7) | `0x0A77` |
| FPI | fpi2_A0 | `0x0A12` |
| FPI | fpi2_A31 | `0x0A14` |
| FPI | fpi2_A7 | `0x0A13` |
| FPI | fpi2_ACK0 | `0x0A0D` |
| FPI | fpi2_ACK1 | `0x0A0E` |
| FPI | fpi2_D0 | `0x0A15` |
| FPI | fpi2_D7 | `0x0A16` |
| FPI | fpi2_ENDINIT | `0x0A0B` |
| FPI | fpi2_GPTU0_DMAREQ | `0x0A01` |
| FPI | fpi2_GPTU1_DMAREQ | `0x0A02` |
| FPI | fpi2_OCDS_SUSPEND | `0x0A0C` |
| FPI | fpi2_OPC0 | `0x0A06` |
| FPI | fpi2_OPC1 | `0x0A07` |
| FPI | fpi2_OPC2 | `0x0A08` |
| FPI | fpi2_OPC3 | `0x0A09` |
| FPI | fpi2_RD | `0x0A03` |
| FPI | fpi2_RDY | `0x0A0F` |
| FPI | fpi2_SLEEP_n | `0x0A0A` |
| FPI | fpi2_SVM | `0x0A11` |
| FPI | fpi2_TOUT | `0x0A10` |
| FPI | fpi2_WR | `0x0A04` |
| GPTU | GT0SRC0 | `0x0A46` |
| GPTU | GT0SRC1 | `0x0A47` |
| GPTU | GT0SRC2 | `0x0A48` |
| GPTU | GT0SRC3 | `0x0A49` |
| GPTU | GT0SRC4 | `0x0A4A` |
| GPTU | GT0SRC5 | `0x0A4B` |
| GPTU | GT0SRC6 | `0x0A4C` |
| GPTU | GT0SRC7 | `0x0A4D` |
| GPTU | GT1SRC0 | `0x0A4E` |
| GPTU | GT1SRC1 | `0x0A4F` |
| GPTU | GT1SRC2 | `0x0A50` |
| GPTU | GT1SRC3 | `0x0A51` |
| GPTU | GT1SRC4 | `0x0A52` |
| GPTU | GT1SRC5 | `0x0A53` |
| GPTU | GT1SRC6 | `0x0A54` |
| GPTU | GT1SRC7 | `0x0A55` |
| I2C | I2C_INT_D_O | `0x0A6C` |
| I2C | I2C_INT_E_O | `0x0A6E` |
| I2C | I2C_INT_P_O | `0x0A6D` |
| Keypad | keypad_mon(0) | `0x0A58` |
| Keypad | keypad_mon(1) | `0x0A59` |
| Keypad | keypad_mon(2) | `0x0A5A` |
| Keypad | keypad_mon(3) | `0x0A5B` |
| Keypad | keypad_mon(4) | `0x0A5C` |
| Keypad | keypad_mon(5) | `0x0A5D` |
| Keypad | keypad_mon(6) | `0x0A5E` |
| Keypad | keypad_mon(7) | `0x0A5F` |
| Meas./Ana. | ANA_EOC | `0x0A22` |
| Meas./Ana. | ana_meas_penirq_n | `0x0A23` |
| Meas./Ana. | GSM_ADCTRIG | `0x0A24` |
| Meas./Ana. | MEAS_CLK | `0x0A20` |
| Meas./Ana. | MEAS_SOC | `0x0A21` |
| RTC | RTC0IR | `0x0A62` |
| RTC | RTC1IR | `0x0A63` |
| RTC | RTC2IR | `0x0A64` |
| RTC | RTC3IR | `0x0A65` |
| RTC | RTC_CLK | `0x0A68` |
| RTC | RTC_INT | `0x0A60` |
| RTC | RTC_REF_CLK | `0x0A69` |
| RTC | RTC_T14INT | `0x0A61` |
| RTC | RTCALARM | `0x0A66` |
| RTC | RTCBAD | `0x0A67` |

### FPI22 (`block_id=0x0B`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| GPIO | gpio_pad_01 | `0x0B01` |
| GPIO | gpio_pad_02 | `0x0B02` |
| GPIO | gpio_pad_03 | `0x0B03` |
| GPIO | gpio_pad_04 | `0x0B04` |
| GPIO | gpio_pad_05 | `0x0B05` |
| GPIO | gpio_pad_06 | `0x0B06` |
| GPIO | gpio_pad_07 | `0x0B07` |
| GPIO | gpio_pad_08 | `0x0B08` |
| GPIO | gpio_pad_09 | `0x0B09` |
| GPIO | gpio_pad_10 | `0x0B0A` |
| GPIO | gpio_pad_11 | `0x0B0B` |
| GPIO | gpio_pad_12 | `0x0B0C` |
| GPIO | gpio_pad_13 | `0x0B0D` |
| GPIO | gpio_pad_14 | `0x0B0E` |
| GPIO | gpio_pad_15 | `0x0B0F` |
| GPIO | gpio_pad_16 | `0x0B10` |
| GPIO | gpio_pad_17 | `0x0B11` |
| GPIO | gpio_pad_18 | `0x0B12` |
| GPIO | gpio_pad_19 | `0x0B13` |
| GPIO | gpio_pad_20 | `0x0B14` |
| GPIO | gpio_pad_21 | `0x0B15` |
| GPIO | gpio_pad_22 | `0x0B16` |
| GPIO | gpio_pad_23 | `0x0B17` |
| GPIO | gpio_pad_24 | `0x0B18` |
| GPIO | gpio_pad_25 | `0x0B19` |
| GPIO | gpio_pad_26 | `0x0B1A` |
| GPIO | gpio_pad_27 | `0x0B1B` |
| GPIO | gpio_pad_28 | `0x0B1C` |
| GPIO | gpio_pad_29 | `0x0B1D` |
| GPIO | gpio_pad_30 | `0x0B1E` |
| GPIO | gpio_pad_31 | `0x0B1F` |
| GPIO | gpio_pad_32 | `0x0B20` |
| GPIO | gpio_pad_33 | `0x0B21` |
| GPIO | gpio_pad_34 | `0x0B22` |
| GPIO | gpio_pad_35 | `0x0B23` |
| GPIO | gpio_pad_36 | `0x0B24` |
| GPIO | gpio_pad_37 | `0x0B25` |
| GPIO | gpio_pad_38 | `0x0B26` |
| GPIO | gpio_pad_39 | `0x0B27` |
| GPIO | gpio_pad_40 | `0x0B28` |
| GPIO | gpio_pad_41 | `0x0B29` |
| GPIO | gpio_pad_42 | `0x0B2A` |
| GPIO | gpio_pad_43 | `0x0B2B` |
| GPIO | gpio_pad_44 | `0x0B2C` |
| GPIO | gpio_pad_45 | `0x0B2D` |
| GPIO | gpio_pad_46 | `0x0B2E` |
| GPIO | gpio_pad_47 | `0x0B2F` |
| GPIO | gpio_pad_48 | `0x0B30` |
| GPIO | gpio_pad_49 | `0x0B31` |
| GPIO | gpio_pad_50 | `0x0B32` |
| GPIO | gpio_pad_51 | `0x0B33` |
| GPIO | gpio_pad_52 | `0x0B34` |
| GPIO | gpio_pad_53 | `0x0B35` |
| GPIO | gpio_pad_54 | `0x0B36` |
| GPIO | gpio_pad_55 | `0x0B37` |
| GPIO | gpio_pad_56 | `0x0B38` |
| GPIO | gpio_pad_57 | `0x0B39` |
| GPIO | gpio_pad_58 | `0x0B3A` |
| GPIO | gpio_pad_59 | `0x0B3B` |
| GPIO | gpio_pad_60 | `0x0B3C` |
| GPIO | gpio_pad_61 | `0x0B3D` |
| GPIO | gpio_pad_62 | `0x0B3E` |
| GPIO | gpio_pad_63 | `0x0B3F` |
| GPIO | gpio_pad_64 | `0x0B40` |
| GPIO | gpio_pad_65 | `0x0B41` |
| GPIO | gpio_pad_66 | `0x0B42` |
| GPIO | gpio_pad_67 | `0x0B43` |
| GPIO | gpio_pad_68 | `0x0B44` |
| GPIO | gpio_pad_69 | `0x0B45` |
| GPIO | gpio_pad_70 | `0x0B46` |
| GPIO | gpio_pad_71 | `0x0B47` |
| GPIO | gpio_pad_72 | `0x0B48` |
| GPIO | gpio_pad_73 | `0x0B49` |
| GPIO | gpio_pad_74 | `0x0B4A` |
| GPIO | gpio_pad_75 | `0x0B4B` |
| GPIO | gpio_pad_76 | `0x0B4C` |
| GPIO | gpio_pad_77 | `0x0B4D` |

### FPI3 (`block_id=0x07`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| FPI | FPI3_A(0) | `0x070F` |
| FPI | FPI3_DATA_RD(0) | `0x0714` |
| FPI | FPI3_DATA_RD(7) | `0x0715` |
| FPI | FPI3_DATA_WR(0) | `0x0712` |
| FPI | FPI3_DATA_WR(7) | `0x0713` |
| FPI | FPI3_OPC(0) | `0x070B` |
| FPI | FPI3_OPC(1) | `0x070C` |
| FPI | FPI3_OPC(2) | `0x070D` |
| FPI | FPI3_OPC(3) | `0x070E` |
| FPI | FPI3_RD_n | `0x0706` |
| FPI | FPI3_RDY | `0x0708` |
| FPI | FPI3_WR_n | `0x0707` |
| FPI | FPI_A(7) | `0x0710` |
| FPI | FPI_A(31) | `0x0711` |
| FPI | FPI_ACK(0) | `0x0709` |
| FPI | FPI_ACK(1) | `0x070A` |
| FPI | FPI_ENDINIT | `0x0702` |
| FPI | FPI_SLEEP_n | `0x0701` |
| FPI | FPI_SVM | `0x0705` |
| FPI | FPI_TOUT | `0x0703` |
| FPI | OCDS_P_SUSPEND | `0x0704` |
| GPRS | GPRS_DMAREQ(0) | `0x0750` |
| GPRS | GPRS_DMAREQ(1) | `0x0751` |
| GPRS | GPRS_IRQ(0) | `0x074E` |
| GPRS | GPRS_IRQ(1) | `0x074F` |
| GPRS | gprs_kernel_irq (1E) | `0x071E` |
| GPRS | gprs_kernel_irq (1F) | `0x071F` |
| GSM | GSM_DSP_CODON | `0x0730` |
| GSM | GSM_DSP_FCON | `0x072D` |
| GSM | GSM_DSP_RXON | `0x072F` |
| GSM | GSM_DSP_SCON | `0x072E` |
| GSM | GSM_EQON | `0x072B` |
| GSM | GSM_INT_GP0 | `0x0721` |
| GSM | GSM_INT_GP1 | `0x0722` |
| GSM | GSM_INT_GP2 | `0x0723` |
| GSM | GSM_INT_GP3 | `0x0724` |
| GSM | GSM_INT_GP4 | `0x0725` |
| GSM | GSM_INT_GP5 | `0x0726` |
| GSM | GSM_INT_GP6 | `0x0727` |
| GSM | GSM_MEAS_ADCTRIG | `0x0734` |
| GSM | GSM_MONON | `0x072C` |
| GSM | GSM_RFSSCTINT | `0x0720` |
| GSM | GSM_SPCU_SLPSTART | `0x0731` |
| GSM | GSM_T_INT1 | `0x0728` |
| GSM | GSM_T_INT2 | `0x0729` |
| GSM | GSM_TXON | `0x072A` |
| PA Ramping | DCPA_DATA0 | `0x0736` |
| PA Ramping | DCPA_DATA1 | `0x0737` |
| PA Ramping | DCPA_DATA2 | `0x0738` |
| PA Ramping | DCPA_DATA3 | `0x0739` |
| PA Ramping | DCPA_DATA4 | `0x073A` |
| PA Ramping | DCPA_DATA5 | `0x073B` |
| PA Ramping | DCPA_DATA6 | `0x073C` |
| PA Ramping | DCPA_DATA7 | `0x073D` |
| PA Ramping | DCPA_DATA8 | `0x073E` |
| PA Ramping | DCPA_DATA9 | `0x073F` |
| PA Ramping | DCPA_DATA10 | `0x0740` |
| PA Ramping | DCPA_DATAVALID | `0x0741` |
| PA Ramping | PAR_CLR_INT_FSM | `0x0735` |
| PA Ramping | PAR_DAC0 | `0x0743` |
| PA Ramping | PAR_DAC1 | `0x0744` |
| PA Ramping | PAR_DAC2 | `0x0745` |
| PA Ramping | PAR_DAC3 | `0x0746` |
| PA Ramping | PAR_DAC4 | `0x0747` |
| PA Ramping | PAR_DAC5 | `0x0748` |
| PA Ramping | PAR_DAC6 | `0x0749` |
| PA Ramping | PAR_DAC7 | `0x074A` |
| PA Ramping | PAR_DAC8 | `0x074B` |
| PA Ramping | PAR_DAC9 | `0x074C` |
| PA Ramping | PAR_DAC10 | `0x074D` |
| PA Ramping | PAR_DAC_CLK | `0x0742` |
| SHM | CFS2 | `0x0761` |
| SHM | CSBA | `0x0754` |
| SHM | CSBB | `0x0755` |
| SHM | DSPCOMR2 | `0x0760` |
| SHM | DSPCOMS2 | `0x075F` |
| SHM | DSPSEM_INT4 | `0x075B` |
| SHM | DSPSEMSR4 | `0x0759` |
| SHM | RWBB | `0x0756` |
| SHM | UCCOMR2 | `0x075E` |
| SHM | UCCOMS2 | `0x075D` |
| SHM | UCSEM_INT4 | `0x075C` |
| SHM | UCSEMSR4 | `0x075A` |

### LMU (`block_id=0x0F`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| a[20:2] | a(02) | `0x0F22` |
| a[20:2] | a(03) | `0x0F23` |
| a[20:2] | a(04) | `0x0F24` |
| a[20:2] | a(05) | `0x0F25` |
| a[20:2] | a(06) | `0x0F26` |
| a[20:2] | a(07) | `0x0F27` |
| a[20:2] | a(08) | `0x0F28` |
| a[20:2] | a(09) | `0x0F29` |
| a[20:2] | a(10) | `0x0F2A` |
| a[20:2] | a(11) | `0x0F2B` |
| a[20:2] | a(12) | `0x0F2C` |
| a[20:2] | a(13) | `0x0F2D` |
| a[20:2] | a(14) | `0x0F2E` |
| a[20:2] | a(15) | `0x0F2F` |
| a[20:2] | a(16) | `0x0F30` |
| a[20:2] | a(17) | `0x0F31` |
| a[20:2] | a(18) | `0x0F32` |
| a[20:2] | a(19) | `0x0F33` |
| a[20:2] | a(20) | `0x0F34` |
| Miscellaneous | be_i(0) | `0x0F1E` |
| Miscellaneous | be_i(1) | `0x0F1F` |
| Miscellaneous | be_i(2) | `0x0F20` |
| Miscellaneous | be_i(3) | `0x0F21` |
| Miscellaneous | boot_rom_i | `0x0F0F` |
| Miscellaneous | clk_en_o | `0x0F03` |
| Miscellaneous | clk_ram_mon_o | `0x0F13` |
| Miscellaneous | clk_rom_mon_o | `0x0F12` |
| Miscellaneous | cmd_i(0) | `0x0F18` |
| Miscellaneous | cmd_i(1) | `0x0F19` |
| Miscellaneous | cmdval_i | `0x0F10` |
| Miscellaneous | hclk_mon_o | `0x0F11` |
| Miscellaneous | lmu_cmdack_o_tmp | `0x0F02` |
| Miscellaneous | lmu_rspval_o_tmp | `0x0F05` |
| Miscellaneous | mbist_control_i | `0x0F0C` |
| Miscellaneous | mbist_debug_i | `0x0F09` |
| Miscellaneous | mbist_done_o_tmp | `0x0F06` |
| Miscellaneous | mbist_fail_o_tmp | `0x0F07` |
| Miscellaneous | mbist_nogo_o_tmp | `0x0F08` |
| Miscellaneous | mbist_tdo_o_tmp | `0x0F0A` |
| Miscellaneous | mbist_tdo_o_tmp | `0x0F0B` |
| Miscellaneous | mem_fuse_en_i_mux | `0x0F0D` |
| Miscellaneous | pdft_scan_mode_i_mux | `0x0F04` |
| Miscellaneous | sccu_lmuram_vrail_en_i_mux | `0x0F01` |
| Miscellaneous | wib(0) | `0x0F1A` |
| Miscellaneous | wib(1) | `0x0F1B` |
| Miscellaneous | wib(2) | `0x0F1C` |
| Miscellaneous | wib(3) | `0x0F1D` |

### ML-AHB (`block_id=0x0E`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| N/A | adat_hgrant_tmp | `0x0E10` |
| N/A | adat_hwrite_i | `0x0E18` |
| N/A | ains_hgrant_tmp | `0x0E11` |
| N/A | ains_hwrite_i | `0x0E19` |
| N/A | ARM_FIQ_N | `0x0E02` |
| N/A | ARM_IRQ_N | `0x0E01` |
| N/A | ARM_ONLY_ACK_N | `0x0E31` |
| N/A | ARM_ONLY_REQ | `0x0E30` |
| N/A | dmac1_dmacinttc(0) | `0x0E38` |
| N/A | dmac1_dmacinttc(1) | `0x0E39` |
| N/A | dmac1_dmacinttc(2) | `0x0E3A` |
| N/A | dmac1_dmacinttc(3) | `0x0E3B` |
| N/A | dmac1_dmacinttc(4) | `0x0E3C` |
| N/A | dmac1_dmacinttc(5) | `0x0E3D` |
| N/A | dmac1_dmacinttc(6) | `0x0E3E` |
| N/A | dmac1_dmacinttc(7) | `0x0E3F` |
| N/A | dmac1_dmainterr | `0x0E50` |
| N/A | dmac1_hgrantm(0) | `0x0E12` |
| N/A | dmac1_hgrantm(1) | `0x0E13` |
| N/A | dmac1_hgrantm(2) | `0x0E14` |
| N/A | dmac1_hwritem | `0x0E1A` |
| N/A | ebu_hsel_tmp | `0x0E20` |
| N/A | fpi1_empty_tmp | `0x0E32` |
| N/A | fpi1_hsel | `0x0E22` |
| N/A | fpi2_empty_tmp | `0x0E36` |
| N/A | fpi2_hsel | `0x0E2B` |
| N/A | fpi3_empty_tmp | `0x0E35` |
| N/A | fpi3_hsel | `0x0E26` |
| N/A | HCLK | `0x0E04` |
| N/A | hsel_dmac1 | `0x0E28` |
| N/A | icu_hsel | `0x0E25` |
| N/A | lahb_hsel | `0x0E27` |
| N/A | lmu_hsel | `0x0E21` |

### SCU (`block_id=0x09`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| Interrupt | DSPIRQ0 | `0x0901` |
| Interrupt | DSPIRQ1 | `0x0902` |
| Interrupt | DSPIRQ2 | `0x0903` |
| Interrupt | DSP_INT0 | `0x0907` |
| Interrupt | DSP_INT1 | `0x0908` |
| Interrupt | DSP_INT2 | `0x0909` |
| Interrupt | DSP_INT3 | `0x090A` |
| Interrupt | EIRQ0 | `0x090D` |
| Interrupt | EIRQ1 | `0x090E` |
| Interrupt | EIRQ2 | `0x090F` |
| Interrupt | EIRQ3 | `0x0910` |
| Interrupt | EIRQ4 | `0x0911` |
| Interrupt | EIRQ5 | `0x0912` |
| Interrupt | DSP_TOMCU0 | `0x0913` |
| Interrupt | DSP_TOMCU1 | `0x0914` |
| Interrupt | DSP_TOMCU2 | `0x0915` |
| Interrupt | DSP_TOMCU3 | `0x0916` |
| Interrupt | WDT_IRQ0 | `0x0919` |
| Interrupt | WDT_IRQ1 | `0x091A` |
| Interrupt | EBU_INT_REQ | `0x091B` |
| Interrupt | EBU_IRQ | `0x091C` |
| Reset | AUX_RES | `0x091F` |
| Reset | SCCU_RESD | `0x0920` |
| Reset | SCCU_RESA | `0x0921` |
| Reset | SFT_RES_REQ | `0x0922` |
| Reset | RST_CON_N | `0x0923` |
| Reset | RST_STM_N | `0x0924` |
| Reset | RST_DBG_N | `0x0925` |
| Reset | RST_DSP_N | `0x0926` |
| Reset | RST_SIM_N | `0x0927` |
| Reset | RST_FPI_N | `0x0928` |
| Reset | RST_PLL_N | `0x0929` |
| Reset | RST_PADCTL_N | `0x092A` |
| Reset | RST_PADCTL_OPT_N | `0x092B` |
| Reset | RST_BOOT_N | `0x092C` |
| Reset | RST_RTC_N | `0x092D` |
| Reset | RST_CGU_N | `0x092E` |
| Reset | RST_ANA_N | `0x092F` |
| Reset | RST_USB_N | `0x0930` |
| Reset | RST_SSC0_N | `0x0932` |
| Reset | RST_USART0_N | `0x0934` |
| Reset | RST_USART1_N | `0x0935` |
| Reset | RST_DISP_N | `0x0937` |
| Reset | RST_DMA_N | `0x0938` |
| Miscellaneous | WD_ERR | `0x091D` |
| Miscellaneous | WD_DOUBLE_ERR | `0x091E` |
| Miscellaneous | PMST0 | `0x093B` |
| Miscellaneous | PMST1 | `0x093C` |
| Miscellaneous | FPI_SLEEP_N | `0x093D` |
| Miscellaneous | ENDINIT | `0x093E` |
| Miscellaneous | BOOTOPT0 | `0x093F` |
| Miscellaneous | BOOTOPT1 | `0x0940` |
| Miscellaneous | BOOT_ROM | `0x0941` |
| Miscellaneous | EBU_SY_REQ | `0x0942` |
| Miscellaneous | EBU_ST | `0x0943` |
| Miscellaneous | SW_AOM_REQ | `0x0944` |
| Miscellaneous | AHB_AOM_ACK | `0x0945` |
| Miscellaneous | OCDS_P_SUSPEND | `0x0946` |
| Miscellaneous | OCDS_E_N | `0x0947` |
| Miscellaneous | SPCU_RESD_O | `0x0948` |
| Miscellaneous | SPCU_RESA_O | `0x0949` |
| Miscellaneous | SLP_VCXO_OFF | `0x094A` |
| Miscellaneous | PRE_WAKEUP | `0x094B` |
| Miscellaneous | HWWUP | `0x094C` |
| Miscellaneous | SPCU_VCXO_EN_DEL | `0x094D` |
| Miscellaneous | FR_CORE_OUT | `0x094E` |
| Miscellaneous | FR_ANA_OUT | `0x094F` |
| Miscellaneous | SLPRST | `0x0951` |
| Miscellaneous | RESET_32 | `0x0952` |
| Miscellaneous | RESET_13 | `0x0953` |
| Miscellaneous | RES_UCSLP | `0x0956` |
| Miscellaneous | RES_UCWUP | `0x0957` |
| Miscellaneous | CLK_CPU_EN_DEL_O | `0x0958` |
| Miscellaneous | CLK_GSM_ON | `0x0959` |
| Miscellaneous | SPCU_SHAP_EN_DEL | `0x095A` |
| Miscellaneous | TSTSTATE13M(0) | `0x095C` |
| Miscellaneous | TSTSTATE13M(1) | `0x095D` |
| Miscellaneous | TSTSTATE32K(0) | `0x095E` |
| Miscellaneous | TSTSTATE32K(1) | `0x095F` |
| Miscellaneous | STATUS(0) | `0x0960` |
| Miscellaneous | STATUS(1) | `0x0961` |
| Miscellaneous | STATUS(2) | `0x0962` |
| Miscellaneous | STATUS(3) | `0x0963` |
| Miscellaneous | STATUS(4) | `0x0964` |
| Miscellaneous | PREWUP_INT_O | `0x0965` |

### INPUT-MODE (`block_id=0x00`)

| Subgroup | Signal | `MON_CR` |
|---|---|---:|
| N/A | N/A | `0x8000` |
