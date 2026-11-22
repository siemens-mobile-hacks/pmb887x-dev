import { Command, InvalidArgumentError, Option } from "commander";

import { boot } from "./boot.ts";
import { PLATFORMS, type Platform } from "./header.ts";
import { findSerialAdapter } from "./serial.ts";
import { parseInteger } from "./utils.ts";

const DEFAULT_SPEED = 1625000;
const DWD_USB_PATH = "webusb://device?vid=058B&pid=0015";

type Options = {
	platform?: Platform;
	device?: string;
	usb?: boolean;
	speed: number;
	writeAddr?: number;
	execAddr?: number;
	ign: boolean;
};

const program = new Command()
	.name("boot")
	.description("Load and run a PMB887X binary.")
	.argument("<payload>", "binary to execute")
	.addOption(new Option("--platform <platform>", "platform (default: from header)").choices(PLATFORMS))
	.addOption(new Option("-d, --device <path>", "serial port, tcp://, unix:// or webusb:// path (default: auto)")
		.conflicts("usb"))
	.option("--usb", `boot over the USB DWD interface (${DWD_USB_PATH})`)
	.addOption(new Option("-s, --speed <baud>", "baud rate").argParser(parseSpeedOption).default(DEFAULT_SPEED)
		.conflicts("usb"))
	.option("--write-addr <address>", "write address (default: from header)", parseAddressOption)
	.option("--exec-addr <address>", "entry address (default: from header)", parseAddressOption)
	.showHelpAfterError()
	.parse();
const options = program.opts<Options>();
const [payloadPath] = program.args as [string];

let status = 0;
try {
	let devicePath = options.usb ? DWD_USB_PATH : options.device;
	if (devicePath === undefined) {
		const adapter = await findSerialAdapter();
		console.error(`Using UART adapter: ${adapter.description}`);
		devicePath = adapter.path;
	}
	await boot({
		payloadPath,
		devicePath,
		platform: options.platform,
		speed: options.usb ? undefined : options.speed,
		writeAddress: options.writeAddr,
		execAddress: options.execAddr,
	});
} catch (error) {
	console.error(error instanceof Error ? error.message : String(error));
	status = 1;
}
await new Promise((resolve) => process.stdout.write("", resolve));
process.exit(status);

function parseAddressOption(value: string) {
	const address = parseInteger(value);
	if (address === undefined)
		throw new InvalidArgumentError("Not a number.");
	if (address < 0 || address > 0xFFFFFFFF)
		throw new InvalidArgumentError("Not a 32-bit address.");
	return address;
}

function parseSpeedOption(value: string) {
	const speed = parseInteger(value);
	if (speed === undefined)
		throw new InvalidArgumentError("Not a number.");
	if (speed <= 0)
		throw new InvalidArgumentError("Not a positive baud rate.");
	return speed;
}
