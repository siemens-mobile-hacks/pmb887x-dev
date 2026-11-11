import { isIP, type Socket } from "node:net";

import { DNS, TcpSocket, type PPP } from "@sie-js/serial";
import { Agent } from "undici";

const HTTP_PORT = 80;

export function createPppDispatcher(ppp: PPP, dnsServer: string) {
	return new Agent({
		connect(options, callback) {
			connectHttp(ppp, dnsServer, options.protocol, options.hostname, options.port).then(
				(socket) => callback(null, socket as unknown as Socket),
				(error: Error) => callback(error, null),
			);
		},
	});
}

async function connectHttp(ppp: PPP, dnsServer: string, protocol: string, hostname: string, port: string) {
	if (protocol != "http:")
		throw new Error(`${protocol} is not supported over PPP`);
	const address = isIP(hostname) ? hostname : (await new DNS(ppp).resolveIpv4(hostname, dnsServer))[0];
	if (address === undefined)
		throw new Error(`${hostname} has no IPv4 address`);
	const socket = new TcpSocket(ppp);
	await socket.connect(address, port ? Number(port) : HTTP_PORT);
	return socket;
}
