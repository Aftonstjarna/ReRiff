# Faster symbolic assembly recovery

`draft_assembly.cpp` is a C++ tool for drafting assembly from an exact, reviewed
native instruction range. It emits mnemonics, address labels and external target
symbols, never `.incbin` or literal machine-code arrays. It preserves common
register-opcode direction choices and LEA padding encodings. This removes much
of the mechanical transcription, but does not infer original C++ source or make
decompiler interpretations authoritative.

Build and test inside `development`:

```sh
make -C Decomp OUT=../build/decomp-container test-draft
```

Example for the reviewed application worker (write to a new output filename):

```sh
objdump -d -Mintel --insn-width=16 \
  --start-address=0x5282e0 --stop-address=0x52892b \
  "$HOME/.local/share/Steam/steamapps/common/Riff Racer/Game.exe" > /tmp/worker.asm
build/decomp-container/draft-assembly /tmp/worker.asm \
  5282e0 52892b original_application_worker /tmp/worker-draft.S
```

The tool requires an instruction-contiguous range and rejects gaps, overlaps,
truncated instructions, undecoded bytes, internal branches into instructions,
and overwriting existing source. The original disassembly's byte fields are
used to check lengths and select encoding forms; they are not embedded as code.
Its output is explicitly labeled UNREVIEWED and earns no coverage credit.

Review each candidate against Ghidra, original disassembly and relevant IL.
Confirm function boundaries, return/exception paths, register arguments and
any embedded data. Ghidra's function extents can exclude padding or shared tails;
a raw contiguous disassembly may accidentally decode data as instructions.
The drafter does not solve those analysis problems. Unsupported encoding choices
may require manual corrections. Inline jump tables must be recovered as data
and labels, not accepted as decoded instructions.

After review, add the source and fixed-address build rule, register its exact
span in `native_blocks.hpp`, then require `native-check` to report zero byte
differences before including it in coverage. `make coverage` rechecks current
artifacts and rejects overlapping registered spans. Full PE layout, relocations,
managed code, resources and original compiler/linker reconstruction are separate
remaining work.

Validation includes a synthetic symbolic roundtrip covering absolute memory,
register directions, direct/indirect calls, branches and padding. Rejection
checks cover existing output, incomplete instructions and listing gaps. The
tool also reproduced the previously verified 533-byte create routine, then
produced reviewed source for the 99-byte size helper and 1,611-byte worker body;
all were separately compiled and compared with the supported original.
