import type { PPP } from "@sie-js/serial";

import { detachGprs, registerAndAttachGprs, withPpp } from "#lib/gprs.ts";
import { withModem } from "#lib/modem.ts";
import { connectTcp } from "#lib/ppp.ts";
import { WSP_STATUS_OK, WspSession } from "#lib/wsp.ts";

const APN = "wap.vodafone.co.uk";
const HTTP_PORT = 80;
const HTTP_OK = 200;
const HTTP_TIMEOUT_MS = 60_000;
const PAGE_URL = "http://example.org/";
const MMS_CONTENT_TYPE = "application/vnd.wap.mms-message";
const MMS_RECIPIENT = "+9999";
const MMS_TRANSACTION_ID = "e2e";

const FIELD_CONTENT_TYPE = 0x84;
const FIELD_FROM = 0x89;
const FIELD_MESSAGE_ID = 0x8b;
const FIELD_MESSAGE_TYPE = 0x8c;
const FIELD_MMS_VERSION = 0x8d;
const FIELD_RESPONSE_STATUS = 0x92;
const FIELD_TO = 0x97;
const FIELD_TRANSACTION_ID = 0x98;
const MESSAGE_SEND_REQ = 0x80;
const MESSAGE_SEND_CONF = 0x81;
const MMS_VERSION_1_2 = 0x92;
const INSERT_ADDRESS_TOKEN = 0x81;
const CONTENT_TYPE_MULTIPART_MIXED = 0xa3;
const CONTENT_TYPE_MMS_MESSAGE = 0xbe;
const RESPONSE_STATUS_OK = 0x80;

const WAP_PROXIES = [
	{ operator: "Vodafone UK", proxy: "212.183.137.12:8799" },
	{ operator: "Vodafone DE", proxy: "139.7.29.1:80" },
];

const MMS_PROFILES = [
	{ operator: "Orange FR", mmsc: "http://mms.orange.fr", proxy: "192.168.10.200:8080" },
	{ operator: "Vodafone DE", mmsc: "http://139.7.24.1/servlets/mms", proxy: "139.7.29.17:80" },
	{ operator: "Tre SE", mmsc: "http://mms.tre.se", proxy: "mmsproxy.tre.se:8799" },
	{ operator: "Vodafone UK", mmsc: "http://mms.vodafone.co.uk/servlets/mms", proxy: undefined },
];

const WSP_GATEWAYS = [
	{ operator: "MobiFone", gateway: "203.162.21.114:9201" },
];

const WSP_MMS_PROFILES = [
	{ operator: "Telsim", mmsc: "http://mms:6001/MM1Servlet", gateway: "217.31.233.18:9201" },
];

type HttpRequest = {
	method: string;
	url: string;
	server: string;
	target: string;
	mmsPdu?: Buffer;
};

async function sendHttp(ppp: PPP, dnsServer: string, request: HttpRequest) {
	const url = new URL(request.url);
	const { host, port } = parseServer(request.server);
	const socket = await connectTcp(ppp, dnsServer, host, port);
	const headers = [`${request.method} ${request.target} HTTP/1.0`, `Host: ${url.host}`];
	if (request.mmsPdu !== undefined)
		headers.push(`Content-Type: ${MMS_CONTENT_TYPE}`, `Content-Length: ${request.mmsPdu.length}`);
	socket.write(`${headers.join("\r\n")}\r\n\r\n`);
	if (request.mmsPdu !== undefined)
		socket.write(request.mmsPdu);

	const response = await new Promise<Buffer>((resolve, reject) => {
		const chunks: Buffer[] = [];
		const timeout = new Error(`no response within ${HTTP_TIMEOUT_MS / 1000} s`);
		const timer = setTimeout(() => socket.destroy(timeout), HTTP_TIMEOUT_MS);
		socket.on("data", (chunk: Buffer) => chunks.push(chunk));
		socket.on("end", () => {
			clearTimeout(timer);
			socket.destroy();
			resolve(Buffer.concat(chunks));
		});
		socket.on("error", (error) => {
			clearTimeout(timer);
			reject(error);
		});
	});

	const headerEnd = response.indexOf("\r\n\r\n");
	const statusLine = response.subarray(0, response.indexOf("\r\n")).toString();
	const status = Number(statusLine.match(/^HTTP\/1\.\d (\d{3})/)?.[1]);
	if (headerEnd < 0 || status != HTTP_OK)
		throw new Error(`${request.method} ${request.url} via ${request.server}: ${statusLine}`);
	return response.subarray(headerEnd + 4);
}

function parseServer(endpoint: string) {
	const [host, port] = endpoint.split(":");
	if (host === undefined || port === undefined)
		throw new Error(`bad endpoint ${endpoint}`);
	return { host, port: Number(port) };
}

function encodeSendReq() {
	return Buffer.concat([
		Buffer.from([FIELD_MESSAGE_TYPE, MESSAGE_SEND_REQ, FIELD_TRANSACTION_ID]),
		Buffer.from(`${MMS_TRANSACTION_ID}\0`),
		Buffer.from([FIELD_MMS_VERSION, MMS_VERSION_1_2, FIELD_FROM, 1, INSERT_ADDRESS_TOKEN, FIELD_TO]),
		Buffer.from(`${MMS_RECIPIENT}/TYPE=PLMN\0`),
		Buffer.from([FIELD_CONTENT_TYPE, CONTENT_TYPE_MULTIPART_MIXED, 0]),
	]);
}

const SEND_CONF_OK = Buffer.concat([
	Buffer.from([FIELD_MESSAGE_TYPE, MESSAGE_SEND_CONF, FIELD_TRANSACTION_ID]),
	Buffer.from(`${MMS_TRANSACTION_ID}\0`),
	Buffer.from([FIELD_MMS_VERSION, MMS_VERSION_1_2, FIELD_RESPONSE_STATUS, RESPONSE_STATUS_OK, FIELD_MESSAGE_ID]),
]);

async function checkWapProxies(ppp: PPP, dnsServer: string) {
	for (const { operator, proxy } of WAP_PROXIES) {
		const body = await sendHttp(ppp, dnsServer, { method: "GET", url: PAGE_URL, server: proxy, target: PAGE_URL });
		console.log(`${operator} WAP: GET ${PAGE_URL} via ${proxy}: ${body.length} bytes`);
	}
}

async function checkMmsProfiles(ppp: PPP, dnsServer: string) {
	for (const { operator, mmsc, proxy } of MMS_PROFILES) {
		const url = new URL(mmsc);
		const server = proxy ?? `${url.hostname}:${url.port || HTTP_PORT}`;
		const target = proxy === undefined ? `${url.pathname}${url.search}` : mmsc;
		const request = { method: "POST", url: mmsc, server, target, mmsPdu: encodeSendReq() };
		const body = await sendHttp(ppp, dnsServer, request);
		if (!body.subarray(0, SEND_CONF_OK.length).equals(SEND_CONF_OK))
			throw new Error(`${operator} MMS: POST ${mmsc}: no m-send-conf with status OK: ${body.toString("hex")}`);
		console.log(`${operator} MMS: POST ${mmsc} via ${proxy ?? "direct connection"}: m-send-conf OK`);
	}
}

async function withWspSession(ppp: PPP, gateway: string, task: (session: WspSession) => Promise<void>) {
	const { host, port } = parseServer(gateway);
	const session = await WspSession.connect(ppp, host, port);
	try {
		await task(session);
	} finally {
		await session.disconnect();
	}
}

async function checkWspGateways(ppp: PPP) {
	for (const { operator, gateway } of WSP_GATEWAYS) {
		await withWspSession(ppp, gateway, async (session) => {
			const request = `${operator} WAP: WSP GET ${PAGE_URL} via ${gateway}`;
			const reply = await session.get(PAGE_URL);
			if (reply.status != WSP_STATUS_OK)
				throw new Error(`${request}: status 0x${reply.status.toString(16)}`);
			console.log(`${request}: ${reply.body.length} bytes`);
		});
	}
}

async function checkWspMmsProfiles(ppp: PPP) {
	for (const { operator, mmsc, gateway } of WSP_MMS_PROFILES) {
		await withWspSession(ppp, gateway, async (session) => {
			const request = `${operator} MMS: WSP POST ${mmsc} via ${gateway}`;
			const reply = await session.post(mmsc, CONTENT_TYPE_MMS_MESSAGE, encodeSendReq());
			if (reply.status != WSP_STATUS_OK || !reply.body.subarray(0, SEND_CONF_OK.length).equals(SEND_CONF_OK))
				throw new Error(`${request}: status 0x${reply.status.toString(16)}, ${reply.body.toString("hex")}`);
			console.log(`${request}: m-send-conf OK`);
		});
	}
}

await withModem(async (at) => {
	await registerAndAttachGprs(at);
	await withPpp(at, APN, async (ppp, connection) => {
		if (connection.primaryDns === undefined)
			throw new Error("PPP: no DNS server");
		await checkWapProxies(ppp, connection.primaryDns);
		await checkMmsProfiles(ppp, connection.primaryDns);
		await checkWspGateways(ppp);
		await checkWspMmsProfiles(ppp);
	});
	await detachGprs(at);
});
