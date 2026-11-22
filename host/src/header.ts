const HEADER_SIZE = 32;
const BOARD_NAME_SIZE = 20;
const BOOT_MAGIC_OFFSET = 0x3C;
const SIEMENS_BOOT_MARKER_OFFSET = 0x28;

export type Platform = "siemens" | "apoxi";

export const PLATFORMS: Platform[] = ["siemens", "apoxi"];

type Header = {
	cpu: string;
	platform: Platform;
	board: string;
	loadAddress: number;
};

function findHeaderOffset(data: Buffer) {
	if (data.toString("utf8", BOOT_MAGIC_OFFSET, BOOT_MAGIC_OFFSET + 4) == "CJKT")
		return 64;
	if (data.toString("utf8", SIEMENS_BOOT_MARKER_OFFSET, SIEMENS_BOOT_MARKER_OFFSET + 16) == "SIEMENS_BOOTCODE")
		return 96;
	return 4;
}

export function decodeHeader(data: Buffer): Header | undefined {
	const offset = findHeaderOffset(data);
	const magic = data.toString("utf8", offset, offset + 7);
	if (!/^PMB\d{4}$/.test(magic))
		return undefined;
	if (data.length < offset + HEADER_SIZE)
		throw new Error(`truncated PMB887X header at offset ${offset}`);

	const platformId = data.readUInt8(offset + 7);
	const platform = PLATFORMS[platformId];
	if (platform === undefined)
		throw new Error(`invalid PMB887X header platform: ${platformId}`);

	const name = data.subarray(offset + 12, offset + 12 + BOARD_NAME_SIZE);
	const nameEnd = name.indexOf(0);
	return {
		cpu: magic.toLowerCase(),
		platform,
		board: name.toString("utf8", 0, nameEnd < 0 ? name.length : nameEnd),
		loadAddress: data.readUInt32LE(offset + 8),
	};
}
