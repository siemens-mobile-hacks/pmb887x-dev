@echo off
setlocal
set "BSP_DIR=%~dp0"
if not exist "%BSP_DIR%node_modules\.modules.yaml" (
	call pnpm --dir "%BSP_DIR%." install --frozen-lockfile --prefer-offline 1>&2 || exit /b 1
)
node "%BSP_DIR%host\boot.js" %*
