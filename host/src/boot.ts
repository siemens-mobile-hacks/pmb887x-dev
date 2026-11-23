import { readFileSync } from "node:fs";
import { setTimeout as delay } from "node:timers/promises";

import { findDwdPhonePort, resetPhoneViaDwd } from "./dwd.ts";
import { decodeHeader, type Platform } from "./header.ts";
import { forwardOutput } from "./hostCommands.ts";
import { Preloader } from "./Preloader.ts";
import { DEFAULT_FORMAT, isUsbDeviceConnected, openPort } from "./serial.ts";
import { formatHex } from "./utils.ts";

const USB_WAIT_INTERVAL_MS = 100;
const DWD_POLL_INTERVAL_MS = 500;

export type BootOptions = {
	payloadPath: string;
	devicePath: string;
	platform: Platform | undefined;
	speed: number | undefined;
	writeAddress: number | undefined;
	execAddress: number | undefined;
	resetDwdPhones: boolean;
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
		await waitForUsbDevice(options.devicePath, options.resetDwdPhones);
	}

	const port = await openPort(options.devicePath, 115200, DEFAULT_FORMAT);
	try {
		await port.flush();

		console.error("Waiting for BootROM...");
		const preloader = new Preloader(port, layout.platform);
		await preloader.connect();
		console.error(`Connected via ${preloader.getBootLoader()}${preloader.isSecureBoot() ? " (secure boot)" : ""}`);
		console.error(`CPU detected: ${preloader.getDetectedCPU().toUpperCase()}`);

		if (layout.cpu !== undefined && preloader.getDetectedCPU() != "pmb887x" && preloader.getDetectedCPU() != layout.cpu)
			throw new Error(`Payload CPU ${layout.cpu} does not match the ${preloader.getDetectedCPU()}!`);

		if (options.speed !== undefined) {
			await preloader.setSpeed(options.speed);
			await preloader.ping();
		}

		console.error("Preloader ready");

		const board = layout.board === undefined ? "" : ` [${layout.board}]`;
		console.error(`Loading payload: ${options.payloadPath}${board} -> 0x${writeAddress} (${payload.length} bytes)`);
		await preloader.writePayload(address, payload);
		await preloader.execute(layout.execAddress);

		console.error("<------- cut here -------");
		await forwardOutput(port, options.devicePath);
	} finally {
		await port.close();
	}
}

async function waitForUsbDevice(path: string, resetDwdPhones: boolean) {
	let resetPath: string | undefined;
	let nextDwdPoll = 0;
	while (!(await isUsbDeviceConnected(path))) {
		if (resetDwdPhones && performance.now() >= nextDwdPoll) {
			resetPath = await resetNewDwdPhone(resetPath);
			nextDwdPoll = performance.now() + DWD_POLL_INTERVAL_MS;
		}
		await delay(USB_WAIT_INTERVAL_MS);
	}
}

async function resetNewDwdPhone(resetPath: string | undefined) {
	const phone = await findDwdPhonePort();
	if (phone === undefined || phone.path == resetPath)
		return phone?.path;

	console.error(`Resetting phone via DWD: ${phone.description}`);
	try {
		await resetPhoneViaDwd(phone.path);
		return phone.path;
	} catch (error) {
		console.error(`DWD reset failed: ${error instanceof Error ? error.message : String(error)}`);
		return undefined;
	}
}
