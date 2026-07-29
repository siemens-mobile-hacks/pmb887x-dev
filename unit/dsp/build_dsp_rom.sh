#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if [[ -n "${MAKEDSP1:-}" ]]; then
	makedsp1="$MAKEDSP1"
else
	makedsp1=~/build/teakra/build/src/makedsp1/makedsp1
fi

temp_dir="$(mktemp -d)"
trap 'rm -rf -- "$temp_dir"' EXIT

build_image() {
	local name="$1"
	local symbol="$2"
	local dsp1_file="$temp_dir/$name.dsp1"
	local inc_file="$temp_dir/$name.inc"

	"$makedsp1" "$script_dir/$name.asm" "$dsp1_file"
	xxd -i -c 16 -n "$symbol" "$dsp1_file" |
		sed 's/^unsigned char/static const uint8_t/; /_len =/d' > "$inc_file"
	chmod 0644 "$inc_file"
	mv -- "$inc_file" "$script_dir/$name.inc"
}

build_image commands-0602 DSP_TEST_IMAGE_0602
build_image commands-0604 DSP_TEST_IMAGE_0604
build_image commands-0801 DSP_TEST_IMAGE_0801
build_image irqs-0602 DSP_IRQ_IMAGE_0602
build_image irqs-0604 DSP_IRQ_IMAGE_0604
build_image irqs-0801 DSP_IRQ_IMAGE_0801
