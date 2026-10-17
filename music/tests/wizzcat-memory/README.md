# Wizzcat memory, table and PCM regression

Run `python3 run.py`. Requires the local cross toolchain, vasm, Hatari, German
TOS 1.62 and the five MODs listed in the script. Uses the Hatari skill's session
helper; target is STE / 8 MHz / 1 MiB. Generated binaries, PCM and logs live in
ignored `build/`.

Five real MODs plus two synthetic modules (sparse/short samples and empty slots)
run for 1000 blocks each. Allocation canaries are checked each block and on
release, including temporary preparation storage. Module sizes are independently
computed by the host. Tests exercise allocation failure at Load and scratch
allocation, undersized explicit workspace, repeated Start on prepared data,
invalid header/loop/volume, and unloading while the VBL callback is installed.

`probe.s` calls the native Wizzcat VBL routine directly with the native buffer
flips and tracker/mixer intact; no callback is installed during deterministic
PCM capture. DMA is stopped after each call to capture its freshly filled page.
The native example separately verifies normal callback-driven DMA and visuals.

Optional `--baseline /path/to/old/sources` compiles the old `wizzcat.c`, `.h`, `_s.s`
without changing the original preparation or mixer. Five real modules produce
10 MB PCM; all bytes (via per-module SHA256) and 29728 increment/volume table
bytes must match. Synthetic empty slots are only tested on the new core because
the original does not provide a safe silent loop for those instruments.

`results.json` records allocation sizes, HZ200 Init elapsed ticks, hashes, guard
errors and target faults. Init ticks include guarded allocations and preparation;
only the first new Init builds tables. This is not a runtime CPU profile.
