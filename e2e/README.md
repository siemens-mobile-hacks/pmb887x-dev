# E2E tests

Tests over the AT interface of the running phone firmware.

- Test `NAME`: `NAME/NAME.ts` or `NAME/NAME.py`, argument: serial port.
- Shared code: `lib/`.

```sh
./run.sh gsm-creg [SERIAL]                              # phone
tools/unit.py -d siemens-el71 -f EL71.bin -u e2e/gsm-creg  # emulator
```

| Test | Checks |
| --- | --- |
| `gsm-at` | AT interface, SIM ready |
| `gsm-creg` | Network registration |
