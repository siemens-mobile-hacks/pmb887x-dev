#!/bin/bash
cd $(dirname $0)
./gen_headers_qemu.pl > ../../qemu/hw/arm/pmb887x/regs.h
./gen_decoder.pl > ../../qemu/hw/arm/pmb887x/regs_info.c

