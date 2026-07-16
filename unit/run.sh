#!/bin/bash
set -e
set -x

test_name="$1"
shift

cmake_args=(-B build -DBOOT=extram -DTEST_COLOR="${TEST_COLOR:-ON}")
if [[ -n "${BOARD:-}" ]]; then
	cmake_args+=(-DBOARD="$BOARD")
fi

cmake "${cmake_args[@]}"
cmake --build build --target "$test_name"

chaos_args=(--exec="build/$test_name.bin" --speed=115200)
perl ../chaos-boot.pl "${chaos_args[@]}" "$@"
