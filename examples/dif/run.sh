#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
sie-tool boot -i build/app.bin --follow $@
