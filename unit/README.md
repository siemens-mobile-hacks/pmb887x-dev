# Peripheral tests

Run a test on real hardware:

```sh
./run.sh <test>
```

## Tests

| Test | Coverage |
| --- | --- |
| `stm` | System timer registers and counter views |
| `dmac` | DMA transfers, requests, linked lists, arbitration, and interrupts |
| `dsp` | Communication flags, firmware command handshake, shared RAM access widths, interrupt requests, and reset |
| `mod` | Common CLC, SRC, and SRB register blocks |
| `scu` | CPU identification, watchdog protection, mode transitions, counter, and status |
| `scu-reset` | Intentional software reset using the firmware reset-control sequence |
| `scu-wdt-reset` | Intentional watchdog reset; reaching the failure message means the reset did not occur |
| `adc` | ADC registers, conversion, interrupts, and battery voltage measurement |
| `afc` | AFC reference value, output enable, write mask, and firmware-safe nominal value |
| `pll` | PLL output branches, USB clock source and divider, lock state, and register layout |
| `rtc` | RTC registers, synchronous/asynchronous operation, timer chain, and interrupt sub-node |
| `vic` | VIC pending state, arbitration, priority masking, acknowledge, IRQ, and FIQ routing |
| `gptu` | Both GPTU instances, T0/T1 concatenation, T2 split mode, reload, and service requests |
| `sccu` | GSM sleep timer, ARM WFI, DSP power-down, standby-clock calibration, and wakeup interrupt |
| `tpu` | TDMA counter clock, modulo overflow, compare interrupts, correction, offset, and frame skip |
| `tpu-ram` | TPU RF/Timer RAM partitioning, widths, timing events, and address pointers |
| `tpu-rf-ssc` | TPU RF SSC direct transfers, formats, clocks, strobe selection, and completion interrupt |
| `tcm` | ITCM and DTCM region registers, memory access, remap, and overlay behavior |
| `usb` | sci-worx USB core and wrapper reset values, BROM start sequence, and interrupt enable masks |
| `i2c-v1` | I2Cv1 registers, hardware bits, IRQ transfers, and PMIC SMBus reads |
| `i2c-v2` | I2Cv2 registers, IRQ transfers, SMBus, FIFO modes, and bus scan |
| `i2c-v2-dma` | I2Cv2 SMBus transfers through DMA |
| `cfi-intel-buffer-abort` | Destructive Intel/ST E8/E9 cross-block buffered-program abort and recovery |
| `ssc` | SSC loopback, serial formats, FIFO modes, interrupts, and errors |
| `ssc-dma` | SSC full-duplex FIFO DMA, bursts, LLI, widths, statuses, and interrupts |
| `dif-v1` | DIFv1 loopback, FIFO modes, interrupts, errors, and complete 32-to-16 BMREG matrix |
| `dif-v1-dma` | DIFv1 full-duplex FIFO DMA, bursts, LLI, widths, statuses, and interrupts |
| `usart` | USART loopback, frame modes, FIFO, interrupts, and timeout |
| `usart-dma` | USART loopback transfers through DMA |
| `sim-card-dma` | Complete hardware T=0 SELECT through DMA, including data, T0END, and SW1/SW2 |
| `sim-card-timers` | Hardware T=0 character timeout; BWT is skipped without a T=1 card |
| `dif-v2` | DIFv2 IRQ loopback, serial modes, FIFO alignment and bursts, BSCONF, LCD reads, and conversion |
| `dif-v2-dma` | DIFv2 DMA flows, FIFO alignment and bursts, BSCONF, parallel LCD reads, and recovery |
| `dif-v2-lcd` | EL71 JBT6K71 command/data I/O, polling/TPS BSCONF modes, GRAM, and conversion gating |
| `cfi-intel` | Safe Intel/ST CFI discovery, identification, geometry, and read commands |
| `cfi-intel-rw` | Intel/ST CFI program, erase, lock, and suspend/resume operations |
| `cfi-intel-efa-rw` | Intel/ST extended flash array program and erase operations |
| `cfi-intel-otp-rw` | Irreversible Intel/ST OTP program and freeze operations |

`cfi-intel-buffer-abort` uses a detected blank, non-lock-down erase block and the geometry reported by that flash. It
unlocks the block for the test, verifies the affected write buffer, cross-block word, and both full-block CRCs, then
restores the original lock state. It skips destructive access if no suitable block or Intel/ST buffered-program command
set is found.
