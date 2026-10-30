# ReRiff 0.1.0 — Initial release

## Included

- Offline Linux/Proton self-extracting installer and native Windows setup package.
- Windows package includes portable Python 3.13.17 and pinned server libraries.
- Verified binary-delta patching of the supported 2017 game DLLs; game binaries
  and assets are not redistributed.
- Native Windows launcher, INI editor, automatic local backend startup, and
  `--skip-launcher` mode. Proton uses the native Linux Python backend.
- Client-side external/fallback server selection; blank addresses disable Parse
  networking. Missing or invalid settings fail closed.
- Six-second, non-activating, click-through server notification overlay.
- Structured, credential-redacted request/response traces and gap flags.
- Project-folder original/version backups, repeat installation, hash validation,
  transactional rollback on install failure, and original-DLL restore mode.

## Requirements and known limitations

- Linux requires Python 3.10+ and a working Steam/Proton installation. Windows
  package requires x64 Windows. The launcher's native code adds no .NET dependency;
  the original game's CLR and DirectX requirements remain.
- The supported Game.exe/DLL build is the 2017 release identified by installer
  hashes. Other versions or unknown modifications are rejected.
- Startup notifications are Win32 overlays, not renderer hooks. Exclusive
  fullscreen visibility depends on Proton/compositor behavior; borderless mode
  is recommended. Actual controller hardware and all Proton variants are not
  exhaustively tested.
- Backend selection is fixed for a game session. Local and remote databases are
  not synchronized, and the admin UI manages local storage only.
- The bundled backend inherits a generic-profile update problem and malformed
  news JSON observed during live investigation. Scores, ghost upload/download,
  and track storage have worked in captured sessions; full feature parity is
  not claimed. The backend is intended for localhost use.
- The patched Parse platform assembly retains its assembly identity but cannot
  be re-signed with the original publisher's private key. Strict strong-name
  verification can reject it.
- Windows execution is tested under disposable Wine prefixes; a physical Windows
  installation and full Steam/Proton gameplay remain live validation targets.
