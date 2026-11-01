#!/bin/bash
set -e
set -x
cd "$(dirname "$0")"
cmake -B build -DBOARD="${BOARD:-siemens-el71}"
cmake --build build
../../boot.py build/app.bin "$@"
