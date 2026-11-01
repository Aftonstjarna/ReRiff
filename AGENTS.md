# Original-game recovery
- Matching-only reconstruction is integrated into `decomp`; reserve `redux` for
  intentional behavior changes.

- EXE reconstruction is now tracked in the separate Git repository
  `/home/afton/.dev/RiffRacer` (branch `decomp`). Continue EXE recovery there;
  preserve this workspace's existing `Decomp` snapshot and Linux-port work.

- Use suitable tools from the explicitly supplied project/tool directories to
  accelerate recovery. Use Ghidra for native analysis and ILSpy for managed IL;
  verify any reconstruction against original bytes regardless of tool output.
- Progress updates must include verified matching bytes divided by 8,635,392
  and the corresponding percentage. Analysis output alone earns no byte credit.

- Continue recovery unattended within the authorized scope. Do not stop for
  routine confirmations or small progress checkpoints; ask the user only for
  an unresolved error, missing input, or a decision that genuinely needs them.

- Use the existing Distrobox container named `development` for development
  commands and builds. Enter with `distrobox enter development -- ...` and work
  in `/var/home/afton/.dev/ReRiff`. Install any needed build packages inside that
  container, not on the Bazzite host. Container verification outputs currently
  use `build/decomp-container`.

- The user requires the original Windows decompilation to start from a 100%
  byte-for-byte matching reconstruction. Treat this as the baseline objective.
- Current priority is Game.exe reconstruction. Leave the user's working DLL
  changes for track generation/sync-server support alone; they are not failures
  of the EXE reconstruction. Defer further Linux port features until this focus changes.
- Keep original reconstruction and the native Linux C++ port distinct. Numerical
  parity on sampled inputs does not establish a matching decompilation.
- Do not claim a matching rebuild without compiling recovered source and comparing
  the entire resulting executable against the supported original, including
  headers, metadata, native code, resources, padding and trailing data. Do not mask
  differences or count copied original bytes as reconstructed source.
- Preserve exact original IL, tokens, native disassembly, data and provenance.
  Record unresolved reconstruction/build/toolchain gaps explicitly.
- Use original code and assets, not invented substitute gameplay or physics.
  Substantive new implementation must be C++/C or Rust; Python only for simple tools.
- Continue to keep driving disabled until the original force/contact integration
  is recovered. Preserve existing uncommitted work.
