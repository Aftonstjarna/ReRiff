# Additional VFS archives

Keep the original `vfs.dat` and `vfs.zip` beside `Game.exe`. Starting with 1.4.0-beta.11, each mod can have its own immediate subfolder of `mods/`
with an unnumbered archive pair:

```text
Riff Racer/
  Game.exe
  vfs.dat
  vfs.zip
  mods/
    reriff-assets/
      vfs.dat
      vfs.zip
    noto-cjk-font/
      vfs.dat
      vfs.zip
      noto-cjk/
        OFL.txt
        jp.rrcf
        jp64.rrcf
        jp96.rrcf
        ...
    noto-latin-font/
      vfs.dat
      vfs.zip
      noto-latin/
        OFL.txt
        latin32.rrcf
        latin64.rrcf
        latin96.rrcf
```

Bundled ReRiff artwork installs in `mods/reriff-assets/`. Updates reuse that
folder and migrate hash-verified older numbered artwork pairs. The stock
`vfs.dat` and `vfs.zip` remain untouched.

Starting with beta.12: if the entire `mods/` folder was removed,
setup recreates it and reinstalls the font packs selected by the existing
checkboxes. Stale named-font records for unselected packs are cleared; bundled
artwork is restored. Partial or modified installations still require integrity
checks and are not silently overwritten.

CJK and Latin install independently. Each has its own textures, metrics, license
and ownership record. Disabling a language retains its files. Updating a font
pack leaves the other pack's archive unchanged. The installer splits older
shared font archives after verifying their ownership hashes; changed or
conflicting files are retained and reported instead of overwritten.

ReRiff discovers immediate mod folders in ordinal name order and assigns unused
native volume IDs at runtime. No numbering or rewriting the archive contents is
needed. Resource paths inside each archive remain the game's original lookup
paths. The game's original index parsing, ZIP opening, checksums and resource
lookup handle the archives.

Each folder must contain exactly one `vfs.dat`/`vfs.zip` pair. Linked folders and
files are skipped. Deeper nested mod folders are not discovered. The original
engine supports IDs 1–31 for extra volumes; existing numbered volumes reserve
their IDs before named folders are assigned the remaining slots. When there are
more than 31 eligible named folders, none of those folders are loaded. If the
remaining slots run out, later folders are skipped and logged.

Legacy numbered pairs in `mods/` are still supported. The `.reriff-vfs` marker
records that numbered component files have moved there, so keep it when using
that older layout. Stock archives stay at the game root.

This support is not in beta.10 or earlier. Older loaders will not discover these
folders; retain the migration backup when reverting. Native mount tests do not
replace a live rendering check in the game.

Native evidence and test scope: [mods VFS research](../reports/MODS-VFS-DIRECTORY.md).

## Replacing original assets

Named mods already override duplicate stock resource paths in beta.11 and
beta.12. Put a valid native index/archive pair in `mods/<your-mod>/vfs.dat` and
`vfs.zip`, containing the original resource path. No extra override flag or
manifest is needed. For example, ALF livery 1 uses
`graphics/vehicles/decals/alf_01/batchtextures.brres` in the native lookup.
Use the existing archive writer's lowercase resource names; its native index
must agree with the ZIP. An archive pair is required, not just loose textures.

The original secondary loader inserts archives by descending volume ID, and
lookup returns the first match. Named folders receive increasing free IDs in
ordinal folder-name order, so the **later folder wins** when named mods contain
the same path. Numbered archives retain their native ID priority; a higher-ID
legacy archive can beat a named mod. Paths absent from higher-priority archives
fall back to lower-priority archives, including stock volume 0.

Fully restart after adding, changing or removing an asset mod. Already loaded
textures are not hot-reloaded. Stock archives remain untouched. Native read
regressions verify the actual original ALF resource is replaced and original
bytes remain available without the test mods; garage/race rendering and decal
shader semantics still require visual validation.

Development fix: discovery now refreshes the file-manager pointer at scan time,
so hooks installed before manager creation reserve already mounted IDs correctly.
This startup timing fix is not yet included in beta.12.

See [override validation](../reports/MODS-VFS-OVERRIDES.md) for evidence and
reproduction instructions. Earlier research described the secondary loader as
appending records; that description was incorrect.
