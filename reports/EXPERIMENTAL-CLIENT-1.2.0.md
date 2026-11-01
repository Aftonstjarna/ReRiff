# Experimental client 1.2.0 validation

Branch: `codex/experimental-client`. Server image work remains on
`codex/experimental-server`.

- Built the native launcher and original-engine notification/menu DLL, the
  .NET Framework client helpers, and the portable source-built patcher.
- Applied the patcher to supported original DLLs and recorded exact output
  hashes in `packaging/required-client.json`. No original game DLLs are shipped.
- Verified installation from vanilla, stable, and already experimental DLLs
  in disposable directories, followed by restoration of original backups.
  Settings remained unchanged and backups used the game's `bak` folder.
- Verified the actual published v0.1.10 Linux installer can restore stable from
  the experimental SDK through the updater's compatibility adapter. The SDK,
  installed channel, and version returned to stable; settings were preserved.
- Passed updater extraction/rollback tests, installer tests, engine-menu setup
  tests, and the networking toggle test. The latter verifies that disabling
  networking makes no endpoint probes and preserves the configuration bytes.

Linux installation or SDK-hook upgrades require Mono for local source patching.
The Windows installer uses .NET Framework. Engine-menu behavior still needs
normal gameplay testing on the user's installation.

The original `Game.exe` is unchanged. Verified reconstruction coverage remains
64,760 / 8,635,392 bytes (0.749937%); this is not a matching whole-EXE rebuild.
Linux driving remains disabled.
