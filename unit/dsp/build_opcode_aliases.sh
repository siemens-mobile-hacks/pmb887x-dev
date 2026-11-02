#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if (($# != 0)); then
	echo "usage: $0" >&2
	exit 2
fi

for corpus in opcode-aliases opcode-reserved-modify delay-slot-overflow; do
	perl "$script_dir/lle/opcode-aliases/build_instructions.pl" "$corpus"
	perl "$script_dir/lle/instructions/pack_instructions.pl" "$script_dir/lle/$corpus"
done
