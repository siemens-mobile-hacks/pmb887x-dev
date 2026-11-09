import { withModem, waitForSimReady } from "#lib/modem.ts";

await withModem(async (at) => {
	const manufacturer = await at.sendCommandNoPrefix("AT+CGMI");
	if (!manufacturer.success || manufacturer.lines.length == 0)
		throw new Error(`AT+CGMI: ${manufacturer.status}`);
	console.log(`manufacturer: ${manufacturer.lines.join(" ")}`);

	await waitForSimReady(at);
	console.log("AT interface is ready");
});
