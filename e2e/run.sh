#!/bin/bash
set -e
set -o pipefail

cd "$(dirname "$0")"

test_name="$1"
serial="$2"

if [[ -f $test_name/$test_name.ts ]]; then
	marker=../node_modules/.modules.yaml
	if [[ ! -f $marker || ../pnpm-lock.yaml -nt $marker ]]; then
		pnpm --dir .. install --frozen-lockfile --prefer-offline
	fi
	command=(node_modules/.bin/tsx "$test_name/$test_name.ts")
elif [[ -f $test_name/$test_name.py ]]; then
	command=(python3 -u "$test_name/$test_name.py")
else
	echo "usage: run.sh NAME [SERIAL]" >&2
	exit 2
fi

if [[ -z $serial ]]; then
	serial=$(python3 -c 'import sys; sys.path.insert(0, ".."); import boot; print(boot.autodetect_device())')
fi

status=0
"${command[@]}" "$serial" 2>&1 | sed -u 's/^/# /' || status=$?
if [[ $status -eq 0 ]]; then
	printf 'ok 1 - %s\n1..1\n# result: PASS (0 failed)\n' "$test_name"
else
	printf 'not ok 1 - %s\n1..1\n# result: FAIL (1 failed)\n' "$test_name"
fi
exit $status
