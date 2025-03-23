#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
export PERL5LIB="$PERL5LIB:."
perl ../boot.pl --boot=build/app.bin --module=io_bridge.pm $@
