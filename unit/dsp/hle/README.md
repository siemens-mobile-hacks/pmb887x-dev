# DSP HLE tests

These tests exercise a complete original startup User ROM through its
MCU-visible ABI without loading purpose-built DSP code. The User ROM is selected
by the factory Mask ROM ID and the board's Siemens or APOXI software platform.

Runtime fixtures come from `rom/dsp/`. Tests use generic CPU boards unless
physical RF or another board component is part of the contract.

`dsp-hle-runtime-transport` checks the three runtime pipes, interrupt gating,
simultaneous commands, ACK values, result visibility and rejected command IDs.
