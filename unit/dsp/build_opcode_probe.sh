#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
for cpu in pmb8875 pmb8876; do
	suffix=${cpu#pmb}
	perl "$script_dir/lle/opcode-probe/build_probe.pl" probe.asm "probe-image-$suffix.inc" \
		"DSP_OPCODE_PROBE_IMAGE_$suffix" raw "$cpu"
	perl "$script_dir/lle/opcode-probe/build_probe.pl" probe.asm "expansion-probe-image-$suffix.inc" \
		"DSP_EXPANSION_PROBE_IMAGE_$suffix" expansion "$cpu"
	perl "$script_dir/lle/opcode-probe/build_probe.pl" program-backing.asm "program-backing-image-$suffix.inc" \
		"DSP_PROGRAM_BACKING_IMAGE_$suffix" raw "$cpu"
done
