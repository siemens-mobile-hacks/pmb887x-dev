#!/bin/bash
cd $(dirname $0)
./gen_headers_qemu.pl > ../../qemu/hw/arm/pmb887x/gen/cpu_regs.h
./gen_decoder.pl > ../../qemu/hw/arm/pmb887x/gen/cpu_meta.c
./gen_qemu_flashes.pl > ../../qemu/hw/arm/pmb887x/gen/flash-info.c
