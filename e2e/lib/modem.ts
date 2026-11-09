import { setTimeout as delay } from "node:timers/promises";

import { AsyncSerialPort, AtChannel } from "@sie-js/serial";

const BAUD_RATE = 115200;
const AT_READY_TIMEOUT_MS = 120_000;
const AT_PROBE_TIMEOUT_MS = 1000;
const SIM_READY_TIMEOUT_MS = 60_000;
const SIM_POLL_INTERVAL_MS = 1000;

export async function withModem(task: (at: AtChannel) => Promise<void>) {
	const serialPath = process.argv[2];
	if (serialPath === undefined)
		throw new Error("usage: TEST SERIAL");

	const port = new AsyncSerialPort({ path: serialPath, baudRate: BAUD_RATE });
	await port.open();
	const at = new AtChannel(port);
	at.start();
	try {
		await waitForAt(at);
		const verboseErrors = await at.sendCommandNoResponse("AT+CMEE=1");
		if (!verboseErrors.success)
			throw new Error(`AT+CMEE=1: ${verboseErrors.status}`);
		await task(at);
	} finally {
		at.stop();
		await port.close();
	}
}

export async function waitForSimReady(at: AtChannel) {
	const deadline = Date.now() + SIM_READY_TIMEOUT_MS;
	let state = "";
	while (Date.now() < deadline) {
		const response = await at.sendCommand("AT+CPIN?", "+CPIN:");
		state = response.lines[0] ?? response.status;
		if (state == "+CPIN: READY")
			return;
		await delay(SIM_POLL_INTERVAL_MS);
	}
	throw new Error(`SIM is not ready within ${SIM_READY_TIMEOUT_MS / 1000} s, last AT+CPIN?: ${state}`);
}

async function waitForAt(at: AtChannel) {
	const deadline = Date.now() + AT_READY_TIMEOUT_MS;
	while (Date.now() < deadline) {
		if ((await at.sendCommandNoResponse("ATQ0 V1 E0", AT_PROBE_TIMEOUT_MS)).success)
			return;
	}
	throw new Error(`no AT response within ${AT_READY_TIMEOUT_MS / 1000} s`);
}
