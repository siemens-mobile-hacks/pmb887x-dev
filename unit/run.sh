#!/bin/bash
set -e
set -x

cmake -B build -DBOARD="$BOARD" -DBOOT=extram -DTEST_COLOR="${TEST_COLOR:-ON}"
cmake --build build --target "$1"
perl ../chaos-boot.pl --exec="build/$1.bin" "${@:2}"
