#/bin/bash
set -e
set -x
[[ -d build ]] || cmake -B build
cmake --build build
perl ../../chaos-boot.pl --exec=build/app.bin $@
