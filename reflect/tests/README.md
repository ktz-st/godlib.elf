# Reflect / JSON / FED regression

Build and run the production library on an emulated 68000:

```sh
make -C ../..
make check
```

`REFTEST.PRG` uses the same `-mshort -mfastcall -m68000` ABI and libcmini as the
examples. Hatari uses STE, 8 MHz, 1 MiB, RGB PAL and German TOS 1.62. The Python
runner uses the installed hatari-debugging skill helper and checks the result
written by the native program; no emulator memory is patched. Test binaries and
transient RESULT.BIN are ignored. `verification.json` records the release hash.

The fixture checks all scalar widths, signed/unsigned 32-bit limits (including
values beyond 16-bit int), field-name lookup, booleans, floats and exponent
notation, bounded non-terminated token views, string descriptor copies, malformed
and overflowing values, invalid metadata/input, and byte-for-byte guards around
writes. It also runs the real JSON parser/mapper, including negative/scientific
numbers, and uses the actual FED layout reflection descriptor.

Reflect_SetData returns 1 after writing a supported field; failures return 0 and
leave its destination unchanged. Integers are decimal and must fit the target
field; out-of-range narrowing is rejected instead of the previous truncation.
F32 accepts decimal/exponent notation, rejecting overflow and non-finite values.
Strings retain the existing shallow descriptor-copy semantics: no allocation,
no automatic freeing, and the source storage must remain valid. JSON mapping
still ignores the per-field return value and leaves rejected fields unchanged.
