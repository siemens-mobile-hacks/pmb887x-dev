import { UDP, type PPP, type UdpMessage } from "@sie-js/serial";

const RESPONSE_TIMEOUT_MS = 30_000;

const WTP_INVOKE = 1;
const WTP_RESULT = 2;
const WTP_ACK = 3;
const WTP_GTR_TTR = 0x06;
const WTP_TID_VERIFY = 0x04;
const WTP_RESPONDER_TID = 0x8000;
const WTP_CLASS_0 = 0;
const WTP_CLASS_2 = 2;

const WSP_CONNECT = 0x01;
const WSP_CONNECT_REPLY = 0x02;
const WSP_REPLY = 0x04;
const WSP_DISCONNECT = 0x05;
const WSP_GET = 0x40;
const WSP_POST = 0x60;
const WSP_VERSION_1_0 = 0x10;
const WSP_HEADER_ACCEPT = 0x80;
const ACCEPT_ANY = Buffer.concat([Buffer.from([WSP_HEADER_ACCEPT]), Buffer.from("*/*\0")]);

export const WSP_STATUS_OK = 0x20;

export type WspReply = {
	status: number;
	body: Buffer;
};

function encodeUintvar(value: number) {
	const bytes = [value & 0x7f];
	for (value >>>= 7; value > 0; value >>>= 7)
		bytes.unshift(0x80 | (value & 0x7f));
	return Buffer.from(bytes);
}

function decodeUintvar(data: Buffer, position: number) {
	let value = 0;
	for (;;) {
		const byte = data[position++];
		if (byte === undefined)
			throw new Error(`WSP: truncated uintvar: ${data.toString("hex")}`);
		value = (value << 7) | (byte & 0x7f);
		if (byte < 0x80)
			return { value, position };
	}
}

function decodeReply(pdu: Buffer): WspReply {
	if (pdu[0] != WSP_REPLY || pdu.length < 3)
		throw new Error(`WSP: no Reply: ${pdu.toString("hex")}`);
	const headers = decodeUintvar(pdu, 2);
	return { status: pdu[1]!, body: pdu.subarray(headers.position + headers.value) };
}

export class WspSession {
	private readonly udp: UDP;
	private readonly messages: UdpMessage[] = [];
	private waiter: (() => void) | undefined;
	private nextTid = 1;
	private sessionId = 0;

	private constructor(ppp: PPP, private readonly address: string, private readonly port: number) {
		this.udp = new UDP(ppp);
		this.udp.onMessage((message) => {
			this.messages.push(message);
			this.waiter?.();
		});
	}

	static async connect(ppp: PPP, address: string, port: number) {
		const session = new WspSession(ppp, address, port);
		try {
			const reply = await session.transact(Buffer.from([WSP_CONNECT, WSP_VERSION_1_0, 0, 0]));
			if (reply[0] != WSP_CONNECT_REPLY)
				throw new Error(`WSP: no ConnectReply from ${address}:${port}: ${reply.toString("hex")}`);
			session.sessionId = decodeUintvar(reply, 1).value;
		} catch (error) {
			session.udp.close();
			throw error;
		}
		return session;
	}

	async get(uri: string) {
		const uriBytes = Buffer.from(uri);
		const pdu = Buffer.concat([Buffer.from([WSP_GET]), encodeUintvar(uriBytes.length), uriBytes, ACCEPT_ANY]);
		return decodeReply(await this.transact(pdu));
	}

	async post(uri: string, contentType: number, body: Buffer) {
		const uriBytes = Buffer.from(uri);
		const headers = Buffer.concat([Buffer.from([contentType]), ACCEPT_ANY]);
		const pdu = Buffer.concat([
			Buffer.from([WSP_POST]), encodeUintvar(uriBytes.length), encodeUintvar(headers.length),
			uriBytes, headers, body,
		]);
		return decodeReply(await this.transact(pdu));
	}

	async disconnect() {
		const pdu = Buffer.concat([Buffer.from([WSP_DISCONNECT]), encodeUintvar(this.sessionId)]);
		try {
			await this.invoke(this.nextTid++, WTP_CLASS_0, pdu);
		} finally {
			this.udp.close();
		}
	}

	private async invoke(tid: number, transactionClass: number, payload: Buffer) {
		const header = Buffer.from([(WTP_INVOKE << 3) | WTP_GTR_TTR, tid >> 8, tid & 0xff, transactionClass]);
		await this.udp.send(this.address, this.port, Buffer.concat([header, payload]));
	}

	private async acknowledge(tid: number, flags: number) {
		await this.udp.send(this.address, this.port, Buffer.from([(WTP_ACK << 3) | flags, tid >> 8, tid & 0xff]));
	}

	private async transact(payload: Buffer) {
		const tid = this.nextTid++;
		await this.invoke(tid, WTP_CLASS_2, payload);
		for (;;) {
			const pdu = await this.receive();
			const type = (pdu[0]! >> 3) & 0x0f;
			if (pdu.readUInt16BE(1) != (tid | WTP_RESPONDER_TID))
				throw new Error(`WTP: unexpected TID: ${pdu.toString("hex")}`);
			if (type == WTP_ACK && (pdu[0]! & WTP_TID_VERIFY)) {
				await this.acknowledge(tid, WTP_TID_VERIFY);
			} else if (type == WTP_RESULT) {
				await this.acknowledge(tid, 0);
				return pdu.subarray(3);
			} else if (type != WTP_ACK) {
				throw new Error(`WTP: unexpected PDU: ${pdu.toString("hex")}`);
			}
		}
	}

	private async receive() {
		const deadline = Date.now() + RESPONSE_TIMEOUT_MS;
		while (this.messages.length == 0) {
			const remaining = deadline - Date.now();
			if (remaining <= 0)
				throw new Error(`WTP: no response from ${this.address}:${this.port}`);
			await new Promise<void>((resolve) => {
				const timer = setTimeout(resolve, remaining);
				this.waiter = () => {
					clearTimeout(timer);
					resolve();
				};
			});
			this.waiter = undefined;
		}
		const message = this.messages.shift()!;
		if (message.address != this.address || message.port != this.port || message.data.length < 3)
			throw new Error(`WTP: unexpected datagram from ${message.address}:${message.port}`);
		return message.data;
	}
}
