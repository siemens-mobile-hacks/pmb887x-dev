#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if (($# != 0)); then
	echo "usage: $0" >&2
	exit 2
fi

perl "$script_dir/opcode-aliases/build_instructions.pl"
exec perl "$script_dir/instructions/pack_instructions.pl" "$script_dir/opcode-aliases"
