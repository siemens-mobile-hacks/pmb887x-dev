import { AsyncSerialPort, DWD } from "@sie-js/serial";
import { SerialPort } from "serialport-rs";

const DWD_PHONES = new Map([
	["04DA:21XX", "Panasonic"],
	["1004:6000", "LG"],
]);

export async function findDwdPhonePort() {
	for (const port of await SerialPort.list()) {
		const usbId = `${port.vendorId ?? ""}:${port.productId ?? ""}`.toUpperCase();
		for (const [pattern, name] of DWD_PHONES) {
			if (matchesUsbId(pattern, usbId))
				return { path: port.path, description: `${port.path} (${usbId}, ${name})` };
		}
	}
	return undefined;
}

export async function resetPhoneViaDwd(path: string) {
	const port = new AsyncSerialPort({ path, baudRate: 115200, hupcl: false });
	await port.open();
	try {
		const dwd = new DWD(port);
		await dwd.connect();
		await dwd.reset();
	} finally {
		await port.close();
	}
}

function matchesUsbId(pattern: string, usbId: string) {
	return pattern.length == usbId.length && [...pattern].every((char, i) => char == "X" || char == usbId[i]);
}
