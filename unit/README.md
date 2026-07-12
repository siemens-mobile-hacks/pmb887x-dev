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
| `mod` | Common CLC, SRC, and SRB register blocks |
| `adc` | ADC registers, conversion, interrupts, and battery voltage measurement |
| `rtc` | RTC registers, synchronous/asynchronous operation, timer chain, and interrupt sub-node |
| `vic` | VIC pending state, arbitration, priority masking, acknowledge, IRQ, and FIQ routing |
| `gptu` | Both GPTU instances, T0/T1 concatenation, T2 split mode, reload, and service requests |
| `tpu` | TDMA counter clock, modulo overflow, compare interrupts, correction, offset, and frame skip |
| `tpu-ram` | TPU RF/Timer RAM partitioning, widths, timing events, and address pointers |
| `tpu-rf-ssc` | TPU RF SSC direct transfers, formats, clocks, strobe selection, and completion interrupt |
| `tcm` | ITCM and DTCM region registers, memory access, remap, and overlay behavior |
| `i2c-v1` | I2Cv1 registers, hardware bits, IRQ transfers, and PMIC SMBus reads |
| `i2c-v2` | I2Cv2 registers, IRQ transfers, SMBus, FIFO modes, and bus scan |
| `i2c-v2-dma` | I2Cv2 SMBus transfers through DMA |
| `ssc` | SSC loopback, serial formats, FIFO modes, interrupts, and errors |
| `ssc-dma` | SSC full-duplex FIFO DMA, bursts, LLI, widths, statuses, and interrupts |
| `dif-v1` | DIFv1 loopback, FIFO modes, interrupts, errors, and LCD bit conversion registers |
| `dif-v1-dma` | DIFv1 full-duplex FIFO DMA, bursts, LLI, widths, statuses, and interrupts |
| `usart` | USART loopback, frame modes, FIFO, interrupts, and timeout |
| `usart-dma` | USART loopback transfers through DMA |
| `dif-v2` | DIFv2 IRQ loopback, serial modes, FIFO, and data conversion |
| `dif-v2-dma` | DIFv2 loopback transfers through DMA |
| `cfi-intel` | Safe Intel/ST CFI discovery, identification, geometry, and read commands |
| `cfi-intel-rw` | Intel/ST CFI program, erase, lock, and suspend/resume operations |
| `cfi-intel-efa-rw` | Intel/ST extended flash array program and erase operations |
| `cfi-intel-otp-rw` | Irreversible Intel/ST OTP program and freeze operations |
