# Release package metadata

`release.json` schema 2 identifies downloads explicitly. The client updater
selects exactly one record with `component: client`, the target platform, and
the architecture of that package. It never identifies schema-2 packages by a
filename suffix or their order in GitHub's asset list.

```json
{
  "schema": 2,
  "version": "1.4.0-beta.3",
  "commit": "<source commit>",
  "artifacts": ["ReRiff-1.4.0-beta.3-windows.zip"],
  "packages": [
    {
      "component": "client",
      "platform": "windows",
      "architecture": "x86",
      "filename": "ReRiff-1.4.0-beta.3-windows.zip",
      "size": 123456,
      "sha256": "<64 hexadecimal characters>"
    }
  ]
}
```

The example is abbreviated. The publisher emits records for both client
packages, both optional server packages, the two font downloads and source.
Each size and SHA-256 comes from the final archive, not an extracted file.
Windows client packages target x86, including on 64-bit Windows; the native
Linux client helper and both server packages target x86_64. Font and source
packages use `any` for platform and architecture.

`packaging/release_metadata.py` defines package roles at build time and refuses
to publish missing, empty or linked inputs. Both the launcher and installer
preflight validate the selected record. The launcher cross-checks its SHA-256
against `SHA256SUMS` and verifies the downloaded size and contents. Preflight
binds the archive's filename, size, checksum, platform and architecture to the
selected client record before loading installer code.

## Compatibility

The existing `version`, `commit`, `artifacts` and `SHA256SUMS` remain available
for released launchers. Windows server archives retain their distinct
`ReRiff-Windows-Server.zip` name so old suffix-based clients cannot mistake
them for client packages.

An updater receiving older metadata without `packages` accepts the legacy
artifact list (schema absent or 1). Schema 2 requires exactly one compatible
client record. An unknown schema, malformed package, unsupported architecture,
missing match or duplicate match stops the update. It must not silently fall
back to filename matching when explicit metadata is present.

The release and launcher workflows test publisher output against the actual
native selector and exercise legacy metadata plus installer preflight failures.
Adding an architecture requires a deliberate selector/publisher update; it is
not inferred from a new asset's name.

The compatibility fixture `NativeLauncher/fixtures/release_channel-v1.3.2.h`
is the unmodified file from tag v1.3.2 (commit
`af1a9e36f17ab72859751a1f5a64f1c3146bcc14`), SHA-256
`73dd0f1694c262e6b45d651e539a8c64604e8298b35148b16a34decc0343773c`.
Tests check that hash, compile the original parser separately, and feed it the
new publisher metadata for both platforms. This is parser compatibility
coverage; full historical binary installation testing is separate.
