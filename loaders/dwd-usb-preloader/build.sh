#!/bin/sh
set -eu

cd "$(dirname "$0")"

for board in apoxi-pmb8876; do
	cmake -S . -B "build/$board" -DBOARD="$board"
	cmake --build "build/$board" --parallel
	install -v -m 644 "build/$board/dwd-usb-preloader.bin" "../../boot/dwd-usb-preloader-$board.bin"
done
