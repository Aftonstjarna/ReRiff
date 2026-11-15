# ReRiff artwork

These are the canonical supplied assets. Build scripts read them without changing
source pixels; screenshots and documentation illustrations remain in `docs/images`.

- `reriff-icon-*x*.png`: authored application icon sizes, embedded together in the
  Windows launcher/setup icon resource. Linux setup uses the 64-pixel icon.
- `reriff-logo.png`: launcher/setup wordmark. Build exports crop transparent
  padding and preserve its aspect ratio. The launcher places it at bottom left,
  in a native button matching the action row; clicking it opens the repository.
  Setup uses its header.
- `reriff-prestige-achievement.png`: Re:Start achievement artwork. The client
  installer includes and installs its native texture automatically for the first
  game launch, in a separate, owned VFS pair. No checkbox or download is needed. Optional
  language packs continue to share their own font VFS pair.

Setup stages artwork with the other client files, verifies hashes, and records
ownership in `localserver/artwork/core.json`. It reuses its owned volume on an
upgrade, adopts an identical prototype pair, or selects a free additional volume.
It refuses modified/missing owned files and never replaces an unrelated archive
or the original `vfs.dat` / `vfs.zip`. Original game artwork remains the fallback
if the custom texture is unavailable.
