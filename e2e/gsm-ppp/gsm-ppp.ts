import { setTimeout as delay } from "node:timers/promises";

import type { AtChannel } from "@sie-js/serial";

import { waitForSimReady, withModem } from "#lib/modem.ts";

const REGISTRATION_TIMEOUT_MS = 180_000;
const ATTACH_TIMEOUT_MS = 120_000;
const DIAL_TIMEOUT_MS = 120_000;
const PPP_CONNECT_TIMEOUT_MS = 60_000;
const PING_TIMEOUT_MS = 10_000;
const PDP_CONTEXT_ID = 1;
const APN = "internet";
const GGSN_ADDRESS = "172.16.32.1";
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

async function getRegistrationState(at: AtChannel, query: string, prefix: string) {
	const response = await at.sendCommand(query, prefix);
	const match = response.lines[0]?.match(/^\S+ \d+,(\d+)/);
	if (!response.success || match?.[1] === undefined)
		throw new Error(`${query}: ${response.lines[0] ?? response.status}`);
	return { state: Number(match[1]), line: response.lines[0] };
}

async function waitForRegistration(at: AtChannel, query: string, prefix: string, name: string, timeout: number) {
	const deadline = Date.now() + timeout;
	let lastState: number | undefined;
	while (Date.now() < deadline) {
		const { state, line } = await getRegistrationState(at, query, prefix);
		if (state !== lastState) {
			console.log(`${name}: ${REGISTRATION_STATES.get(state) ?? "invalid"} (${line})`);
			lastState = state;
		}
		if (REGISTERED_STATES.has(state))
			return;
		if (state == REGISTRATION_DENIED)
			throw new Error(`network denied ${name}`);
		await delay(REGISTRATION_POLL_INTERVAL_MS);
	}
	throw new Error(`no ${name} within ${timeout / 1000} s`);
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

async function attachGprs(at: AtChannel) {
	const attach = await at.sendCommandNoResponse("AT+CGATT=1", ATTACH_TIMEOUT_MS);
	if (!attach.success)
		throw new Error(`AT+CGATT=1: ${attach.status}`);
	await waitForRegistration(at, "AT+CGREG?", "+CGREG:", "GPRS attach", ATTACH_TIMEOUT_MS);
}

async function definePdpContext(at: AtChannel) {
	const command = `AT+CGDCONT=${PDP_CONTEXT_ID},"IP","${APN}"`;
	const response = await at.sendCommandNoResponse(command);
	if (!response.success)
		throw new Error(`${command}: ${response.status}`);
}

async function pingOverPpp(at: AtChannel) {
	const ppp = await at.connectPPP(`ATD*99***${PDP_CONTEXT_ID}#`, DIAL_TIMEOUT_MS);
	try {
		const connection = await ppp.connect({ timeout: PPP_CONNECT_TIMEOUT_MS });
		console.log(`PPP: address ${connection.localAddress}`);

		const reply = await ppp.ping(GGSN_ADDRESS, PING_TIMEOUT_MS);
		console.log(`ping ${reply.address}: ${reply.bytes} bytes, ${reply.time} ms, TTL ${reply.ttl}`);
	} finally {
		await ppp.disconnect();
		await at.exitDataMode();
	}
}

async function detachGprs(at: AtChannel) {
	const response = await at.sendCommandNoResponse("AT+CGATT=0", ATTACH_TIMEOUT_MS);
	if (!response.success)
		throw new Error(`AT+CGATT=0: ${response.status}`);
}

await withModem(async (at) => {
	await waitForSimReady(at);
	await waitForRegistration(at, "AT+CREG?", "+CREG:", "registration", REGISTRATION_TIMEOUT_MS);

	const operator = await at.sendCommand("AT+COPS?", "+COPS:");
	if (!operator.success)
		throw new Error(`AT+COPS?: ${operator.status}`);
	console.log(`operator: ${operator.lines[0]}`);

	await checkSignal(at);
	await attachGprs(at);
	await definePdpContext(at);
	await pingOverPpp(at);
	await detachGprs(at);
});
