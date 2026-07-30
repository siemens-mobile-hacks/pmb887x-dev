#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
perl "$script_dir/opcode-probe/build_probe.pl" probe.asm probe-image.inc DSP_OPCODE_PROBE_IMAGE raw
perl "$script_dir/opcode-probe/build_probe.pl" probe.asm expansion-probe-image.inc DSP_EXPANSION_PROBE_IMAGE expansion
exec perl "$script_dir/opcode-probe/build_probe.pl" program-backing.asm program-backing-image.inc DSP_PROGRAM_BACKING_IMAGE
