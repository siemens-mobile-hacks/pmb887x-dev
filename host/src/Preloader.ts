import { readFileSync } from "node:fs";
import { join } from "node:path";
import { crc32 } from "node:zlib";

import { AsyncSerialPort, BSLStatus, loadBootCode } from "@sie-js/serial";
import { compressBlock, compressBound } from "lz4js";

import type { Platform } from "./header.ts";
import { formatHex } from "./utils.ts";

const PRELOADER_DIR = join(import.meta.dirname, "..", "..", "boot");
const PRELOADER_TIMEOUT_MS = 2000;
const LZ4_BLOCK_SIZE = 0x1000;
const LZ4_HASH_TABLE_SIZE = 1 << 16;

enum Command {
	PING = 0x41,
	INIT_EXTRAM = 0x45,
	GOTO = 0x47,
	SET_SPEED = 0x48,
	WRITE_RAM_LZ4 = 0x4C,
	RAM_WRITE = 0x57,
}

enum Status {
	PONG = 0x52,
	READY = 0xA5,
	SUCCESS = 0xC1,
}

function compressLz4Block(block: Buffer, hashTable: Uint32Array) {
	const output = Buffer.alloc(compressBound(block.length));
	hashTable.fill(0);
	const size = compressBlock(block, output, 0, block.length, hashTable);
	return output.subarray(0, size);
}

export class Preloader {
	readonly cpu: string;
	readonly secureBoot: boolean;
	#port: AsyncSerialPort;

	private constructor(port: AsyncSerialPort, cpu: string, secureBoot: boolean) {
		this.cpu = cpu;
		this.secureBoot = secureBoot;
		this.#port = port;
	}

	static async load(port: AsyncSerialPort, platform: Platform, payloadCpu: string | undefined, ignition: boolean) {
		const result = await loadBootCode(port, ({ cpu }) => {
			if (payloadCpu !== undefined && cpu != payloadCpu)
				throw new Error("payload CPU does not match the connected phone");
			return readFileSync(join(PRELOADER_DIR, `preloader-${platform}-${cpu}.bin`));
		}, { autoIgnition: ignition });
		if (!result.success)
			throw new Error(`BSL: ${result.error}`);
		if (result.status != BSLStatus.SUCCESS)
			throw new Error(`BSL: ${BSLStatus[result.status]}`);

		const preloader = new Preloader(port, result.cpu, result.secureBoot);
		await preloader.#expectByte(Status.READY, "preloader ready status");
		return preloader;
	}

	async #expectByte(expected: Status, description: string) {
		const value = await this.#port.readByte(PRELOADER_TIMEOUT_MS);
		if (value < 0)
			throw new Error(`timeout waiting for ${description}`);
		if (value != expected)
			throw new Error(`invalid ${description}: 0x${formatHex(value)}, expected 0x${formatHex(expected)}`);
	}

	async #command(command: Command, argument: Buffer, status = Status.SUCCESS) {
		await this.#port.write(Buffer.concat([Buffer.from([command]), argument]));
		await this.#expectByte(status, `${Command[command]} status`);
	}

	async setSpeed(baudRate: number) {
		if (baudRate == this.#port.baudRate)
			return;
		const request = Buffer.alloc(5);
		request[0] = Command.SET_SPEED;
		request.writeUInt32LE(baudRate, 1);
		await this.#port.write(request);
		await this.#port.drain();
		await this.#port.update({ baudRate });
	}

	async ping() {
		await this.#command(Command.PING, Buffer.alloc(0), Status.PONG);
	}

	async initExtram(address: number, size: number) {
		const argument = Buffer.alloc(8);
		argument.writeUInt32LE(address, 0);
		argument.writeUInt32LE(size, 4);
		await this.#command(Command.INIT_EXTRAM, argument);
	}

	async writePayload(address: number, data: Buffer) {
		const hashTable = new Uint32Array(LZ4_HASH_TABLE_SIZE);
		for (let offset = 0; offset < data.length; offset += LZ4_BLOCK_SIZE) {
			const block = data.subarray(offset, offset + LZ4_BLOCK_SIZE);
			const crc = crc32(block);
			const compressed = compressLz4Block(block, hashTable);
			if (compressed.length != 0 && compressed.length < block.length) {
				const argument = Buffer.alloc(12 + compressed.length + 4);
				argument.writeUInt32LE(address + offset, 0);
				argument.writeUInt32LE(compressed.length, 4);
				argument.writeUInt32LE(block.length, 8);
				compressed.copy(argument, 12);
				argument.writeUInt32LE(crc, 12 + compressed.length);
				await this.#command(Command.WRITE_RAM_LZ4, argument);
			} else {
				const argument = Buffer.alloc(8 + block.length + 4);
				argument.writeUInt32LE(address + offset, 0);
				argument.writeUInt32LE(block.length, 4);
				block.copy(argument, 8);
				argument.writeUInt32LE(crc, 8 + block.length);
				await this.#command(Command.RAM_WRITE, argument);
			}
		}
	}

	async goto(address: number) {
		const argument = Buffer.alloc(4);
		argument.writeUInt32LE(address);
		await this.#command(Command.GOTO, argument);
	}
}
