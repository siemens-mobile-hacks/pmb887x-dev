import { readFileSync } from "node:fs";
import { join } from "node:path";
import { crc32 } from "node:zlib";

import { AsyncSerialPort, BSLStatus, EBL, loadBootCode } from "@sie-js/serial";
import { compressBlock, compressBound } from "lz4js";

import type { Platform } from "./header.ts";
import { formatHex } from "./utils.ts";

const PRELOADER_DIR = join(import.meta.dirname, "..", "..", "boot");
const PRELOADER_TIMEOUT_MS = 2000;
const LZ4_HASH_TABLE_SIZE = 1 << 16;

const DWD_CODE_LINK = 0x1AE0;
const DWD_COPY_LEN = 0x89C;
const DWD_COPY_SRC = 0x2760;

enum Command {
	PING = 0x41,
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
	#port: AsyncSerialPort;
	#platform: Platform;
	#isDWD: boolean = false;
	#isSecureBoot: boolean = false;
	#bootLoader: string = "unknown";
	#detectedCPU: string = "unknown";

	constructor(port: AsyncSerialPort, platform: Platform) {
		this.#port = port;
		this.#platform = platform;
	}

	async connect() {
		const result = await loadBootCode(this.#port, ({ cpu }) => {
			return readFileSync(join(PRELOADER_DIR, `preloader-${this.#platform}-${cpu}.bin`));
		});
		if (!result.success)
			throw new Error(`BSL: ${result.error}`);
		if (result.status != BSLStatus.SUCCESS && result.status != BSLStatus.DWD_BOOTLOADER)
			throw new Error(`BSL: ${BSLStatus[result.status]}`);

		this.#detectedCPU = result.cpu;
		this.#isDWD = result.status == BSLStatus.DWD_BOOTLOADER;

		if (this.#isDWD) {
			const preloaderFile = join(PRELOADER_DIR, "dwd-usb-preloader-apoxi-pmb8876.bin");
			const preloaderBuffer = readFileSync(preloaderFile);

			const payloadBuffer = Buffer.alloc(0x800);
			payloadBuffer.writeUInt16LE(DWD_COPY_LEN, 0);
			preloaderBuffer.copy(payloadBuffer, 0x14);
			payloadBuffer.writeUInt32LE(DWD_CODE_LINK, 0x678);

			const ebl = new EBL(this.#port);
			await ebl.connect({ skipBSL: true });

			const eblInfo = ebl.getInfo();
			this.#bootLoader = `${eblInfo.name} ${eblInfo.major}.${eblInfo.minor} (0x${eblInfo.mode.toString(16).toUpperCase()})`;

			const flashInfo = await ebl.getFlashInfo();
			await ebl.selectFlashDriver(flashInfo);

			await ebl.setAddress(0x60000000 + DWD_COPY_SRC);
			await ebl.sendPacket(0x803, payloadBuffer);
		} else {
			this.#bootLoader = "BSL";
		}

		await this.#waitACK(Status.READY, "preloader ready status");
		await this.ping();
		await this.ping();
	}

	getDetectedCPU() {
		return this.#detectedCPU;
	}

	getBootLoader() {
		return this.#bootLoader;
	}

	isSecureBoot() {
		return this.#isSecureBoot;
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
		await this.sendCommand(Command.PING, Buffer.alloc(0), Status.PONG);
	}

	async writePayload(address: number, data: Buffer) {
		if (this.#isDWD) {
			return await this.writeUncompressedPayload(address, data, 0x400);
		} else {
			return await this.writeCompressedPayload(address, data, 0x1000);
		}
	}

	async writeUncompressedPayload(address: number, data: Buffer, blockSize: number) {
		for (let offset = 0; offset < data.length; offset += blockSize) {
			const block = data.subarray(offset, offset + blockSize);
			const crc = crc32(block);
			const command = Buffer.alloc(8 + block.length + 4);
			command.writeUInt32LE(address + offset, 0);
			command.writeUInt32LE(block.length, 4);
			block.copy(command, 8);
			command.writeUInt32LE(crc, 8 + block.length);
			await this.sendCommand(Command.RAM_WRITE, command);
		}
	}

	async writeCompressedPayload(address: number, data: Buffer, blockSize: number) {
		const hashTable = new Uint32Array(LZ4_HASH_TABLE_SIZE);
		for (let offset = 0; offset < data.length; offset += blockSize) {
			const block = data.subarray(offset, offset + blockSize);
			const crc = crc32(block);
			const compressed = compressLz4Block(block, hashTable);
			if (compressed.length != 0 && compressed.length < block.length) {
				const command = Buffer.alloc(12 + compressed.length + 4);
				command.writeUInt32LE(address + offset, 0);
				command.writeUInt32LE(compressed.length, 4);
				command.writeUInt32LE(block.length, 8);
				compressed.copy(command, 12);
				command.writeUInt32LE(crc, 12 + compressed.length);
				await this.sendCommand(Command.WRITE_RAM_LZ4, command);
			} else {
				const command = Buffer.alloc(8 + block.length + 4);
				command.writeUInt32LE(address + offset, 0);
				command.writeUInt32LE(block.length, 4);
				block.copy(command, 8);
				command.writeUInt32LE(crc, 8 + block.length);
				await this.sendCommand(Command.RAM_WRITE, command);
			}
		}
	}

	async execute(address: number) {
		const command = Buffer.alloc(4);
		command.writeUInt32LE(address);
		await this.sendCommand(Command.GOTO, command);
	}

	async sendCommand(command: Command, argument: Buffer, status = Status.SUCCESS) {
		await this.#port.write(Buffer.concat([Buffer.from([command]), argument]));
		await this.#waitACK(status, `${Command[command]} status`);
	}

	async #waitACK(expected: Status, description: string) {
		const value = await this.#port.readByte(PRELOADER_TIMEOUT_MS);
		if (value < 0)
			throw new Error(`timeout waiting for ${description}`);
		if (value != expected)
			throw new Error(`invalid ${description}: 0x${formatHex(value)}, expected 0x${formatHex(expected)}`);
	}
}
