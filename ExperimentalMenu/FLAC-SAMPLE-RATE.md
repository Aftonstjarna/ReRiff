# FLAC analysis sample-rate patch

This intentional ReRiff patch converts decoded FLAC analysis PCM to 44,100 Hz
before DAT analysis. It leaves source files and playback unchanged. Already
44.1 kHz input takes the original path without resampling.

## Original evidence

Supported Game.exe SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`
(8,635,392 bytes; Steam depot 351991, build 1687655). References were checked
against `packaging/vanilla-reference.json` and the user's RiffRacerOriginals copy.
Addresses below are original VAs at image base `0x00400000`.

* Managed token `0x060014DB`, `gaNewTrackScreen.startDecodeForEssentia`, requests
  44,100 Hz at IL `0051`. Token `0x060014DD`, `essentiaJob`, supplies 44,100 Hz
  at IL `0059` to `gaAudioAnalysis.analyse` (call at `006a`).
* `gaAudioAnalysis.analyse` (`0x06000EC9`) calculates duration from samples/rate.
  Section and volume stages (`0x06000EAE`, `0x06000EB2`) use the original
  `s_requiredSampleRate` global. Merely relabeling 96 kHz samples is incorrect.
* Worker `0x0048D9C0` calls FLAC decoder `0x004F9080` at `0x0048DB0B`.
  Guard bytes are `e8 70 b5 06 00 83 c4 1c`; decoder prefix `55 8b ec 6a ff`.
  Input pointer/size arrive in ECX/EDX; seven stack arguments are caller-popped.
  This is not ordinary Microsoft `__fastcall`. Preserve all seven arguments,
  metadata and the original decoder's PCM conversion/channel behavior.
* Original Ogg resampler constructor: `0x005623E0`, prefix
  `55 8b ec 83 ec 18 66 0f 28 c1`. Ratio arrives in XMM1/XMM2; plain return.
  Processing: `0x00562630`, prefix `55 8b ec 83 ec 28 f2 0f 10 4d 1c`.
  ECX = state, EDX = input; caller-popped stack = input frames, EOF, consumed
  pointer, output, capacity, double ratio. Return = produced frames.
* Preserve the original float target/source division before converting to double.
  Ogg cleanup at `0x004F8D65` frees state pointers at offsets `0x2c`, `0x40`,
  `0`, `4`, then state. Original new[] entry is `0x008F87C8`; delete[] IAT
  is `0x0092D4A0`, CRT free IAT `0x0092D3A4`.

Evidence: original managed IL, original instruction disassembly and Ghidra
read-only exports in the private `/tmp/reriff-flac-research/Audio` project.
The targeted export at `0x005624B0` is an interior constructor address, not a
separate function. No compiler-built matching-byte credit is claimed.

## Integration and validation

`flac_sample_rate.h` replaces only the worker's FLAC CALL in memory after the
existing executable identity checks and DLL lifetime pin. Exact instruction
guards refuse unsupported code. The original decoder runs first; its PCM is
converted through the original Ogg resampler, flushed at EOF, and replaced only
on success using original allocator/free pairs. Failure rejects analysis.
The on-disk Game.exe is unchanged. Standard STREAMINFO and a bounded optional
ID3v2 prefix supply the input rate. This adds no external codec dependency.

`test_flac_sample_rate.c` maps the original native code in an isolated Wine
prefix, resolves native imports and applies relocation entries when necessary;
it never starts GameMain or accesses saves. It invokes the installed CALL target,
checks guard refusal/idempotence, metadata, finite samples and frame counts.
Build through `ExperimentalMenu/build-engine.sh`; invoke `test-flac.exe` with
original Game.exe, input FLAC and temporary output float-PCM paths.

| Input | Original frames | Analysis frames | Duration at 44.1 kHz |
| --- | ---: | ---: | ---: |
| FLOW — GET BACK, 96 kHz / 24-bit | 18,757,120 | 8,616,553 | 195.386689 s |
| Same song, 44.1 kHz / 24-bit | 8,616,552 | 8,616,552 | 195.386667 s |
| 2-second tone, 32 kHz | 64,000 | 88,202 | 2.000045 s |
| 2-second tone, 48 kHz | 96,000 | 88,201 | 2.000023 s |
| 2-second tone, 96 kHz | 192,000 | 88,201 | 2.000023 s |
| 2-second tone, 192 kHz | 384,000 | 88,201 | 2.000023 s |

The 44.1 kHz PCM is byte-for-byte identical to the original decoder output.
Tone frequency/RMS checks retain pitch and amplitude. One or two extra output
frames are the observed original resampler's boundary behavior. Integrated
native menu/input/display/UI-scale/save-prompt and D3D9/9Ex AA checks also pass.
An in-game regenerated-track race remains necessary to confirm the full fix.

## Existing caches

Existing `_v18` analysis is not rewritten: back up and regenerate affected
tracks. Old scores/ghosts were created against different geometry; this patch
does not migrate server history or change its source-song identity mapping.
See [original reproduction](../reports/FLAC-SAMPLE-RATE.md). This is an analysis
rate fix, not a replacement decoder or a direct 96 kHz DAT pipeline.
