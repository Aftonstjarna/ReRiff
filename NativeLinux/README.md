# Native Linux reconstruction

This directory contains the native reconstruction and an initial Vulkan vehicle
viewer. It is **not yet a playable Linux game**.
The native pipeline is C++17 throughout; see [migration details](MIGRATION.md).
The existing Windows/Proton restoration remains separate. Nothing here patches
the installed game, launches it, or modifies its saved data.

Vehicle meshes and texture batches can now be decoded and exported as glTF,
DDS, and PNG. See [asset decoding and format findings](ASSET-FORMATS.md) for
commands, validation, loader evidence, and remaining shader limitations.

After building, to open the native viewer, run `build/native-linux/reriff` from the project
root. See [viewer controls, dependencies and verification](VIEWER.md).
Add `--track` for the reconstructed road scene; see [track findings](TRACK-SCENE.md).
Use `--section combined` for curved, sloped and banked test geometry generated
with the recovered native math. See [spline findings and validation](SPLINES.md).
Use `--song 'Rolling On'` to generate a full road from the original bundled
analysis, or `--list-songs` to list the bundled catalog. See
[song decoding, generation and reference-validation status](SONG-TRACKS.md).

## Build and exercise the native code

Requires a C++17 compiler, Make, zlib, OpenSSL and libpng development headers,
SDL3 runtime/headers, Vulkan drivers and libshaderc_shared. FFmpeg is optional
for audio preparation. Build, tests and runtime do not require Python.
From the ReRiff project root:

```sh
make -C NativeLinux all test
build/native-linux/reriff-native '/path/to/Riff Racer' --verify
build/native-linux/reriff-native '/path/to/Riff Racer' --list
build/native-linux/reriff-native '/path/to/Riff Racer' --config scripts/config/vehicles_PC.cfg
```

`reriff-native` is a Linux ELF executable. It opens the original `vfs.zip`
directly, supports stored and raw-deflate entries, verifies sizes and CRCs, and
decodes GFCF v3 configuration trees into JSON. It needs neither a CLR nor Wine.
ZIP64, split archives, encrypted entries, and files over 256 MiB are rejected.
It deliberately has no asset extraction/write command.

`gameplay.hpp` contains two standalone C++ translations from the original IL:
`gaTrackGen.calcVol` and `gaVehicleGearbox.torqueFunction`. Their source method
tokens and RVAs are recorded alongside the functions. These are exercised by the
native test executable; they are not yet connected to a game loop. Tests cover
known values and boundaries; comparison against the running original remains to
be done. Platform math libraries can differ at the last floating-point bits.

## Reproduce the executable survey

```sh
python3 -m venv build/native-analysis-env
build/native-analysis-env/bin/pip install -r NativeLinux/analysis-requirements.txt
build/native-analysis-env/bin/python scripts/analyze-native-port.py \
  --game-dir '/path/to/Riff Racer' --output analysis/native-port
```

The survey records executable identity, imported symbols, method signatures,
tokens and addresses, asset headers, and resolved IL disassemblies for methods
whose names begin with `ga` or `gf`. This is disassembly, not recompilable C++.
It preserves exception handler records and local signature tokens, but does not
reconstruct native class layouts, virtual calls, or exception behavior.

Generated files are under the git-ignored `analysis/` directory. They contain
original game material and are local research inputs, not release assets.

## Findings on the installed executable

Analyzed SHA-256:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.

- PE32 x86, image base `0x400000`, mixed managed/native executable; CLR flags 0.
- Metadata contains 6,224 methods: 5,722 marked IL and 502 marked native.
  These counts describe implementation flags, not counts of complete portable
  functions. Some methods are wrappers or require Windows/native dependencies.
- 2,962 `ga`/`gf` method bodies disassembled without parser failures.
- Named logic includes `gaTrackGen`, `gaAudioAnalysis`, `gaVehiclePhys`,
  `gaVehicleGearbox`, `gaRace`, and the menu/state managers.
- 686 archive entries, all decompressed and CRC-checked by the native reader;
  all 26 GFCF configurations decoded with every record reachable exactly once.
- Asset extensions: 261 `.bdae`, 134 `.brres`, 93 `.btinfo`, 93 `.hzc`,
  26 each `.cfg`, `.luac`, `.luac64`, 24 `.str`, two `.wavebatch`, one `.dat`.
- Example headers are `EADB` for BDAE and `TABT` for texture batches.
  A `.brres` suffix does not establish compatibility with Nintendo BRRES tools.
- Lua chunks begin with a Lua 5.1 signature; the sampled 32-bit chunk declares
  four-byte numbers. Compatibility needs validation before choosing a Lua build.

The native reader recovers, for example, `carParams/base/TopSpeed5 = 230`,
`Gravity = 40`, and `Mass = 750` from `vehicles_PC.cfg`. These are stored values;
their units and the behavior of the `fast` overrides require caller analysis.

## GFCF v3 layout

All observed integers are little endian. Header:

| Offset | Meaning |
| --- | --- |
| 0 | `GFCF` |
| 4 | Version, 3 |
| 8 | String pool byte length |
| 12 | Record count |
| 16 | Root record count |
| 20 | Start of 32-byte records |

Each record contains eight 32-bit words: name string offset, zero, value, zero,
name hash, type, child count, zero. Type 0 is a string pool offset, type 1 a
float32 bit pattern, type 2 an absolute index of the first child record. Table
children occupy a contiguous range; the first root records are at index zero.
The shared NUL-terminated string pool immediately follows the record array.
The zero fields may be padding/high pointer words in the original serializer;
their broader semantics are unconfirmed. The decoder accepts only observed v3
layout. Non-ASCII string bytes are JSON-escaped individually, without assuming
an encoding. Hashes are preserved, not recomputed or validated.

## Remaining implementation

Vehicle EADB mesh/index/material data and TABT textures have a working
decoder and diagnostic preview. The native SDL3/Vulkan viewer now renders the
body and wheels using approximate materials. Track mode reconstructs a straight
road from recovered rules and samples original track texture masks through a
static color approximation. Native spline and orientation routines now generate
validated curved, sloped and banked test sections. Bundled-song analysis now drives
the ported segment generator and complete road scenes. Full-route parity with
the original runtime now passes for Rolling On within 0.01 world units; see [song validation](SONG-TRACKS.md). Import evidence also
identifies XInput, X3DAudio, Win32 window/filesystem calls, FFTW, and the Parse
bridge. Audio backend internals and physics library boundaries still need
identification; an `fmod` string alone does not establish the complete backend.

Native audio playback and a moving song-synchronized preview are available with
`build/native-linux/reriff --song 'Rolling On' --play`. Original timing
evaluations pass independent comparison; output latency is not calibrated.
Next, restore shaders and port vehicle physics, input, race states, menus and persistence. Compare deterministic outputs with the original
at each boundary: analyzed beats, generated geometry, torque curves, and replay
positions. A full port cannot be produced by renaming the EXE or recompiling the
Parse DLLs; the 32-bit pointers and Windows/native calls in the recovered IL
must be replaced explicitly.

## Vehicle physics

The prerequisite byte-for-byte original source rebuild is not complete.
See [matching decompilation status](../Decomp/README.md); the comparisons below
validate sampled behavior, not executable identity.

The placeholder driving integrator has been removed. Original vehicle routines
have been located. Pace control, manual drive impulses, drift-input transitions,
steering, grip, friction, adhesion, skid state, skid forces, yaw damping, body force accumulation, suspension and tire forces are now ported and checked against 21,447
original-executable cases. Steered wheel transforms and body/contact integration remain unported.
Use the recovered song/asset preview above. See [source map and validation](DRIVING.md).
