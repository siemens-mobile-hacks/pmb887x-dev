import { fetch } from "undici";

import { detachGprs, registerAndAttachGprs, withPpp } from "#lib/gprs.ts";
import { withModem } from "#lib/modem.ts";
import { createPppDispatcher } from "#lib/ppp.ts";

const APN = "internet";
const INTERNET_ADDRESS = "1.1.1.1";
const PING_TIMEOUT_MS = 10_000;
const HTTP_URL = "http://example.org/";
const HTTP_TIMEOUT_MS = 60_000;

await withModem(async (at) => {
	await registerAndAttachGprs(at);
	await withPpp(at, APN, async (ppp, connection) => {
		const reply = await ppp.ping(INTERNET_ADDRESS, PING_TIMEOUT_MS);
		console.log(`ping ${reply.address}: ${reply.bytes} bytes, ${reply.time} ms, TTL ${reply.ttl}`);

		if (connection.primaryDns === undefined)
			throw new Error("PPP: no DNS server");
		const dispatcher = createPppDispatcher(ppp, connection.primaryDns);
		try {
			const response = await fetch(HTTP_URL, { dispatcher, signal: AbortSignal.timeout(HTTP_TIMEOUT_MS) });
			const body = await response.text();
			if (!response.ok)
				throw new Error(`GET ${HTTP_URL}: HTTP ${response.status}`);
			console.log(`GET ${HTTP_URL}: HTTP ${response.status}, ${body.length} bytes`);
		} finally {
			await dispatcher.close();
		}
	});
	await detachGprs(at);
});
