# DSP HLE tests

These tests exercise the factory Mask ROM or a complete extracted startup
runtime through its MCU-visible ABI without loading purpose-built DSP code.

Runtime fixtures come from `rom/dsp/`. Tests use generic CPU boards unless
physical RF or another board component is part of the contract.

`dsp-hle-boot-timing` checks Mask ROM boot-command ownership and records reset and
copy latency without inventing limits before hardware data exists.
`dsp-hle-runtime-transport` checks the three runtime pipes, interrupt gating,
simultaneous commands, ACK values, result visibility and rejected command IDs.
