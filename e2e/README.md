# E2E tests

Tests over the AT interface of the running phone firmware.

- Test `NAME`: `NAME/NAME.ts` or `NAME/NAME.py`, argument: serial port.
- Shared code: `lib/`.

```sh
./run.sh gsm-ppp [SERIAL]                                                          # phone
tools/unit.py -d siemens-el71 -f EL71.bin -u e2e/gsm-ppp -- --gsm-trxmux 127.0.0.1:5750  # emulator, network from bts/
```

| Test | Checks |
| --- | --- |
| `gsm-ppp` | Network registration, GPRS attach, PPP, ping to 1.1.1.1 |
