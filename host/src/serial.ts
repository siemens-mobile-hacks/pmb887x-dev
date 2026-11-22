import { setTimeout as delay } from "node:timers/promises";

import { AsyncSerialPort } from "@sie-js/serial";
import { SocketBinding } from "serialport-bindings-socket";
import { WebUSBBinding } from "serialport-bindings-webusb";
import { SerialPort } from "serialport-rs";

const USB_WAIT_INTERVAL_MS = 10;
const START_BITS = 1;
const STOP_BITS = 1;
const IDLE_SIGNALS = { dtr: false, rts: false, brk: false };

const SERIAL_ADAPTERS = new Map([
	["1A86:5523", "QinHeng CH341"],
	["1A86:7522", "QinHeng CH340"],
	["1A86:7523", "QinHeng CH340"],
	["1A86:7584", "QinHeng CH340S"],
	["0403:6001", "FTDI FT232"],
	["0403:6010", "FTDI FT2232"],
	["0403:6011", "FTDI FT4232"],
	["0403:6014", "FTDI FT232H"],
	["0403:6015", "FTDI bridge"],
	["10C4:EA60", "Silicon Labs CP210x"],
	["10C4:EA61", "Silicon Labs CP210x"],
	["10C4:EA63", "Silicon Labs CP210x"],
	["10C4:EA70", "Silicon Labs CP2105"],
	["10C4:EA71", "Silicon Labs CP2108"],
	["067B:2303", "Prolific PL2303"],
]);

export type SerialFormat = {
	dataBits: 7 | 8;
	parity: "none" | "even" | "odd";
};

export const DEFAULT_FORMAT: SerialFormat = { dataBits: 8, parity: "none" };

export async function findSerialAdapter() {
	const adapters = [];
	for (const port of await SerialPort.list()) {
		const usbId = `${port.vendorId ?? ""}:${port.productId ?? ""}`.toUpperCase();
		const name = SERIAL_ADAPTERS.get(usbId);
		if (name !== undefined)
			adapters.push({ path: port.path, description: `${port.path} (${usbId}, ${name})` });
	}

	const [adapter, ...others] = adapters;
	if (adapter === undefined)
		throw new Error("no UART adapter found; specify --device");
	if (others.length != 0) {
		const lines = adapters.map(({ description }) => `\t${description}`);
		throw new Error(["multiple UART adapters found; specify --device:", ...lines].join("\n"));
	}
	return adapter;
}

export async function waitForUsbDevice(path: string) {
	const selector = [...new URL(path).searchParams];
	const matches = (portPath: string) => {
		const params = new URL(portPath).searchParams;
		return selector.every(([name, value]) => params.get(name)?.toLowerCase() == value.toLowerCase());
	};

	while (!(await WebUSBBinding.list()).some((port) => matches(port.path)))
		await delay(USB_WAIT_INTERVAL_MS);
}

export async function openPort(path: string, baudRate: number, format: SerialFormat) {
	const options = { path, baudRate, ...format };
	let port;
	if (/^(tcp|unix)[a-z-]*:/.test(path)) {
		port = new AsyncSerialPort({ ...options, binding: SocketBinding });
	} else if (path.startsWith("webusb:")) {
		port = new AsyncSerialPort({ ...options, binding: WebUSBBinding });
	} else {
		port = new AsyncSerialPort({ ...options, hupcl: false });
	}
	await port.open();
	await port.setSignals(IDLE_SIGNALS);
	return port;
}

export async function transmit(port: AsyncSerialPort, format: SerialFormat, data: Buffer) {
	const started = performance.now();
	await port.write(data);
	await port.drain();

	const parityBits = format.parity == "none" ? 0 : 1;
	const frameBits = START_BITS + format.dataBits + parityBits + STOP_BITS;
	const remainingMs = data.length * frameBits * 1000 / port.baudRate - (performance.now() - started);
	if (remainingMs > 0)
		await delay(remainingMs);
}

export async function sendBreak(port: AsyncSerialPort, durationMs: number) {
	await port.setSignals({ ...IDLE_SIGNALS, brk: true });
	await delay(durationMs);
	await port.setSignals(IDLE_SIGNALS);
}
