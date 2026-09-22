#/bin/bash
set -e
set -x
cmake -B build -DBOARD=$BOARD
cmake --build build
exec ../../boot.py build/app.bin "$@"
#sie-tool boot -i build/app.bin --follow $@
