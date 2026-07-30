#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
if (($# != 0)); then
	echo "usage: $0" >&2
	exit 2
fi

perl "$script_dir/instructions/build_instructions.pl"
perl "$script_dir/instructions/pack_instructions.pl"
