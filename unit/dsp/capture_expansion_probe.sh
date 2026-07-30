#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
unit_dir=$(cd -- "$script_dir/.." && pwd)
family_first=${1:-0}
family_last=${2:-18}
board=${BOARD:-siemens-el71}
baudrate=${DSP_BAUDRATE:-1600000}
jobs=${DSP_JOBS:-$(nproc)}

"$script_dir/build_opcode_probe.sh"

for ((family = family_first; family <= family_last; family++)); do
	cmake -S "$unit_dir" -B "$unit_dir/build" \
		-DDSP_EXPANSION_PROBE_FAMILY_FIRST="$family" \
		-DDSP_EXPANSION_PROBE_FAMILY_LAST="$family" \
		-DDSP_EXPANSION_PROBE_WORD_FIRST=0x0000 \
		-DDSP_EXPANSION_PROBE_WORD_LAST=0xFFFF
	cmake --build "$unit_dir/build" --target dsp-expansion-probe -j"$jobs"

	for capture in 1 2; do
		(
			cd "$unit_dir"
			BOARD="$board" TEST_COLOR=OFF ./run.sh dsp-expansion-probe --speed="$baudrate"
		) | tee "/tmp/dsp-capture-$capture.log"
	done

	printf -v family_name '%02u' "$family"
	perl "$script_dir/opcode-probe/merge_expansion_probe.pl" \
		--output "$script_dir/opcode-probe/expansion-family-$family_name-map.csv" \
		/tmp/dsp-capture-1.log /tmp/dsp-capture-2.log
	perl "$script_dir/opcode-probe/pack_maps.pl" --remove-source
done
