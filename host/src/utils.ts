export function formatHex(value: number, digits = 2) {
	return value.toString(16).toUpperCase().padStart(digits, "0");
}

export function parseInteger(text: string) {
	return /^(0x[0-9a-f]+|0o[0-7]+|0b[01]+|[0-9]+)$/i.test(text) ? Number(text) : undefined;
}
