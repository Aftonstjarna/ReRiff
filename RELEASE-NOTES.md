# ReRiff 1.4.0-beta.3 — Explicit update package identities

## In-game summary

- Select client updates using explicit package identities.
- Check package size and checksum before installing updates.
- Keep update metadata compatible with older launchers.

## Full release notes

### Client updates

- Publish release metadata schema 2 with a component, platform, architecture,
  filename, byte size and SHA-256 for each client, optional server, font and
  source package. Client updates select exactly one matching client record
  instead of guessing a package's role from its filename suffix.
- Reject missing or ambiguous client records, unsupported metadata schemas,
  wrong architectures, unsafe filenames and malformed sizes or checksums.
  Invalid explicit metadata never falls back to filename matching.
- Cross-check explicit package checksums against SHA256SUMS and verify the
  downloaded file's exact byte size before installer preflight. Windows and
  native Linux preflight bind the archive's identity, size and checksum to the
  same selected record before loading installer code.
- Retain the legacy artifacts list and release checksum file so previously
  released launchers can reach this beta. Preserve support for older releases
  without explicit package records. The optional Windows server retains its
  distinct archive name for old updater compatibility.

### Release checks

- Test publisher output against the native updater selector on both platforms.
  Compile a hash-verified, unchanged copy of the released 1.3.2 parser and check
  that it selects the new metadata's client packages on Windows and Linux.
- Add wrong-component, wrong-platform, wrong-architecture, duplicate-record,
  missing-checksum, malformed-size and unsupported-schema coverage. Exercise
  filename, size and checksum mismatches in Python and Python-free Linux
  preflight without changing the existing installation.
- Build the Windows launcher/setup/updater and run the x86 selector regression
  under Wine. The release workflow also runs its native Windows checks before
  publishing. Two optional private/full-installer Linux fixtures are not part
  of the local helper run.

### Test scope

This beta retains the prestige progression, ghost recovery, split configuration
and diagnostics from 1.4.0-beta.2. It is focused on updater package selection and
preflight; the local Re:Start achievement/artwork experiment is not included.
Prestige isolation remains opt-in. Stable installations remain on the stable
channel unless experimental releases are explicitly enabled.
