# First Prestige native achievement-list observation

Original Windows depot351991/build1687655 Game.exe SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`,
8,635,392 bytes. Consulted the recovery project's native UI notes and existing
`PRESTIGE-NATIVE-BOUNDARIES.md` and `CJK-FONT-INTEGRATION.md` research. The
planned First Prestige row is a ReRiff extension, not an original achievement
or matching reconstruction. No original archive or executable is edited.

## Original list builder

`gaDriverProfileMenu.setupAchievementsList`, MethodDef0x060013EA,
RVA0x325BBC, 12-byte fat header, 2095 CIL bytes, SHA-256
`8e6d74034b695f1ec1cc3dfb9a2fd0fd785a9f342de0df10f1aa73cf37777354`.
At CIL+0x1B3 the row loop skips index18 with exact bytes `071f123b1d050000`.
The title and description keys use index+1. The list contains38 rows.
At+0x5ED..+0x5F4 the builder stores row SSPs in the Profile vector at+472.
Each row uses the original reel at+420. The original 1400-wide layout, row
spacing, scale calculation, scrollbar and ownership/release calls should be
retained when adding the hidden row.

Each row adds title, description, separator and icon in that order. Locked
rows also add the native lock artwork. The icon ID is index+1+129 when earned,
index+1+168 otherwise; index18 therefore uses sprite148 or187. Original
`elUI.createSprite` at RVA0x22030 checks UI+16 as an integer count and indexes
100-byte descriptors from the pointer at UI+12. The first16 descriptor bytes
are two resource/texture token DWORDs then float width/height, as read by the
original sprite constructor. The native input handler remains separate at
MethodDef0x060013ED / RVA0x32502C and must be inspected before changing actions.

## Read-only probe and pending work

The opt-in probe runs after the original Profile tick, only for an initialized
isolated Singleplayer profile with engine-menu logging and progression tracing
enabled. It snapshots the38-row vector, three representative row rectangles
and four adjacent/unused sprite descriptors. Reads use ReadProcessMemory and
bounded pointer/count checks. It calls no asset loader, does not enable the
hidden row and does not modify achievement, Steam, save or UI state.

The provided logo source is `/var/home/afton/.dev/ReRiff/docs/images/reriff-logo-re.png`.
It has been visually inspected but is not yet installed as native artwork.
A live Achievements visit is required to confirm the initialized descriptor
and row layouts before a native row/texture extension. The earlier font work
found loose texture resources were not discovered; a logo path must not assume
that copying a PNG makes it available to the original native texture loader.

## Isolated probe checks

The native probe test passed under Wine, confirming unchanged row/descriptor
buffers and refusal of malformed counts, end/capacity bounds, null pointers
and short output buffers. A local native DLL was installed after verifying
the prior DLL/helper hashes. Save and configuration hashes were unchanged.
No hidden-row branch or resource artwork has been enabled yet.

## Live validation, 2026-11-15 UTC

The user restarted isolated Singleplayer and opened Achievements. Generation28
logged exactly38 native rows and259 sprite descriptors. Sample rows17/18/37
had4/5/5 owned children respectively; the locked rows include the extra lock.
At the observed native scale, row width was1312.50 and height144.38, with
consecutive row centres2866.88 and3011.25. This confirms the original reel's
spacing rather than a replacement layout.

Sprites147/148/149 use resource token10 and texture token26, with64x64 logical
dimensions. The skipped index18's earned sprite148 uses UV bounds
(0.25,0.25)-(0.375,0.375); its locked sprite187 uses the same bounds and token27.
These are runtime token IDs for this session, not stable asset names.
The100-byte descriptors were captured for exact comparison in local test
evidence; no descriptor was changed.

Original handleScreenInput CIL+0xAD..+0xD6 treats action IDs300 through1999 as
Profile_Achievements analytics and returns. The additional39th native row
would use338, inside that original range; it does not enter either song
selection branch. Its other constants300 at+0x18F/+0x39B are race-config
field offsets, not achievement-count limits. No input-handler patch is needed
for that action ID. This inspection alone does not validate restoring the
row or loading custom artwork. Eligibility remains the original38 visible
achievements, excluding the ReRiff First Prestige flag18.

## Temporary artwork implementation

The user requested reusing original achievement artwork until a custom icon
is ready. Original elUIItemSprite.setDefinition RVA0x48830 takes this in ECX
and one stack DWORD (RET4). It selects the UI singleton's100-byte descriptor
by ID and refreshes the existing owned visual, retaining the item's geometry.
ReRiff already uses this method for native toggle artwork. The First Prestige
row now reuses IDs147/186 (adjacent earned/locked icons) through that setter;
no new sprite, texture, archive or game allocation is required.

The intentional list extension replaces the reviewed8-byte storage-slot18
skip with8 CIL nops after isolated Singleplayer initialization, before Profile
construction/JIT. Instruction length, original branch/EH offsets, native list
builder, row ownership, native reel and input handler remain unchanged. The
patch is process-local, exact-byte guarded, and absent from multiplayer
processes. The existing Credits eligibility guard accepts these nops only
when this extension installed them; eligibility still ignores slot18.
Native Profile tick changes only the added row's title/description and icon;
the original locked tint/overlay and all native child ownership are retained.
Profile cleanup clears the extension's cached row pointers.

This prototype has not yet received an in-game appearance/scrolling check.

The candidate passes six isolated native regression executables: entry guards,
row descriptor observation, Profile display, star/cleanup, prestige prompt,
and save-domain routing. The row test covers locked/earned descriptor IDs,
repeat calls, friend/domain guards, ownership and the original38 eligibility.
Local generation29 was installed with prior DLL and save-bank backups after
checking the experiment DLL/helper and original-save hashes. Save/config
hashes were unchanged by installation. Live visual testing remains pending.

## Local appearance validation

After restarting isolated Singleplayer, the user confirmed that the First
Prestige title, description and borrowed icon appear correctly and that
selection and scrolling work. Generation29's trace confirms39 native rows,
the guarded list extension and the native sprite-definition update. This is
a local Wine/Proton observation; a Windows appearance test remains separate.
No race or new prestige reset was requested for this validation.

## Custom circular artwork candidate

The user supplied docs/images/prestige-reriff.png:64x64 RGB, already square
with the circular frame. Source SHA-256 `510c8fa4472851254915cc10fd98483dc972b9d26e7543411758370877df72b5`.
It is used directly, without cropping away logo strokes or redrawing it.
The build script encodes it as BC2/DXT3 DDS in native TABT v4, resource
`reriff/achievements`, texture `first_prestige`. TABT SHA-256
`31310570626a0802d7947af9b80c5dbf8b1d6a7a52dcbf70b2cbd62dc5a59c4f`. The native logical dimensions remain64x64.

After the native graphics dimension query confirms the resource exists and
is64x64, the candidate copies the neighbouring reviewed descriptor layout
into only unused slots148/187, replacing its resource/texture tokens and all
five UV rectangles with full-texture coordinates. Original native sprite
setDefinition refreshes the existing earned/locked row icon; tint and lock
overlay remain native. Missing artwork retains borrowed icons147/186.
The descriptor test verifies unchanged source data, full UVs, dimensions and
refusal of invalid dimensions/tokens. Original Game.exe and VFS remain untouched.

The small core-art volume is independent of optional fonts. Local testing
uses a free additional native VFS slot; release installer ownership/packaging
is still pending and must be completed before publishing this asset.

## Circular transparency correction

The generation30 screenshot confirmed that the resource loaded, but showed
an opaque black square around the circular badge. The supplied PNG is RGB,
so converting it to RGBA alone assigned opaque alpha to all four corners.
The texture import now clips only alpha to a centred64px circle, with a
one-pixel antialiased rim. The original source PNG and logo RGB are retained.
An encoded-DDS regression checks fully transparent corner samples, opaque
centre, visible rim, deterministic output and unchanged source SHA-256.
The corrected archive passes the original native CRC validator. Local
generation31 replaces only the owned artwork pair and its ownership record;
the native DLL, save/checkpoint and configuration hashes were unchanged.
The corrected in-game circular appearance is awaiting user validation.

## Artwork design deferred; original icon export

The user confirmed circular rendering works but rejected the custom design
and requested returning to the placeholder. Generation32 disables custom
artwork by default, restores native icons147/186 and removes only the exact
hash-owned custom test VFS pair. Source artwork, importer and research remain
available; saves and configurations are unchanged.

At the user's request, original Stuck in a Loop artwork was exported from
the unmodified depot archive: graphics/fe/menus/retina/batchTextures.brres,
TABT v4 member achievement_colour,512x512 DDS. Sprite147's reviewed UVs
(0.125,0.25)-(0.25,0.375) identify native pixels(64,128)-(128,192). Export
decodes and copies that64x64 RGBA rectangle to PNG without redrawing or
resizing it, preserving native alpha. Provenance is also embedded in PNG
text metadata. The editable export is docs/images/stuck-in-a-loop-original.png
in the root workspace, separate from the original source archive.

## Edited source reinstalled

The user supplied a revised64x64 RGBA icon, SHA-256
`f53247ad8ad0e614c483f7b8d244ad36f16424cd0e4d40f79b0f0e554ccaa3b7`.
Generation33 re-enables the custom resource using that source unchanged.
The importer retains authored alpha instead of applying the legacy RGB clip.
A decoded-DDS comparison exposed incorrect partial alpha from the Pillow
exporter, so the importer explicitly packs the BC2 block's16 four-bit alpha
values while keeping its RGB blocks. Every output alpha sample is checked
against the source with BC2 quantization tolerance. The separate legacy RGB
clip fixture still passes, and the original native archive validator accepts
the resulting volume. The original native renderer/descriptor path is
unchanged. Local appearance testing is pending; saves/config hashes were
unchanged by installation.
