# Loading additional VFS pairs from mods

## Original reference and evidence

Original Windows depot 351991, build 1687655, Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Addresses are preferred VAs with image base `0x400000`.
Consulted the RiffRacerDecomp recovery tree and its existing archive research;
no recovered directory/mount implementation was found. Original bytes were
inspected with objdump and a separate Ghidra project, not changed on disk.

- `0x55E130` scans a native wide-string directory passed in ECX into a vector
  passed in EDX. It uses FindFirstFileW/FindNextFileW, skips directories, and
  does not recurse. The vector contains 28-byte entries: DWORD volume ID plus
  a 24-byte MSVC wide string containing the full archive stem.
- Native wide strings have a 16-byte inline buffer/pointer union, length at
  +16, and capacity at +20. Capacity below eight selects the inline buffer.
- Discovery accepts numbered hexadecimal volume suffixes, orders them, and
  uses a special base-volume ID. It is not an arbitrary filename loader.
- `0x55E740` initially scans the filesystem object's path at +0x18 and loads
  the stock archives. At `0x55ECBE` it calls the secondary loader `0x55ED80`.
- The secondary loader calls discovery at `0x55EDE7` (original bytes
  `E8 44 F3 FF FF`), using a different directory string obtained via global
  `0xBFCD08` +0x14. Its original purpose/path is not established here.
- That loader appends to the same mounted archive vector at object +0x48,
  +0x4C and +0x50. Mounted entries are 20 bytes, volume ID at +12.
  It reuses native index loading `0x560780`, ZIP opening `0x598420`, and
  checksum validation `0x560CB0`. The file manager instance is at `0xBFCD2C`.

## Intentional ReRiff extension

`ExperimentalMenu/mods_vfs.h` redirects only the secondary discovery call.
It preserves that original scan, then scans `<game>/mods/` once. When ReRiff
starts after the initial mounts, it invokes the existing secondary loader with
only the mods scan enabled, avoiding a second load of the original directory.
The executable hash guard remains in engine startup; the hook additionally
checks the exact original call and scanner prologue. Game.exe is not patched
on disk. The marker `mods/.reriff-vfs` enables this behavior after migration.

Component ownership records retain logical volume names/IDs and original hashes;
the marker directs their physical lookup into mods. The shared migration planner
is used by both installers and participates in their existing rollback. It
leaves the stock vfs.dat/vfs.zip in place, refuses symlinks, aliases, incomplete
pairs and differing same-name root/mods files. Migration currently accepts IDs
1 through 31, matching ReRiff's component writers; higher-ID third-party packs
are retained with an error rather than silently moved or discarded.

## Validation and limits

An isolated Wine fixture maps the hash-pinned original with imports resolved
and HIGHLOW relocations applied only to its private mapping. Native discovery
returned the expected full mods path and ID. The production hook's late-mount
path then registered a generated archive as one 20-byte native mount entry.
`ExperimentalMenu/test_mods_vfs.c` preserves that executable fixture.
The archive payload is a test byte string, not a rendered texture; this proves
native archive discovery/mounting, not a live game's resource rendering.
Linux installer tests cover dry-run, migration, repeat install, unchanged stock
files, conflict refusal, symlink refusal, incomplete pairs and rollback.
Real-game startup, fonts and achievement artwork still need a live retest.
These are behavioral findings and intentional patches, not byte-matching
reconstruction credit.

The same production-hook fixture also mounted all three installed extra pairs
from staged copies in mods (60 bytes / three native mount records), retaining
their IDs. Windows helper lifecycle and migration-command checks passed after
making command failure exits explicit. The local development installation moved
all three pairs with a rollback backup; executable, stock archives, unrelated
DLLs and settings were hash-checked unchanged. Live rendering remains pending.

## Unnumbered per-mod folders (follow-up feasibility probe)

The user supplied `mods/noto-cjk-font/vfs.dat`, `vfs.zip` and a loose
`noto-cjk/` metrics/license directory. The supplied ZIP matches both installed
CJK and Latin ownership hashes: it is the shared combined-font archive.
At the time of this probe, the production extension scanned only the immediate mods directory and
cannot yet discover this layout. Font metric lookup still uses game/fonts.

An isolated native probe scanned this exact example directory. The original
scanner returned one entry with ID 0 and full stem ending in
`mods/noto-cjk-font/vfs`. The probe changed only that entry's in-memory ID to 1
before invoking the original secondary loader. It successfully created one
20-byte mounted entry with ID 1, including native ZIP/index/checksum handling.
No filename, archive byte, installed loader or ownership record was changed.
This confirms that numeric suffixes are needed by the original discovery policy,
not by the archive file format: ReRiff can assign IDs for unnumbered folders.
It does not validate multi-mod collision handling, deterministic priority,
updated loose-metric paths, updater ownership/migration, or live rendering.
Probe source/output remains in build/mods-research/probe_folder.c/.exe.

## Named font mods (2026-11-16)

The production hook now calls the original scanner for each eligible immediate
mod subfolder, then replaces its emitted unnumbered volume ID 0 with a free
ID in 1–31. Folder names are sorted ordinally; existing mounted and scanned
numbered volumes reserve their IDs. Archive stems retain their full folder
paths. The secondary mount and resource lookup implementations remain original.
This is an intentional ReRiff policy change, not original reconstructed code.

CJK and Latin ownership migration splits the old owned ZIP by the existing
`graphics/fonts/reriff/cjk/` and `graphics/fonts/reriff/latin/` resource prefixes.
It rebuilds each index using the existing verified font-index writer, and moves
loose metrics into the corresponding mod folder. Original game files are not
modified. `test_language_component.py` exercises root, flat and manually moved
legacy layouts, file conflicts, modified-file refusal and failed-write rollback.
`test_font_packs.py` checks actual packages and native ZIP/index records.

The production-hook fixture mounted staged copies of both separated font mods
and the existing artwork volumes successfully: four native mount records (80
bytes), unique IDs 1–4. This uses the hash-pinned original Game.exe mapping and
does not launch the installed game. The local installation has verified hashes
and a rollback receipt in `build/font-mods/install.json`. Live rendering remains
a user test; successful mount/checksum checks alone do not prove rendering.
