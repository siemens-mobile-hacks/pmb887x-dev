#!/bin/bash
cd $(dirname $0)
./gen_headers_qemu.pl > ../../qemu/hw/arm/pmb887x/gen/cpu_regs.h
./gen_headers_qemu.pl --peripherals ../../qemu/hw/arm/pmb887x/gen/peripheral
./gen_decoder.pl --header > ../../qemu/hw/arm/pmb887x/gen/cpu_meta.h
./gen_decoder.pl > ../../qemu/hw/arm/pmb887x/gen/cpu_meta.c
./gen_qemu_flashes.pl > ../../qemu/hw/arm/pmb887x/gen/flash-info.c
./gen_dsp_headers.pl --qemu ../../qemu/hw/arm/pmb887x/gen
./gen_cpu_modules_qemu.pl > ../../qemu/hw/arm/pmb887x/gen/cpu_modules.c
./gen_dsp_rom_qemu.pl ../../qemu/hw/arm/pmb887x/gen
