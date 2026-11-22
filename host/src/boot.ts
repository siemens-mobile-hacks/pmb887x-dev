import { readFileSync } from "node:fs";

import { decodeHeader, type Platform } from "./header.ts";
import { forwardOutput } from "./hostCommands.ts";
import { Preloader } from "./Preloader.ts";
import { DEFAULT_FORMAT, openPort, waitForUsbDevice } from "./serial.ts";
import { formatHex } from "./utils.ts";

const BOOT_SPEED = 115200;

export type BootOptions = {
	payloadPath: string;
	devicePath: string;
	platform: Platform | undefined;
	speed: number | undefined;
	writeAddress: number | undefined;
	execAddress: number | undefined;
};

function resolvePayloadLayout(options: BootOptions, payload: Buffer) {
	const header = decodeHeader(payload);
	if (header === undefined) {
		if (options.platform === undefined)
			throw new Error("payload has no PMB887X header; specify --platform");
		if (options.writeAddress === undefined || options.execAddress === undefined)
			throw new Error("payload has no PMB887X header; specify --write-addr and --exec-addr");
		return {
			cpu: undefined,
			platform: options.platform,
			board: undefined,
			writeAddress: options.writeAddress,
			execAddress: options.execAddress,
		};
	}

	if (options.platform !== undefined && options.platform != header.platform)
		throw new Error("--platform does not match the PMB887X header");
	return {
		cpu: header.cpu,
		platform: header.platform,
		board: header.board,
		writeAddress: options.writeAddress ?? header.loadAddress,
		execAddress: options.execAddress ?? header.loadAddress,
	};
}

export async function boot(options: BootOptions) {
	const payload = readFileSync(options.payloadPath);
	if (payload.length == 0)
		throw new Error("payload is empty");

	const layout = resolvePayloadLayout(options, payload);
	const address = layout.writeAddress;
	const writeAddress = formatHex(address, 8);

	if (options.devicePath.startsWith("webusb:")) {
		console.error(`Waiting for USB device ${options.devicePath}...`);
		await waitForUsbDevice(options.devicePath);
	}
	const port = await openPort(options.devicePath, BOOT_SPEED, DEFAULT_FORMAT);
	try {
		await port.flush();
		console.error("Waiting for BootROM...");
		const preloader = await Preloader.load(port, layout.platform);
		console.error(`CPU detected: ${preloader.cpu.toUpperCase()}${preloader.secureBoot ? " (secure boot)" : ""}`);
		if (layout.cpu !== undefined && preloader.cpu != layout.cpu)
			throw new Error("payload CPU does not match the connected phone");
		if (options.speed !== undefined)
			await preloader.setSpeed(options.speed);
		await preloader.ping();
		console.error("Preloader ready");

		const board = layout.board === undefined ? "" : ` [${layout.board}]`;
		console.error(`Loading payload: ${options.payloadPath}${board} -> 0x${writeAddress} (${payload.length} bytes)`);
		await preloader.writePayload(address, payload);
		console.error("<------- cut here -------");
		await preloader.goto(layout.execAddress);
		await forwardOutput(port, options.devicePath);
	} finally {
		await port.close();
	}
}
