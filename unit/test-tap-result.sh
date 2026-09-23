#!/bin/bash
set -e

cd "$(dirname "$0")"

check() {
	expected=$1
	output=$2
	status=0
	printf '%b' "$output" | awk -f tap-result.awk >/dev/null 2>&1 || status=$?
	if [[ "$status" != "$expected" ]]; then
		printf 'expected status %s, got %s\n' "$expected" "$status" >&2
		exit 1
	fi
}

check 0 'TAP version 13\nok 1 - x\n1..1\n# result: PASS (0 failed)\n'
check 0 'TAP version 13\n# result: \033[32mPASS\033[0m (0 failed)\n'
check 1 'TAP version 13\nnot ok 1 - x\n1..1\n# result: FAIL (1 failed)\n'
check 1 'TAP version 13\nok 1 - x\n'
check 1 'TAP version 13\n# result: PASS (0 failed)\nextra output\n'
