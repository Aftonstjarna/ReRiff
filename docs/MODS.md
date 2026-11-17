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
