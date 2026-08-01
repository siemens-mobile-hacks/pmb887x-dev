#!/bin/bash
set -e
set -x

test_name="$1"
shift
board=${BOARD:-siemens-el71}
build_dir="build/$board"

cmake_args=(-B "$build_dir" -DBOARD="$board" -DBOOT=extram -DTEST_COLOR="${TEST_COLOR:-ON}")

cmake "${cmake_args[@]}"
cmake --build "$build_dir" --target "$test_name"

chaos_args=(--exec="$build_dir/$test_name.bin" --speed=115200)
perl ../chaos-boot.pl "${chaos_args[@]}" "$@"
