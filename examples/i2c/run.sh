#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
perl ../../boot.pl --boot=build/app.bin $@
