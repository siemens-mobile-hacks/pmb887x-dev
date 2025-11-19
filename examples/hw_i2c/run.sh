#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
sie-tool boot --follow -i build/app.bin "$@"
