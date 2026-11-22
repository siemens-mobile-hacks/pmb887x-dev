#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
perl ../../chaos-boot.pl --exec=build/app.bin $@
