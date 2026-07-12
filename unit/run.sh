#!/bin/bash
set -e
set -x

cmake -B build -DBOARD="$BOARD" -DBOOT=extram
cmake --build build --target "$1"
perl ../chaos-boot.pl --exec="build/$1.bin" "${@:2}"
