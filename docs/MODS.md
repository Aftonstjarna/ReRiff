# Additional VFS archives

## Loose packages and automatic overflow merge (development)

The working development loader also accepts an `assets/` directory inside each
immediate mod folder. Distribute one ZIP containing these loose assets; extract
it into `Riff Racer/mods/`. Do not include generated caches in downloads.

```text
Riff Racer/mods/
  my-mod/
    assets/
      graphics/vehicles/decals/alf_01/batchtextures.brres
    .reriff-cache-<hash>.dat
    .reriff-cache-<hash>.zip
    .reriff-cache-<hash>.receipt
  another-mod/
    assets/...
  .reriff-merged-<hash>.dat
  .reriff-merged-<hash>.zip
  .reriff-merged-<hash>.receipt
  .disabled/
```

ReRiff builds each cache **inside its owning mod folder**, then passes it to the
original native archive loader. Source files stay unchanged. Content hashes
invalidate caches after additions, edits or removals; unchanged caches are
verified and reused. No Python installation is needed to load these mods.
Use lowercase ASCII native resource paths; links and file/directory conflicts
are rejected. A mod must use either `assets/` or `vfs.dat`/`vfs.zip`, not both.
Keep licenses and documentation beside `assets/`, outside the native asset tree.

When active named mods exceed the available native slots, the later folders are
combined into a cached archive. **The overflow merge resides directly in
`mods/`, never in a subfolder.** Earlier mods keep their individual slots and
the later-folder-wins order is preserved within the merge. Hidden generated
filenames are not rediscovered as mods. Disabled folders never enter the merge.
Legacy named VFS mods can participate alongside loose mods.

Already mounted and numbered volumes still reserve their IDs. At least one
free slot is required; the loader does not replace an already mounted archive.
Native per-archive limits still apply (16,382 files and 16-bit tree/string-pool
bounds), so merging does not make archive capacity unlimited. A preparation
failure is logged and falls back to legacy discovery; loose mods are then not
loaded. ZIP64 and encrypted archives are unsupported.

Caches are disposable. Close the game before removing `.reriff-cache-*` or
`.reriff-merged-*` files; the next start rebuilds what is needed. Older cache
versions are retained rather than deleting potentially open files. Existing
release installers' bundled font/artwork formats are unchanged by this work.

For mod authors, package a loose source asset tree with:

```sh
python scripts/package-loose-mod.py /path/to/assets my-mod.zip --name my-mod
```

The ZIP contains `my-mod/assets/...`, with no nested archive. Include the mod's
license beside `assets/` before distributing it. This format requires the new
loader; older releases require a prebuilt VFS pair.

## Existing VFS packages

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
and ownership record. Disabling a language retains its files under
`mods/.disabled/`. Updating a font
pack leaves the other pack's archive unchanged. The installer splits older
shared font archives after verifying their ownership hashes; changed or
conflicting files are retained and reported instead of overwritten.

ReRiff discovers immediate mod folders in ordinal name order and assigns unused
native volume IDs at runtime. No numbering or rewriting the archive contents is
needed. Resource paths inside each archive remain the game's original lookup
paths. The game's original index parsing, ZIP opening, checksums and resource
lookup handle the archives.

For the existing VFS format, each folder must contain exactly one `vfs.dat`/`vfs.zip` pair. Linked folders and
files are skipped. Deeper nested mod folders are not discovered. The original
engine supports IDs 1–31 for extra volumes; existing numbered volumes reserve
their IDs before named folders are assigned the remaining slots. Released loaders without the development merge support skip all named folders
when there are more than 31, or skip later folders when remaining slots run out.

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
must agree with the ZIP. Released loaders require an archive pair; the development loose format above
builds that pair automatically.

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

## Disabled mods and languages

Installers create `mods/.disabled/`. Move a mod folder into it to disable that
mod, and back into `mods/` to enable it. The loader skips this directory and
its contents; disabled archives use no volume slots.

The launcher’s Languages checkboxes move installed CJK and Latin/European
packs between `mods/` and `mods/.disabled/`. Re-enabling reuses the retained
files. Restart the game after changing mods or languages. Modified or
conflicting font files are preserved and reported rather than overwritten.
