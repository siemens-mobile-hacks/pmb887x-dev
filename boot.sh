#!/bin/bash
set -e

bsp_dir="$(cd "$(dirname "$0")" && pwd)"
marker="$bsp_dir/node_modules/.modules.yaml"
if [[ ! -f $marker || $bsp_dir/pnpm-lock.yaml -nt $marker ]]; then
	pnpm --dir "$bsp_dir" install --frozen-lockfile --prefer-offline >&2
	touch "$marker"
fi
exec node "$bsp_dir/host/boot.js" "$@"
