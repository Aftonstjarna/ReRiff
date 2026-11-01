# Matching original reconstruction

The baseline objective is a source-built Windows `Game.exe` that matches the
supported original **byte for byte across the entire file**. The native Linux
C++ port is separate work and cannot be used to claim that baseline is complete.

**Current status: no matching source rebuild exists.** ILSpy's `Module.cs` is
research output with unsupported native functions/data and decompiler errors;
it is not compilable recovered game source. Numerical parity tests exercise
selected behavior and do not prove equivalence for every input or binary identity.

## Evidence established

The supported original is 8,635,392 bytes, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The C++ metadata reader independently enumerates 6,224 MethodDef rows, whose
names and RVAs agree with the prior research inventory:

- 5,720 IL bodies preserved exactly, including tiny/fat headers, instruction
  bytes, alignment preceding extra sections and exception tables.
- 502 native MethodDef entries with unresolved function extents. This is not
  a count of all native functions in the executable; unmanaged code also exists
  outside the CLR method table.
- Two MethodDef rows without a body RVA.

Extraction is evidence preservation, **not source reconstruction**. Section
hashes cover original raw data; no padding or metadata is normalized. Original
extracted bytes remain in ignored `analysis/`, not release packages.

The diagnostic `build/song-reference/Game.reference.dll` differs from the
original in seven byte positions across the three intentionally patched header
fields. Its method bodies are unchanged, but the file is not a matching rebuild.

## Tools

On Fedora, the supporting C++ recovery tools require `gcc-c++`, `make`,
`binutils`, `openssl-devel` and `zlib-ng-compat-devel`. These are sufficient for
the tools in this directory, not the original Microsoft mixed-mode build.

```sh
make -C Decomp all test

build/decomp/reriff-match baseline \
  '/path/to/original/Game.exe' analysis/matching-decomp

build/decomp/reriff-match build-evidence \
  '/path/to/original/Game.exe' analysis/matching-decomp/build-evidence.json

build/decomp/reriff-match compare \
  '/path/to/original/Game.exe' '/path/to/source-built/Game.exe' \
  analysis/matching-decomp/rebuild-comparison.json
```

`baseline` checks the original hash, reads the PE/CLR tables directly, preserves
each IL body as `original-il/0xTOKEN.cil` and writes an inventory with per-body
hashes and section hashes. Native boundaries are explicitly unresolved, never
guessed from the next metadata RVA. The original file is read-only.

`compare` checks every byte, including headers, metadata, code, resources,
padding and trailing bytes. It records every differing range without masking
addresses, timestamps or other fields. Exit codes: 0 identical, 1 different,
2 invalid invocation/input. A copied original can of course compare identical;
the report therefore **never** asserts that source rebuilding was verified.
That requires separate build evidence identifying the recovered sources,
compiler/linker and dependencies, commands and resulting artifact.

Tests cover differing bytes, truncation, appended data, tiny/fat IL bodies,
alignment, chained exception sections and rejection of sections outside the file.

## Recovered build evidence and entry instruction

`build-evidence` reads the original EXE's PE/CLR headers, Rich records, debug
records and regular/delay imports. It preserves raw component IDs and build
numbers rather than guessing names or a single compiler version for all objects.
Independent synthetic tests cover Rich decoding and malformed padding, RSDS PDB
records, imports, metadata version and malformed debug tables.

Observed in the supported EXE:

- x86 PE32; linker version 12.0; section/file alignment 4096/512.
- CLR flags 0 (not IL-only); metadata version `v4.0.30319`.
- Runtime imports `MSVCP120.dll`, `MSVCR120.dll`, `mscoree.dll`.
- Rich component 225/build 40629/count 413, component 229/build 40629/count
  370, and component 222/build 40629/count 1, among other input builds.
  The local Ghidra Rich-product table associates component 225 with VS2013 C++
  and 222 with its linker, but does not uniquely distinguish its updates.
- RSDS PDB path `I:\Foam352\DAT\Game\ROMWinMemPC12\Game.pdb`, age 10,
  GUID raw bytes `abc1b685d0257c45a997185b30cc292f`.
- An embedded `.NETFramework,Version=v4.0` string and shader compiler string
  `Microsoft (R) HLSL Shader Compiler 9.27.952.3022`. String matches alone do
  not establish the full SDK/dependency build recipe.

The linker version, runtime imports and component evidence point to the VS2013
toolchain family. Exact compiler/linker versions, flags and library builds are
still unverified. No `Game.pdb`, `cl.exe` or `link.exe` was found in the inspected
project/game directories. This is a scoped local search, not proof that copies
cannot exist elsewhere. A PDB is a game build artifact, not a Fedora package.

The original PE entrypoint at RVA `0x4FD6D0` is a six-byte indirect jump through
the import-table slot at VA `0x0092D690`, resolved to `mscoree!_CorExeMain`.
`native/exe_entry.S` reconstructs this instruction from assembly source:

```sh
make -C Decomp test-entry GAME='/path/to/original/Game.exe'
```

GNU `as --32` compiles the source, `objcopy` extracts the generated instruction,
and the hash-gated checker compares it to those six original bytes. No original
code bytes are inserted into the assembly source. This verifies only the PE
entry instruction, not CRT startup, CLR behavior, a native function body or a
source-built executable. The report explicitly excludes whole-EXE equivalence.

The recovered startup now includes the CRT entry wrapper, security-cookie
initializer, common startup body, local exception filter/handler, SEH frame
helpers, scope table, the game's application entry, startup object helpers,
message pump, worker lifecycle, request handoff, dispatcher, cleanup, display
requests, window handling and cookie verification.
All 30,382 code/padding bytes and 140 data bytes rebuild from
assembly source and match exactly in the development container. See [startup recovery](native/STARTUP.md)
for the original behavior, scope, addresses and reproduction command.

## Required recovery work

The [symbolic assembly drafter](DRAFTING.md) reduces manual transcription for
reviewed native ranges; it never grants coverage credit to unreviewed output.

For an automatically checked coverage report, run in `development`:

```sh
make -C Decomp OUT=../build/decomp-container coverage -j2
```

This builds/tests the registered fragments and writes
`build/decomp-container/coverage.json`. The C++ checker reads the current binary
artifacts, rechecks each entire span against the hash-validated EXE, rejects
overlapping ranges/duplicate block names, and calculates the percentage against
all 8,635,392 original bytes. Missing, shortened or mismatching spans earn no
credit. It does not total old comparison reports or claim a complete PE rebuild.
Regression tests cover overlap, duplicates, out-of-bounds spans, missing output,
wrong lengths, byte mismatches and the distinction between coverage and a full
source rebuild. Adding a registered block still requires reviewed source and a
build rule; raw extracted bytes are not reconstruction evidence.

The supplied [Ghidra workflow](GHIDRA.md) provides native decompilation and
call references alongside ILSpy's managed-code research.

1. Identify and reproduce the original mixed managed/native x86 toolchain,
   libraries, compiler/linker options, metadata ordering and PE layout. The
   existing CLR diagnostic host is not that build toolchain.
2. Recover buildable original code and data while preserving tokens, exception
   regions, calling conventions and native function boundaries. Keep decompiler
   uncertainty explicit and consult original IL/machine code.
3. Compile that reconstruction and use the strict comparator iteratively. Retain
   reproducible build provenance. Copied binary sections do not count as recovered
   source, even if they make a comparison pass.
4. Derive platform changes in the Linux port from the recovered baseline. Continue
   behavioral tests there, but label their narrower guarantees accurately.

No whole-program byte-equivalence or 100% matching completion is claimed today.
