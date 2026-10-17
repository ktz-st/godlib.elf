# Leonard memory and rate tests

Run `python3 run.py` from any directory. Requires the local cross tools, Hatari,
German TOS 1.62 and the five MOD assets named in `run.py`. Builds are ignored in
`build/`. The Hatari skill's `hatari_session.py` handles target access.

Target: STE / 8 MHz / 1 MiB. Five real MODs plus sparse high-note/short-loop and
empty synthetic MODs each run for 1000 deterministic blocks at both rates.
All replay heap allocations have leading/trailing canaries checked every block
and on release. Assertions cover exact module/workspace sizes, code generator
lengths, workspace OOM, rate replacement, repeated shutdown, rejected malformed
MODs, DMA stop and trap #0 restoration. `probe.s` uses the native tracker and
four native voice mixer calls, skipping only the DMA-position scheduling test.
The production example separately verifies the actual DMA scheduler and USP
transition in `godlib.spl/leonard/analysis/verify-hatari.py`.

`HIGH.BIN` is compared with an independent host oracle for periods 108, 113, 127
and 141 at 12.5K. It checks unclamped 16.16 source advances and overshoot across
a six-byte loop after the first 32-byte pass. Generated code must measure
28496/56496 bytes; resident allocations must measure 61134/91134 bytes.

Optional regression:

```sh
python3 run.py --baseline /path/to/old/private/leonard/sources
```

The baseline directory contains `leonard_s.s`, `leonardpaula.s` and
`leonardtracker.s` from the previous 25K sample. The harness adds only code-end
instrumentation, a deterministic mixing probe, bridge symbol renames/ELF section
normalization, and the same clean per-song tracker initialization as the new
version. The old mixer is unchanged. Seven 25K streams (1000 blocks each) must
have identical SHA256 hashes. `--25-only` compares both engines without any
12.5K runs; it was also used to establish that the old partial reset caused a
difference after mixing rates, rather than a change to the 25K kernel.

`results.json` records the latest complete run. PCM files/logs are generated
locally and are not checked in.
