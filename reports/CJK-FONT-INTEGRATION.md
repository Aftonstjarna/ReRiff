# Native font archive integration research

Research evidence, not a byte-matching reconstruction or completed CJK patch.

## Original reference and provenance

- Steam depot 351991, build 1687655, from the configured depot reference.
- `Game.exe`: SHA-256
  `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
- Original `vfs.dat`: SHA-256
  `62d1beb2cc5536f42952af06e6bbb54f8cbf3f9655bad55092b402d544e7f6ee`.
- Original `vfs.zip`: SHA-256
  `f0904bb0198f81bef4c4520c3ad15d6e3ba2dc3a7d69ebbce6d3ebf715908a1d`.
- Addresses below are preferred virtual addresses (image base `0x400000`).
  Findings compare original instructions and data with isolated execution;
  decompiler prototypes alone are insufficient evidence for the native ABI.

## Native archive validation

At `0x560CB0`, the original validator takes one stack argument pointing to a
record, returns a byte result, and cleans four stack bytes. The record contains
the native ZIP context at offset zero and the expected checksum at `+0x10`.
The loop reads each ZIP member CRC32, adds it to EBX at `0x560D2B`, and compares
the modulo-32-bit total against the expected value at `0x560D79`.

The original archive's total is `0x2CD14455`. This is the sum of member CRC32s,
not a CRC32 of the entire ZIP and not the XOR of the member CRC32s.

The index loader is at `0x560780`. Original index layout:

- Five little-endian DWORDs: magic `0x0300FFEE`, node count, file count,
  string-pool byte count, member-CRC32 sum.
- Root record: four WORDs, then node-count records of four WORDs each.
- Node fields: name offset, next sibling index, first child index, flags.
  `0xFFFF` denotes no index; flags `0x3FFF` denote a directory; file nodes
  otherwise contain their file-table index.
- File records are seven DWORDs: ZIP source value 2, compression method,
  compressed size, uncompressed size, filename byte length, local-header
  offset, and central-directory offset relative to that directory's start.
- NUL-terminated ASCII component string pool follows the file table.

All 903 original nodes and 686 file records were checked against the original
ZIP. The original string pool is 8790 bytes and complete index 35250 bytes.

At `0x55E130`, discovery enumerates `vfs*.dat`, parses a nonempty suffix as
hexadecimal, and forms the corresponding `.zip` filename. The base archive
has a special identifier. Additional numbered volumes permit an owned font
archive without modifying the original pair. The current writer restricts
additional identifiers to 1 through 31; collision/ownership handling is still
required before general installer integration.

## Failed probe and correction

An earlier local probe added a resource to the original pair while retaining
the old checksum field. The game reported `vfs.dat is corrupt`. Both original
files were restored from backups and both SHA-256 values above rechecked.
That mutation approach is abandoned. New probes use a separate owned volume.

`ExperimentalMenu/test_native_archive.c` maps the hash-checked original EXE,
resolves the kernel/CRT imports it uses, and executes the original ZIP-open
function at `0x598420` and checksum validator at `0x560CB0` in an isolated Wine
prefix. ZIP-open receives its filename in ECX. This test accepts the generated
probe ZIP's checksum and rejects the same record with one checksum bit changed.
This validates the checksum, not the full mount/render lifecycle.

Wine's `DONT_RESOLVE_DLL_REFERENCES` mapping left native absolute references
unrelocated in this test. The harness applies the original HIGHLOW relocation
table to its private mapping when the guarded operand confirms this condition.
No original file is changed by the harness. Its loader setup is test-only.

`scripts/test_native_archive.py` independently walks generated indexes and checks
all paths, file metadata, CRC totals, deterministic output, and refusal of the
base volume and unsupported resource paths. Three test cases pass.

## Native texture path and outstanding work

The texture-dimension method at `0x494D60` constructs the resource path
`graphics/<token>/batchTextures.brres`. An opt-in probe requests
`fonts/reriff/probe` / `reriff_probe` through this original method. A loose
resource file returned not-found. The numbered-volume probe subsequently
reached the main menu in the user's Linux/Proton test. The log reports
`found=1 width=4 height=4`, matching the owned texture. Private log snapshot
SHA-256 is
`ce5ab02a36459f2a61779c522885235fc7f68276a1c6396dcacb0acd215788e0`.
This verifies native resource discovery for that probe, not glyph rendering.

Noto CJK source is pinned in `packaging/fonts/noto-cjk.json` at commit
`f8d157532fbfaeda587e826d4cd5b21a49186f7c`. The original OFL license is retained.
The regular OTC has ten faces: five proportional and five mono regional
variants (JP, KR, SC, TC, HK). Each face's Unicode cmap covers 44810 code points,
including 42220 BMP and 2590 supplementary code points. The original glyph
lookup consumes one UTF-16 WORD, so supplementary support requires additional
work; font coverage alone does not establish game-wide character support.

## Intentional ReRiff asset generation

`scripts/build-cjk-atlas.py` generates owned textures for all five proportional
regional faces using isolated build-only Pillow/fontTools dependencies. These
tools are not client dependencies. Each atlas is 8192 by 8192, encoded as DXT3
in a TABT v4 container. The current five-face archive is approximately 52 MiB
compressed; each decoded GPU texture would occupy 64 MiB before other native
resource costs. Loading every regional variant simultaneously is not intended.
The user subsequently reached the main menu with the full-size resource probe.
Native dimension lookup reports `found=1 width=8192 height=8192` for
`fonts/reriff/cjk/jp`. This verifies discovery/dimensions, not glyph drawing.

Owned `RRCF` v1 metadata retains sorted 32-bit Unicode codepoints and seven
floats per record: x, y, width, height, bearing X, top-relative bearing Y, advance.
Its eight DWORD header contains magic, version, count, texture width, texture
height, nominal pixel size, ascent, descent. This is a new build artifact format,
not an original game format. It deliberately retains supplementary codepoints
for a future pair-aware integration instead of truncating them to native WORDs.

An independently decoded small DDS sample visibly contains the Japanese trace
characters. `scripts/test_cjk_atlas.py` checks all five regional faces using
Japanese, Korean, Chinese and a supplementary ideograph: encoded texture bounds,
metrics/advance agreement with the source, nonempty glyph alpha, and preservation
of full codepoints. The native renderer integration is still incomplete.

## Native fallback prototype (opt-in, not release-ready)

The original font constructor at `0x4551B0` consumes a vector of 36-byte raw
glyph records and creates 44-byte records. Isolated execution with real CRT
allocation and a controlled graphics-interface slot verifies UV normalization,
width/height normalization, retained pixel bearings/advance, default-glyph
selection, and the destructor at `0x455090`. The harness also executes original
string-width code through the preparation trampoline and verifies combined
advance. This is execution evidence, not reconstructed matching-byte credit.

Original text construction at `0x450340` tests font `+0x68` at `0x450568` and
allocates a second native `elVisual2d` only when a fallback resource exists.
Attaching a font only inside `generateVisual` is too late for a new control.
This constructor also receives positioning state in XMM2/XMM3; preparation
preserves integer registers, flags, and full x87/SSE state. Its guarded original
five-byte prologue is `55 8B EC 6A FF`. A controlled constructor-signature stub
checks stack forwarding.

The opt-in `diagnostics.cjk_render_probe=1` experiment currently requires the
Unicode trace hooks. It prepares a Japanese-face fallback before native text
construction and original string-width measurement (`0x456450`, guarded
prologue `55 8B EC 51 53`). It keeps primary glyphs and declines fonts already
using another fallback. Metadata uses a loose owned `fonts/noto-cjk/jp.rrcf`
file. Token construction, game allocation, font construction, and native text
visual creation remain in use.

This first experiment includes BMP glyphs only, scales metrics against each
primary font's M cap height/baseline, and bounds its retained native fallback
cache to sixteen metric combinations for the process. It does not yet provide
a production cache release/reset policy, supplementary-pair handling, regional
selection, missing-character performance validation, or a fallback for direct
drawing calls with no alternate visual. A visible in-game glyph test is still
required. Do not describe this as game-wide CJK support or enable it by default
in a release.

### Consecutive fallback glyph positioning

The first visible prototype rendered language glyphs, but successive characters
drifted horizontally and vertically. Original `generateVisual` builds a chained
glyph list. At `0x455881`, its fallback path replaces the previously calculated
relative Y with the glyph's unadjusted bearing (`8B 41 20 89 47 18`). At
`0x4558EC`, the fallback flag comparison (`80 7C 24 0E 00`) selects accumulated
pen X instead of the ordinary chain-relative pen update. These rules explain
the observed repeated-fallback staircase; they are original behavior, not an
atlas-coordinate failure.

The intentional prototype correction recognizes glyph-record pointers inside
its bounded owned fallback cache. Only those records retain the ordinary
relative Y and take the normal chain-relative pen update. Original alternate
texture selection, including the UV X tag of 2.0, remains intact. Foreign
fallback records execute the exact displaced original instructions. Complete
hook installation is required before rendering can activate.

The isolated harness copies relocated original blocks `0x4557FB..0x455887` and
`0x4558EC..0x455949` into executable test memory, preserving their ESP-relative
locals. Tests execute them before/after the conditional hooks: foreign fallback
node/pen outputs remain equal, a mixed Latin/owned-glyph run has the expected
positions and width, and a 60-glyph run advances once per glyph with each
baseline determined by its bearing. This checks actual original instruction
behavior. The corrected in-game visual result is still awaiting a user test.

### Mixed-script split-stage regression

The user retest showed that the preceding positioning correction helped some
folders, while mixed Latin/fallback strings still shifted vertically or
overlapped. The earlier 60-glyph test covered the first combined chain only;
it did not establish preservation through native separation into two visuals.

The original split core is `0x455F43..0x456232`. It computes world positions,
uses UV X >= 2.0 to identify alternate-font nodes, and removes those nodes from
the primary chain. At `0x456001`, absolute nodes propagate their position to the
next node before removal. The preceding correction creates ordinary relative
language glyph nodes, so the original extraction assumptions no longer hold at
font transitions. This explains why a pre-split-only fixture passed while the
in-game mixed strings failed.

The additional intentional hook at `0x455FDF` guards the displaced bytes
`F3 0F 10 46 04`. When alternate visual resource/texture tokens (`+0x18/+0x1C`)
match the owned fallback font (`+8/+0x0C`), it canonicalizes only an alternate
glyph to its already computed world X/Y (XMM4/XMM3). Native removal then
propagates that position only if the following node is still relative. An
already absolute node is retained without adding another absolute position.
Existing native list operations and alternate texture selection remain in use;
other resources take the displaced original path.

The extended harness executes the actual relocated split core in the mapped
original EXE, with controlled native glyph objects constructed at `0x41EE00`
and real list updates at `0x41EEF0`. Fixture-owned references prevent stack
objects from reaching native deletion. It reproduces incorrect mixed positions
without the added split hook, then checks both resulting glyph lists after the
hook. Mixed text, all-fallback text, leading-fallback text, all-primary text,
and a pre-positioned subsequent node all preserve expected positions and list
membership. The test stops before final vertical alignment and GPU drawing;
it is not a full-renderer or production lifecycle proof. Visible user retesting
of the new correction remains required.

The user subsequently confirmed normal spacing/baselines for mixed Latin and
Unicode punctuation, all-Japanese folders, and Japanese filenames with a Latin
extension. The corresponding private Unicode trace has 28 glyph records, all
matching their requested codepoints, and no refused hooks. Snapshot SHA-256:
`aa241e22ccda34d76d0b7c56850412f5c760905518f9f5c9a766c0e8a65df7dd`.
This is a Linux/Proton visual confirmation of those cases, not Windows or
game-wide coverage.

### Native text colour propagation

The following test revealed white Noto glyphs inside an otherwise cyan selected
filename. Native text vtable `0x9FA270`, slot 4, points to `0x450C40`. Its
seven-byte entry is `55 8B EC 56 FF 75 08`. The method calls base item colour
update `0x448500`, then (unless item state at `+0x10` is 3) visual colour update
`0x442F50` for the primary visual at `+0x1C4`. It does not update the alternate
visual at `+0x1C8`. This differs from a missing glyph or texture problem.

Native visual colour update stores four floats at `+0x90..+0x9C` and marks
dirty bit `0x100` at `+0x170` when the value changes. The intentional opt-in
extension forwards the original text method first, then calls that same native
visual method for an alternate whose resource/texture tokens identify the owned
Noto font. Other fallback artwork and the original state-3 skip remain intact.
The wrapper preserves post-original flags, integer registers, full x87/SSE
state, and last-error state around the extension.

The original-code fixture first reproduces primary-only tinting, then checks
owned alternate cyan/white/grey-and-alpha updates, the native dirty bit for each
change, foreign alternate exclusion, no-alternate text, and state-3 behavior.
Visible highlight testing of this extension remains pending.

The subsequent Linux/Proton retest confirmed cyan selection and white reset for
the added Japanese glyphs. The startup trace confirms native colour propagation
installed with no refused hooks. Private trace snapshot SHA-256:
`ad18ca0ee47bd7f724094ce912d51273ed7ff097aae6d982f66b341cbfd8da51`.
This closes that visible highlight regression for the tested filename control;
the broader coverage and lifecycle limitations above still apply.

See `UNICODE-TEXT-TRACE.md` for verified UTF-16 layout/glyph evidence and the
separate artist metadata conversion issue. No allocator/lifecycle, atlas,
surrogate-pair, or Windows rendering fix is claimed by this archive research.

## Direct drawing context and native alternate-visual extension

The subsequent live context probe confirms that the affected Japanese
leaderboard/title and pause/NowPlaying layouts supply a non-null primary
visual and a null alternate visual, even after an owned fallback font has
been attached at font offset `+0x68`. The working file picker supplies both
visual arguments. This is a separate coverage gap from metadata encoding.
Original `generateVisual` at VA `0x455520` disables fallback lookup when its
alternate visual argument is null; the trace reproduces U+003F selection.

Verified original APIs and ownership evidence (preferred addresses):

- `elVisual2d` allocation is 380 bytes from the native fixed-size allocator
  at `0x442BD0`, using pool global `0xBBCF74`, with size and stride both 380.
  `elUIItemText` performs the same allocation at `0x4504FB`/`0x450584`.
- Constructor `0x41E3B0` receives `this` in ECX and stack argument 1. Its
  virtual-base table is `0x999EFC`, with reference subobject at `+0x174`,
  vtable `0x999EF8`, and count at `+0x178`. Native SSP code increments this
  count, and destroys only after decrementing it to zero.
- The scalar deleting destructor at `0x41E730` receives **the reference
  subobject**, not the beginning of the visual, in ECX, and stack flag 1.
  It subtracts `0x174`, calls the complete destructor, and returns the
  allocation to the original pool. Its cleanup is `RET 4`.
- `elViewport.addContent` at `0x457BB0` and `removeContent` at `0x457CE0`
  take viewport in ECX and visual on the stack (`RET 4`). The SSP vector is
  at `+0x1B0/+0x1B4/+0x1B8`; adding deduplicates and takes a strong reference.
  Removing releases that reference. Viewport destruction may iterate this
  vector, so a visual destructor must not recursively mutate that vector.
- Original metadata identifies `gaGame.ms_pInstance` at RVA `0x7E9D24`.
  `getViewport` selects from twelve owner slots at **decimal** offset 20;
  the lower viewport is at **decimal** offset 68. `releaseViewports` confirms
  the twelve-slot loop. Reading current owners avoids stale viewport caches.
- Original colour setter `0x442F50` writes RGBA at `+0x90..+0x9C`, setting
  dirty bit `0x100`. `setVisibleCached` at `0x4535E0` is cdecl(visual,bool)
  and updates the visibility cache; it does not write visible byte `+0x14`.
  `setContextDirty` at `0x453580` is cdecl(visual), with an absolute operand
  referencing RVA `0x7FCC0C`; its guard must account for image relocation.
- Visual transform storage starts at `+0x28` and occupies 64 bytes; it is
  not interchangeable with the separately recovered 48-byte `elMatrix`.
  Position is at `+0x68/+0x6C`, scale at `+0x70/+0x74`. Managed native-inline
  setters use dirty bits 1 and 8 respectively. Atlas size starts at `+0x78`,
  UVs at `+0x80`, and must remain owned by each font visual.

The opt-in ReRiff extension `diagnostics.cjk_direct_probe=1` supplies an owned
native alternate visual only for exact original standalone `elVisual2d`
instances with an owned CJK fallback and a non-ASCII layout. It retains a
bounded 64-entry weak-primary/strong-alternate map. Later ASCII redraws reuse
the same alternate argument so the original layout clears stale glyphs.
Existing caller-provided alternates are preserved. It reuses native pooled
construction, viewport add/remove, reference counting and deletion; there is
no separate overlay renderer. Original resource tokens, atlas dimensions,
UVs and child lists remain separate. Existing menu-tick integration refreshes
paired transforms/colour after managed inlined menu assignments.

Validation: the original-instruction harness accepts all six relocated hook
boundaries and the allocator/constructor/child API guards. Native glyph,
mixed-script splitting, highlight/reset and diagnostic ABI tests still pass.
A separate ownership fixture verifies opt-in behavior, foreign/provided
alternate preservation, existing/new viewport attachment, deduplication,
repeated/ASCII redraw routing, atlas isolation, menu refresh, and independent
reference release while a viewport still owns the alternate. That fixture
uses controlled callbacks; it does not prove the full renderer lifecycle.

This is an intentional local prototype, not byte-matching original recovery.
Live leaderboard/pause appearance remains to be checked. Arbitrary visual
parents are refused. Animated race-only text may contain inlined updates
outside menu ticks (`gaTrackManager.tickScrollingText` is one verified case),
so complete game-wide animation synchronization is not established. Concurrent
rendering, device resets, supplementary characters and Windows still need
testing. No metadata conversion or stored-record repair is included.

## Centred pause-title placement

The user confirmed the direct-drawing prototype renders the leaderboard and
pause glyphs with the right colours, but Japanese characters on pause remain
displaced. `gaPauseMenu.onInitScreens` creates title/artist text using native
font 7 with HAlign 1 and VAlign 1. Left-aligned file-picker coverage therefore
does not establish correct placement for this screen.

The original `generateVisual` centre/right alignment branch is
`0x455D3C..0x455ED3`. At `0x455D90`, it loads float 2 from `0xA06618` and
compares this with the node's first U coordinate at `+4`. Coordinates tagged
for the alternate atlas (U >= 2) skip the relative-X accumulation. At
`0x455DD1..0x455DDB`, final node width is multiplied by **the primary font's**
texture width at `+0x10`. These rules do not match the normal chained language
glyphs introduced by the ReRiff fallback extension. The original game's
special alternate artwork remains a distinct case and must retain its rules.

Two scoped ReRiff detours extend that original branch:

- At `0x455D90`, use comparison limit 3 only when the current node is tagged
  for an owned CJK fallback font. Preserve the native branch, accumulation,
  first-node/bearing adjustments and rounding; leave the actual UV tag intact.
- At `0x455DD6`, multiply the final owned fallback glyph width by its own
  font texture width. Ordinary and foreign fallback glyphs still use the
  original primary-font factor. Preserve the original registers and flags;
  only the intended low XMM0 float result changes at this multiplication.

The load guard includes the relocated absolute constant address. Its preferred
bytes are `F3 0F 10 05 18 66 A0 00`; the multiplication's original five bytes
are `F3 0F 59 40 10`. These are evidence/guards, not matching recovery credit
for the intentionally changed behavior.

The isolated original-instruction harness reproduces the displaced alignment
with an 8192-wide alternate atlas and 1024-wide primary atlas. For centre and
right alignment, it compares all-Japanese, mixed-script with Latin endings,
mixed-script with Japanese endings, and all-Latin fixtures with an equivalent
single-atlas run of the **unmodified native alignment algorithm**. It then
executes original visual splitting and original vertical-centre alignment and
compares every glyph's final X/Y position between the two font lists. All eight
comparisons pass with the owned-only extension; unowned fallback output and UV
tags are preserved. This test deliberately retains the original algorithm's
first-node and rounding behavior rather than substituting a new width formula.

Fixture-only corrections were needed for the recovered stack locals and for
an overlapping split-stop/vertical-entry boundary; these do not modify game
files. The Unicode diagnostics harness also now relocates its private original
EXE mapping under Wine before checking absolute-operand guards, as the archive
harness already does. No original executable or archive was rewritten.

The visible pause correction still requires the next in-game check. Justified
wrapping uses a different native branch and is not established by this result;
race-only animation and Windows coverage remain open as described above.

## Higher-detail paged atlas prototype (2026-11-11)

The user subsequently confirmed the pause placement correction. The next
prototype retains the pinned Noto source and the exact 32px layout metrics,
but chooses 32/64/96px raster detail from the native font's visible M height:
up to 32px uses the existing atlas, up to 64px uses 64px glyphs, and larger
text uses 96px glyphs. This is intentional ReRiff behavior, not recovered
original font-quality selection. Native SD/HD font selection remains intact.

All three metric files retain 44,810 source codepoints. The current native
UTF-16 glyph path continues to use BMP entries; supplementary rendering is
not established by retaining those entries. The 64px version uses three
8192-square pages and the 96px version seven. Separate native resources
load pages on demand. Missing HD metrics or insufficient visual-pair slots
select the existing 32px path. Each loaded DXT3 page occupies approximately
64 MiB before native resource overhead; in-game GPU use remains to be measured.

The original font supports one resource/texture per font visual. No native
multi-page-font control has been established. ReRiff therefore extends the
existing glyph-list split after native vertical alignment, at RVA 0x563D9
(preferred bytes `8B 75 08 F3 0F 10 44 24 24`). Extra page visuals use the
original pooled allocator, textured constructor RVA 0x1E7F0, resource setter
0x54F90, subresource setter 0x55010, and child-list setter 0x1EEF0. Setters
have guard bytes `55 8B EC 53 8B 5D 08`. Each page retains its native reference
ownership and its original absolute glyph positions. Routing temporarily
holds each node before detaching and repartitioning its list. Extra visuals
follow native viewport attachment/removal, destruction, RGBA, visibility,
position/scale, rotation at +0xC4, and pivot at +0xD4/+0xD8/+0xDC.

Native glyph generation adds 2 only to Umin; the existing alternate split
subtracts 2 only from Umin. Umax is never tagged. The prototype temporarily
adds another page*2 to Umin in owned font records, preserves those markers
through the owned-only centre/right extension, then removes them after
native splitting. No foreign font or original texture changes this behavior.

Original constructor RVA 0x551B0 computes line height at +0x20 from the
minimum raw top and maximum raw top+height (loop 0x55428..0x55448, final
subtraction/store through 0x554DF). HD raw dimensions describe atlas texels,
so the extension restores both normalized quad dimensions and line height
from the canonical 32px design dimensions. Bearings, advances and baselines
are identical across tiers; higher detail must not change layout.

Validation: source hashes pinned; Python small-page builds cover page
crossings and texture pixels. Full-asset native metric tests compare all
codepoints and canonical fields across tiers, selection thresholds, missing
tier and pair-budget fallback. The original-instruction archive harness
accepts the generated vfs3 index/ZIP checksum and rejects a changed checksum.
Its native centre/right, splitting and vertical-alignment comparisons cover
all seven page markers, all-Japanese, mixed and Latin-only fixtures. Paged
routing tests execute the original native glyph constructor and child setter
with real reference counts; mocked texture loading checks repeated reuse,
empty redraw, resource selection, transforms and recursive release. Native
hook guards, ABI/state preservation, highlight/reset and Auto scaling checks
also pass. Actual GPU loading, sharpness and page rendering still require
an in-game restart and visual check; Windows live coverage remains open.

The executable reference remains the depot Game.exe with SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
This prototype does not modify original Game.exe, vfs.dat or vfs.zip and
claims no matching-byte reconstruction credit.
