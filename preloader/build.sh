#!/bin/sh
set -eu

cd "$(dirname "$0")"

for board in siemens-pmb8875 siemens-pmb8876 apoxi-pmb8875 apoxi-pmb8876; do
	cmake -S . -B "build/$board" -DBOARD="$board"
	cmake --build "build/$board" --parallel
	install -m 644 "build/$board/preloader.bin" "../boot/preloader-$board.bin"
done
