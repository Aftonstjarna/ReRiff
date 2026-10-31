# ReRiff 1.2.1-3 — Linux update patcher runtime

- Include the Linux DLL patcher's runtime in the installer so fresh installs and
  updates no longer require Mono or a host .NET installation.
- Keep the signed-helper loading fix from revision -2, including verified DLL
  repair and preservation of existing vanilla backups.
- Record bounded network and assembly-loading exception details in
  ReRiff-network.log, and include that log in the startup report collector.

The Linux update failure was confirmed in update.log. The separately reported
Windows external-server connection failure remains under investigation; the
Windows selects the external address successfully; its subsequent game-request
failure still needs the new network log to identify the cause.

# ReRiff 1.2.1-2 — Windows assembly loading fix

- Fix the assembly-loading failure identified in the supplied Windows crash
  dump: ReRiff.ClientConfig was unsigned while the Parse transport required a
  strongly named dependency (HRESULT 0x80131044).
- Strong-name sign both ReRiff managed helpers and update their injected
  references. Verify fresh and upgraded DLL outputs against known SHA256 hashes.
- Repair supported older patched clients even when original backups are absent;
  preserve existing verified vanilla backups and game settings.
- Test helper loading on Microsoft .NET Framework x86 before publishing.
- Use numbered prerelease revisions and preserve compatibility with older
  experimental-channel updaters.

Install the complete package with ReRiff-Setup.exe or the experimental updater.
Both the patched Parse DLLs and matching helpers must be updated together.
The identified loader error is addressed; the affected Windows client still
needs to confirm that the game reaches its menu.

# ReRiff 1.2.1 — Experimental Windows startup testing

- Include the read-only Windows startup report collector beside Game.exe. It
  gathers installed component hashes, client logs, display driver information,
  and recent game crash events without changing game or Windows settings.
- Accept Steam-forwarded game commands on native Windows.
- Replace the launcher Servers placeholder with a saved skip-launcher option.
- Refresh the experimental update channel to this build while retaining the
  existing stable release.

Extract the entire Windows ZIP and run ReRiff-Setup.exe, selecting Game.exe.
Copying only the launcher does not install the required DLLs and helpers.
The reported white-screen crash has not yet been diagnosed; this prerelease
provides the startup report needed to investigate it.

# ReRiff 1.2.0 — Experimental in-game server management

This is an experimental prerelease. Stable releases remain on their existing channel.

- Replace Friends with a native Servers screen; preserve normal News.
- Save server settings separately from Play; support both --skip and --skip-launcher.
- Disable networking without clearing the saved server addresses.
- Restore the server notification popup alongside the engine menu.
- Add an experimental-build checkbox and show Experimental beside the installed version.
- Report the client version to News and ask the player to exit and update when the server software is newer.
- Support verified vanilla and existing ReRiff installs through local source-built DLL patching. Keep backups in the game's bak folder.

The original Game.exe is unchanged. Recovery and Linux-port sources are checkpointed on the branch; the release does not enable Linux driving.

# ReRiff 0.1.6 — Updater verification release

This release provides a new version to test **Update from GitHub** from v0.1.5.
It retains v0.1.5's updater and server-stop fixes without additional behavior
changes. It is published as a normal release so the stable-release updater can
find it.

To verify: click **Update from GitHub**, allow the launcher to reopen, and confirm
the title reads `ReRiff Launcher v0.1.6`. Settings and saved data should remain.
An already stopped local server must not block the update; a running owned local
backend is stopped before installation. Docker servers are left untouched.

# ReRiff 0.1.5 — Proton update shutdown fix

- Confirm local-server stop results through an atomic completion marker instead
  of Wine start.exe's exit code. Stopping an already stopped server succeeds.
- Let the Linux update tool verify owned-server shutdown directly, fixing updates
  blocked at "Could not stop this installation's local server" in external-only
  mode. Docker and unrelated processes remain untouched.

Launchers affected by the old stop failure need this update installed manually
once. Subsequent updates use the corrected handoff.

# ReRiff 0.1.4 — Launcher version display

- Show the running launcher's compiled version in its window title, for example
  `ReRiff Launcher v0.1.4`.

Use **Update from GitHub** in launcher 0.1.3 or install this update manually.

# ReRiff 0.1.3 — Automatic client updates

- **Update from GitHub** now downloads and installs the latest stable client
  release, then reopens the launcher, on Windows and Linux/Proton.
- Verify the official release SHA256 and package integrity before closing the
  launcher. Apply through the existing installer with backups and rollback;
  preserve server.ini and saved server data.
- Use a separate temporary native updater to wait for the launcher to exit.
  Windows applies from a private Python runtime; Proton uses native Linux Python.
- Stop only this installation's local Python server before replacement. Docker
  servers are not updated or stopped by the client update button.
- Keep the launcher open on download, checksum or preflight failures, with log
  details. Updates remain disabled during a launcher-managed game session.

Install 0.1.3 once using the release installer to enable this feature. Older
launchers still open the release page; they cannot gain automatic updating
without that initial launcher update. An existing supported ReRiff installation
is required. This release includes only ReRiff components, not game DLL patches.

# ReRiff 0.1.2 — Shared song identity

- Automatically register cached song IDs with audio-file SHA256 identities when
  the launcher starts and after the game closes. Identical files on different
  clients share leaderboards without hand-written per-song mappings.
- Persist the server's song catalog in its existing data volume. Existing scores,
  cached IDs, object IDs and ghost uploads remain intact.
- Resolve shared scores, friend queries, ranking counts, ghost lists, FileData
  lookups and ghost downloads across verified equivalent IDs. Count each player's
  best score once per song/mode/version, without deleting duplicate history.
- Keep different source files separate even if their title and artist match.
  Reject conflicting registrations instead of moving existing history.
- Include native C++ identity libraries for Linux and Windows local servers and
  build the library into the Docker image.

Update the server image and the launcher on **both clients**, then launch the
game once through each updated launcher. Source audio files must still be
accessible. New imports made during a session register when the game closes.
Only hashes, song IDs, title and artist are sent; audio and local paths are not.
Older launchers still connect, but cannot register missing cached song identities.

These remain update-only installers requiring an existing supported ReRiff
installation. No original game binaries or binary patches are distributed.

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
