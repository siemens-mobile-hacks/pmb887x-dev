import { setTimeout as delay } from "node:timers/promises";
import { crc32 } from "node:zlib";

import type { AsyncSerialPort } from "@sie-js/serial";

import { DEFAULT_FORMAT, openPort, sendBreak, transmit, type SerialFormat } from "./serial.ts";
import { formatHex, parseInteger } from "./utils.ts";

const OUTPUT_TIMEOUT_MS = 1000;
const HOST_COMMAND_PREFIX = "# HOST-CMD: ";
const EXECUTION_CONTINUED = "# ERROR: execution continued";
const EXPECTED_RESET_OBSERVATION_MS = 1000;
const EXPECTED_RESET_RELEASE = Buffer.from("R");
const XORSHIFT32_MAX_LENGTH = 1024 * 1024;
const NEWLINE = 0x0A;

const SERIAL_FORMATS = new Map<string, SerialFormat>([
	["7E1", { dataBits: 7, parity: "even" }],
	["7O1", { dataBits: 7, parity: "odd" }],
	["8N1", { dataBits: 8, parity: "none" }],
	["8E1", { dataBits: 8, parity: "even" }],
	["8O1", { dataBits: 8, parity: "odd" }],
]);

type HostCommand = {
	name: string;
	argument: string;
};

type LineSettings = {
	baudRate: number;
	format: SerialFormat;
};

function decodeHostCommand(line: Buffer): HostCommand | undefined {
	const text = line.toString();
	if (!text.startsWith(HOST_COMMAND_PREFIX))
		return undefined;
	const [name = "", ...argument] = text.slice(HOST_COMMAND_PREFIX.length).split(" ");
	return { name, argument: argument.join(" ") };
}

function parseHostNumber(command: string, argument: string, minimum: number) {
	const value = parseInteger(argument);
	if (value === undefined)
		throw new Error(`HOST-CMD ${command} argument is not a number`);
	if (value < minimum)
		throw new Error(`HOST-CMD ${command} must be at least ${minimum}`);
	return value;
}

function parseHostString(command: string, argument: string) {
	if (argument == "")
		throw new Error(`HOST-CMD ${command} has no argument`);
	const text: unknown = JSON.parse(argument);
	if (typeof text != "string")
		throw new Error(`HOST-CMD ${command} argument must be a string`);
	if (text.length == 0)
		throw new Error(`HOST-CMD ${command} argument must not be empty`);
	return Buffer.from(text);
}

function generateXorshift32(length: number, seed: number) {
	const data = Buffer.alloc(length);
	let state = seed;
	for (let offset = 0; offset < length; offset++) {
		state = (state ^ (state << 13)) >>> 0;
		state = (state ^ (state >>> 17)) >>> 0;
		state = (state ^ (state << 5)) >>> 0;
		data[offset] = state & 0xFF;
	}
	return data;
}

function generateHostXorshift32(argument: string) {
	const fields = argument.trim().split(/\s+/);
	if (fields.length != 3)
		throw new Error("HOST-CMD SEND_XORSHIFT32 requires length, seed, and CRC32");
	const [length, seed, expectedCrc] = fields.map(parseInteger);
	if (length === undefined || seed === undefined || expectedCrc === undefined)
		throw new Error("HOST-CMD SEND_XORSHIFT32 length, seed, and CRC32 must be numbers");
	if (length <= 0 || length > XORSHIFT32_MAX_LENGTH)
		throw new Error(`HOST-CMD SEND_XORSHIFT32 length must be between 1 and ${XORSHIFT32_MAX_LENGTH}`);
	if (seed <= 0 || seed > 0xFFFFFFFF)
		throw new Error("HOST-CMD SEND_XORSHIFT32 seed must be a nonzero 32-bit value");
	if (expectedCrc < 0 || expectedCrc > 0xFFFFFFFF)
		throw new Error("HOST-CMD SEND_XORSHIFT32 CRC32 must be a 32-bit value");

	const data = generateXorshift32(length, seed);
	const actualCrc = crc32(data);
	if (actualCrc != expectedCrc) {
		throw new Error(`HOST-CMD SEND_XORSHIFT32 CRC32 is 0x${formatHex(actualCrc, 8)}, ` +
			`expected 0x${formatHex(expectedCrc, 8)}`);
	}
	return data;
}

async function handleExpectedReset(port: AsyncSerialPort, format: SerialFormat, argument: string) {
	const [assertionText = "", failuresText = "", ...nameWords] = argument.split(" ");
	const name = nameWords.join(" ");
	if (name == "")
		throw new Error("HOST-CMD EXPECT_RESET requires assertion number, failure count, and name");
	const assertion = parseInteger(assertionText);
	const failures = parseInteger(failuresText);
	if (assertion === undefined || failures === undefined)
		throw new Error("HOST-CMD EXPECT_RESET assertion number and failure count must be numbers");
	if (assertion <= 0)
		throw new Error("HOST-CMD EXPECT_RESET assertion number must be positive");
	if (failures < 0 || failures >= assertion)
		throw new Error("HOST-CMD EXPECT_RESET failure count must be below the assertion number");

	await transmit(port, format, EXPECTED_RESET_RELEASE);
	const deadline = performance.now() + EXPECTED_RESET_OBSERVATION_MS;
	let continuedOutput = Buffer.alloc(0);
	while (performance.now() < deadline) {
		const data = await port.readNextChunk(AbortSignal.timeout(OUTPUT_TIMEOUT_MS));
		if (data === undefined)
			continue;
		continuedOutput = Buffer.concat([continuedOutput, data]);
		if (continuedOutput.includes(EXECUTION_CONTINUED)) {
			process.stdout.write(continuedOutput.filter((value) => value != 0));
			throw new Error("execution continued after expected reset");
		}
	}

	const status = failures == 0 ? "PASS" : "FAIL";
	process.stdout.write(`ok ${assertion} - ${name}\n1..${assertion}\n# result: ${status} (${failures} failed)\n`);
}

export async function forwardOutput(bootPort: AsyncSerialPort, path: string) {
	let port = bootPort;
	let format = DEFAULT_FORMAT;
	let pending = Buffer.alloc(0);
	let queued: HostCommand[] | undefined;
	let saved: LineSettings | undefined;

	async function configure({ baudRate, format: newFormat }: LineSettings) {
		if (newFormat.dataBits == format.dataBits && newFormat.parity == format.parity) {
			if (baudRate != port.baudRate)
				await port.update({ baudRate });
			return;
		}
		await port.close();
		port = await openPort(path, baudRate, newFormat);
		format = newFormat;
	}

	async function execute({ name, argument }: HostCommand) {
		switch (name) {
			case "BAUDRATE":
				await configure({ baudRate: parseHostNumber(name, argument, 1), format });
				break;
			case "FORMAT": {
				const newFormat = SERIAL_FORMATS.get(argument);
				if (newFormat === undefined)
					throw new Error("HOST-CMD FORMAT must be 7E1, 7O1, 8N1, 8E1, or 8O1");
				await configure({ baudRate: port.baudRate, format: newFormat });
				break;
			}
			case "SEND":
				await transmit(port, format, parseHostString(name, argument));
				break;
			case "EXPECT": {
				const expected = parseHostString(name, argument);
				const actual = await port.read(expected.length, OUTPUT_TIMEOUT_MS);
				if (actual === undefined)
					throw new Error(`HOST-CMD EXPECT timed out waiting for ${JSON.stringify(expected.toString())}`);
				if (!actual.equals(expected)) {
					throw new Error(`HOST-CMD EXPECT received ${JSON.stringify(actual.toString())}, ` +
						`expected ${JSON.stringify(expected.toString())}`);
				}
				break;
			}
			case "SEND_XORSHIFT32":
				await transmit(port, format, generateHostXorshift32(argument));
				break;
			case "WAIT":
				await delay(parseHostNumber(name, argument, 0));
				break;
			case "BREAK":
				await sendBreak(port, parseHostNumber(name, argument, 1));
				break;
			case "RESTORE":
				if (argument != "")
					throw new Error("HOST-CMD RESTORE takes no argument");
				if (saved === undefined)
					throw new Error("HOST-CMD RESTORE outside of a BEGIN/END block");
				await configure(saved);
				break;
			default:
				throw new Error(`unknown HOST-CMD: ${name}`);
		}
	}

	try {
		while (true) {
			let data = await port.readNextChunk(AbortSignal.timeout(OUTPUT_TIMEOUT_MS));
			if (data === undefined)
				continue;

			const terminator = data.indexOf(0);
			if (terminator >= 0)
				data = data.subarray(0, terminator);

			let text = Buffer.concat([pending, data]);
			let expectedReset: string | undefined;
			for (let end = text.indexOf(NEWLINE); end >= 0; end = text.indexOf(NEWLINE)) {
				const command = decodeHostCommand(text.subarray(0, end));
				text = text.subarray(end + 1);
				if (command === undefined)
					continue;

				if (command.argument != "" && (command.name == "BEGIN" || command.name == "END"))
					throw new Error(`HOST-CMD ${command.name} takes no argument`);
				if (command.name == "BEGIN") {
					if (queued !== undefined)
						throw new Error("nested HOST-CMD BEGIN");
					queued = [];
					saved = { baudRate: port.baudRate, format };
				} else if (command.name == "END") {
					if (queued === undefined)
						throw new Error("HOST-CMD END without BEGIN");
					for (const queuedCommand of queued)
						await execute(queuedCommand);
					queued = undefined;
				} else if (command.name == "EXPECT_RESET") {
					if (queued !== undefined)
						throw new Error("HOST-CMD EXPECT_RESET inside a BEGIN/END block");
					if (expectedReset !== undefined)
						throw new Error("repeated HOST-CMD EXPECT_RESET");
					expectedReset = command.argument;
				} else if (queued === undefined) {
					await execute(command);
				} else {
					queued.push(command);
				}
			}
			pending = text;

			if (terminator >= 0 && pending.length != 0)
				throw new Error("unexpected completion byte inside test output");
			if (terminator >= 0 && queued !== undefined)
				throw new Error("unfinished HOST-CMD block");
			process.stdout.write(data);
			if (terminator >= 0)
				return;
			if (expectedReset !== undefined) {
				await handleExpectedReset(port, format, expectedReset);
				return;
			}
		}
	} finally {
		if (port !== bootPort)
			await port.close();
	}
}
