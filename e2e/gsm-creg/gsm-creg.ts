import { setTimeout as delay } from "node:timers/promises";

import type { AtChannel } from "@sie-js/serial";

import { waitForSimReady, withModem } from "#lib/modem.ts";

const REGISTRATION_TIMEOUT_MS = 180_000;
const REGISTRATION_POLL_INTERVAL_MS = 1000;
const REGISTERED_STATES = new Set([1, 5]);
const REGISTRATION_DENIED = 3;
const CSQ_RSSI_UNKNOWN = 99;
const CSQ_RSSI_MIN_DBM = -113;
const CSQ_RSSI_STEP_DB = 2;

const REGISTRATION_STATES = new Map([
	[0, "not registered, not searching"],
	[1, "registered, home network"],
	[2, "not registered, searching"],
	[3, "registration denied"],
	[4, "unknown"],
	[5, "registered, roaming"],
]);

async function getRegistrationState(at: AtChannel) {
	const response = await at.sendCommand("AT+CREG?", "+CREG:");
	const match = response.lines[0]?.match(/^\+CREG: \d+,(\d+)/);
	if (!response.success || match?.[1] === undefined)
		throw new Error(`AT+CREG?: ${response.lines[0] ?? response.status}`);
	return { state: Number(match[1]), line: response.lines[0] };
}

async function waitForRegistration(at: AtChannel) {
	const deadline = Date.now() + REGISTRATION_TIMEOUT_MS;
	let lastState: number | undefined;
	while (Date.now() < deadline) {
		const { state, line } = await getRegistrationState(at);
		if (state !== lastState) {
			console.log(`registration: ${REGISTRATION_STATES.get(state) ?? "invalid"} (${line})`);
			lastState = state;
		}
		if (REGISTERED_STATES.has(state))
			return;
		if (state == REGISTRATION_DENIED)
			throw new Error("network denied registration");
		await delay(REGISTRATION_POLL_INTERVAL_MS);
	}
	throw new Error(`not registered within ${REGISTRATION_TIMEOUT_MS / 1000} s`);
}

async function checkSignal(at: AtChannel) {
	const response = await at.sendCommand("AT+CSQ", "+CSQ:");
	const match = response.lines[0]?.match(/^\+CSQ: (\d+),(\d+)/);
	if (!response.success || match?.[1] === undefined)
		throw new Error(`AT+CSQ: ${response.lines[0] ?? response.status}`);
	const rssi = Number(match[1]);
	if (rssi == CSQ_RSSI_UNKNOWN)
		throw new Error("registered, but AT+CSQ reports unknown signal level");
	console.log(`signal: ${CSQ_RSSI_MIN_DBM + CSQ_RSSI_STEP_DB * rssi} dBm (${response.lines[0]})`);
}

await withModem(async (at) => {
	await waitForSimReady(at);
	await waitForRegistration(at);

	const operator = await at.sendCommand("AT+COPS?", "+COPS:");
	if (!operator.success)
		throw new Error(`AT+COPS?: ${operator.status}`);
	console.log(`operator: ${operator.lines[0]}`);

	await checkSignal(at);
});
