#/bin/bash
set -e
set -x
[[ -d build ]] || cmake -B build
cmake --build build
perl ../../boot.pl --boot=build/app.bin --picocom $@
