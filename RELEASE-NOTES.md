# ReRiff 0.1.1 — Updated launcher

- Add **Stop local server**, restricted to the Python backend belonging to this
  installation. Docker containers and unrelated services are left untouched.
- Add **External server only**, disabling local autostart and fallback in both
  GUI and `--skip-launcher` modes. An unavailable external server disables game
  networking rather than silently using localhost.
- Add **Check for updates**, checking published GitHub releases and opening the
  release page when a newer version is available.
- Include user-agnostic Linux installation instructions in release downloads.
- Publish update-only Windows/Linux packages through GitHub Actions. These
  update ReRiff components and verify the existing patched DLL hashes, without
  including or changing game-derived DLL patches. An existing supported ReRiff
  installation is required; these packages are not fresh-install packages.

Close the game and launcher before installing. Existing settings and saved server
data are preserved. Set Steam's launcher bridge as described in README-LINUX.md.
The game's original Proton/.NET requirements and the backend limitations below
still apply.

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
