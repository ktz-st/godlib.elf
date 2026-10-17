# ModScope sampler oracle

`python3 run.py` builds/runs a native STE test on German TOS 1.62, 8 MHz, 1 MiB.
Requires the cross compiler, Hatari and the Hatari skill's `hatari_session.py`.

The shared assembly sampler is compared to an independent C reference for 256
sets of four synthetic voices (1024 total). Source length, loop start/end,
period, volume, encoding and enabled state vary. Tests include a subsequent loop
end differing from the first sample end, two-byte loops, loop overshoot,
one-shot zero fill, invalid periods, volume clamping, signed-seven-bit Lance
encoding and output sentinels around an intentionally odd destination pointer.
`results.json` stores target errors, case count and faults.

The real replay getter tests are the adjacent memory harnesses with `--scopes`.
They call each getter after every mixed block, check output sentinels and compare
PCM hashes with their previous no-getter results. Visual/framebuffer verification
is `godlib.spl/lancemod/analysis/verify-scopes.py`.
