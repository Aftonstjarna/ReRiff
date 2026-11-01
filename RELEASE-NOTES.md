# ReRiff 1.2.2-8 — Version watermark, update notice and native antialiasing

- Show the running ReRiff version in the bottom-right corner of the main menu,
  including numbered experimental revisions. Use native game text with no input
  handling, and follow the current screen size.
- After a successful unattended installation, show "ReRiff has updated to …"
  once on the next main menu. Keep server and update notifications independent,
  non-interactive and automatically dismissed. Failed/manual updates do not
  create notices. Support upgrades started by older unattended updaters too.
  Test this with --test-auto-update after installing this build.
- Add Antialiasing to the original Graphics menu with the native fullscreen
  switch artwork. Apply saves the setting; Back discards pending changes.
- Implement optional post-process edge smoothing on the original D3D9 device,
  disabled by default. Preserve graphics state and display modes; handle device
  reset and fall back to Off when shader/device support is unavailable.
- Retain existing Graphics controls, server controls, navigation and updates.

Experimental: native menu and renderer fixtures verify ownership, placement,
notice consumption, actual smoothed pixels, state restoration and device reset.
Live gameplay and physical Windows/Proton driver testing are still required.

# ReRiff 1.2.2-7 — Steam unattended-test forwarding fix

- Recognize --test-auto-update and --no-auto-update in the Linux Steam bridge.
  Keep Steam's Proton invocation and forward the original Game.exe path and
  arguments explicitly, preserving spaces and Unicode.
- Accept the test flag before or after Steam's forwarded game command without
  passing it into the original game. The test flag implies skipping the launcher
  for that invocation, including on Windows.
- Add regression checks for the real bridge and native launcher parsing, and
  document the Linux Steam launch-options example.
- Retain the progress window, safe Cancel behavior and same-version test mode
  from revision 6. Existing settings and saved data are preserved.

# ReRiff 1.2.2-6 — Launcher skipping and automatic updates

- Add adjacent Skip launcher and Auto-update when skipping launcher checkboxes
  to the launcher window. Skip launcher shares the existing in-game preference;
  saving in-game refreshes the launcher control.
- Offer opt-in automatic updates before skipped launches, on the installed
  release channel. Compare exact numbered revisions, preserve game arguments,
  resume gameplay after installation, and suppress repeat checks during that
  resume. Failed checks start the installed version and log the failure.
- Show a small native progress window for unattended checks/downloads,
  verification and installation. Cancel starts the installed game before
  handoff; cancellation is disabled during file replacement/rollback.
- Add --test-auto-update for a one-shot, same-version unattended reinstall
  through the normal verified update path, without publishing a test release.
  The resumed game launch skips the second check; older builds are not installed.
- Give the launcher, updater and setup explicit asInvoker manifests so Windows
  uses the current account's permissions instead of installer-name detection.
  The game folder must still be writable by that account.
- Add https://riffapi.hobune.stream/ as a commented example in newly generated
  INI files and a bundled server.ini.example without selecting it or overwriting
  existing configurations.
- Include revision 5's centered native scrollbar and Force Offline toggle,
  with Off meaning networking is enabled.

# ReRiff 1.2.2-5 — Centered scrollbar and Force Offline

- Center the native scrollbar vertically on the screen, retaining its height
  and right-side position on both Settings lists.
- Rename Networking to Force Offline and invert its displayed state: Off means
  networking is enabled, On disables networking. The default is Off.
- Preserve existing saved networking preferences and server addresses, including
  save/reload and selection of local or external servers.
- Retain revision 4's navigation and vanilla Settings activation fixes, reported
  working in live testing.

# ReRiff 1.2.2-4 — Restore vanilla Settings activation

- Use base menu input dispatch for the scrolling Settings list. Keep the
  original audio-slider update code on the Audio page, and clear stale Audio
  drag flags before dispatching original Settings actions.
- Reconcile input masks after the game's recursive screen reset: visible rows
  use the active screen mask; hidden rows, arrows and dividers cannot capture
  input. Preserve ongoing press/release state when a mask is unchanged.
- Add regression coverage for hover/click and controller confirmation on
  Graphics, Audio, Controls and Credits, including scrolling to Credits and
  stale slider flags. Exercise the original native screen mask routine.
- Record up to 64 vanilla Settings activations per visit in
  ReRiff-engine-menu.log for follow-up diagnosis.
- Keep Servers first and retain the native file-picker scrollbar.

Experimental: automated checks pass; the reported live-game hover and activation
failure still needs confirmation with this revision.

# ReRiff 1.2.2-3 — Native scrollbar and Settings navigation

- Move Servers to the first row of Settings, preserving all original entries.
- Reuse the original local-track file picker’s gaMenuReel scrollbar, including
  its artwork, arrows, dragging and range limits. Remove the custom scrollbar
  and extra arrow controls from revision 2.
- Give up/down navigation one selection owner with held-button repeat and
  bounded movement. Apply focus after input dispatch to prevent competing
  highlights, skipped entries and stuck navigation.
- Retain vanilla spacing, separator lines, fullscreen-style switches, wheel,
  Page Up/Down, Home/End, save/reload and Save and restart.
- Guard the original executable and bridge methods; update the managed helper
  and native menu together using the complete installer or updater.

Experimental: automated navigation and bridge checks pass; scrollbar behavior
and controller feel still require a live game trial. Friends restoration is
not included in this revision.

# ReRiff 1.2.2-2 — Vanilla spacing and scrolling Settings

- Give Settings and Servers the original row spacing, text anchor and divider
  lines. Add the vanilla arrow beside Servers, after the existing settings.
- Add a proportional scrollbar with wheel, track-click and thumb-drag support,
  plus Page Up/Down, Home/End and controller edge scrolling.
- Keep every entry reachable without squeezing all rows onto one screen. Hidden
  rows and switches cannot accept input. Focus follows the viewport and returns
  correctly from the original Audio/Graphics settings.
- Preserve server selections, fullscreen-style switches, save/reload and restart,
  plus the Linux installer and optional music-folder link from revision 1.

Experimental: the native input and layout harness passes; the revised spacing,
scrollbar and controller feel still need a live game trial.

# ReRiff 1.2.2-1 — Native Settings integration and Linux GUI setup

- Add Servers to the original Settings screen using the game's own text,
  navigation, focus, and rendering. Existing settings entries and actions are
  retained, and Friends keeps its original main-menu entry.
- Reuse the original fullscreen toggle graphics for Skip launcher, External only,
  and Networking, including the original on/off sprites and focus tint.
- Keep server selection, networking and skip-launcher preferences, save/reload,
  and Save and restart in the Settings submenu. Back returns to Settings.
- Add a native Linux setup window with Steam-folder detection, a folder picker,
  explicit Install action, progress, details, retry, and persistent installer
  logs. It uses the existing transactional installer and verified backups.
- Offer an optional Linux Music link with an editable source folder. Respect
  XDG Music settings, preserve existing music files, and retain opt-out on updates.
- Recognize numbered prerelease versions in server News comparisons, with
  numeric revision ordering and stable releases sorting after their prereleases.
- Preserve terminal and automatic-updater behavior for explicit installer
  arguments, and detect common Flatpak Steam libraries.

Experimental: live game rendering and controller testing of the relocated menu
are still required. Friends-list online functionality is a subsequent task.
The Linux setup window requires GTK 3; command-line setup remains available.

# ReRiff 1.2.1 — Stable in-game server management

Combines the tested client changes through experimental 1.2.1-4 and the
experimental Docker server updates into the stable main branch.

- Add a native, controller-friendly Servers menu inside the game, replacing
  Friends while retaining News. Select local or external servers, toggle
  external-only operation or networking, and save without losing stored URLs.
- Add Save and restart: save settings, close the game normally, wait for the old
  process to exit, and relaunch with the new configuration and original launch
  arguments. It supports launcher-started and directly started games and returns
  straight to gameplay without another launcher window.
- Save the skip-launcher preference, support explicit skip/show flags, preserve
  Steam-forwarded Windows game arguments, and refresh launcher controls after
  settings are changed in the game.
- Fix Windows helper loading failure 0x80131044 with strong-name signed managed
  helpers and matching client references. Validate on Microsoft .NET Framework
  x86 and verify known vanilla and upgraded DLL outputs before installation.
- Bundle the Linux patcher's runtime so installation and DLL-hook updates need
  neither Mono nor a separately installed .NET runtime on the Linux host.
- Preserve verified vanilla backups, reject conflicting backup hashes, and
  retain rollback, release checksum verification, and stable/experimental channel
  checks. Current client backups use the game's bak folder.
- Include the Windows startup collector and bounded network exception logging
  in ReRiff-network.log for diagnosing remaining connection problems.
- Promote the Docker server's native version-aware News responses, persistent
  data configuration, and server checks to main and the stable :latest image.
  The game compatibility Version endpoint remains unchanged.
- Update the main README with player instructions and screenshots first, then
  server administration, build instructions, and reconstruction status.

Prerelease users: uncheck Use experimental builds while the game is closed to
install stable 1.2.1. Manual installs must use the complete platform package.
Existing settings and saved data are retained. Docker updates require pulling
and recreating the container with its existing environment and data mount.

The Windows game still needs its original runtime dependencies and uses Proton
on Linux. A reported Windows connection issue after a successful server probe
still needs network-log diagnosis; this release does not claim to resolve it.

# ReRiff 1.2.1-4 — In-game Save and restart

- Replace the third button from the bottom of the native Servers menu with
  Save and restart.
- Save settings first, close the game normally, and wait for the old process to
  exit before relaunching with the saved server configuration and original game
  arguments. The restart goes straight back into the game.
- Support restarting both launcher-started and directly started games. A save
  or handoff failure leaves the game running; a canceled close restores menu
  interaction instead of forcing termination.
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

# ReRiff 0.1.10 — Launcher release channels

- Add a checkbox to switch between experimental builds and the stable release.
- Uncheck it to restore stable, including when stable has the same or an older version.
- Show Experimental beside the version in the title bar for installed experimental builds.
- Verify release checksums and channel metadata before installing. Failed switches retain the installed channel and files.
- Keep this release focused on the launcher; the experimental in-game menu is not included.

# ReRiff 0.1.9 — Vanilla installation support

- Install onto supported unmodified game files by patching the user's DLLs locally.
  Packages include the source-built patcher, not game DLLs or binary deltas.
- Verify both original and patched DLL hashes before changing the installation.
- Keep matching vanilla backups unchanged and continue installation. Reject
  conflicting backup hashes without overwriting them, and roll back game files
  if installation fails.
- Support restoring verified original backups with the current installer.
- Fresh Windows installs use the game's .NET Framework runtime for patching;
  fresh Linux installs require Mono. Existing patched installs need neither
  patching step and retain their settings and saved data.

# ReRiff 0.1.8 — Repository name alignment

- Use Aftonstjarna/ReRiff as the primary combined server-and-launcher repository
  after the fork rename. The previous repository is now Aftonstjarna/ReRiff-test.
- Point the launcher directly at the current release source and enable publishing
  workflows under the new canonical name.
- Use ghcr.io/aftonstjarna/reriff:latest in Docker instructions and by default in
  Compose. Existing deployments should retain their environment and data mount.

Settings, saved records, ghosts and recorded backup locations are preserved.

# ReRiff 0.1.7 — Unified server and launcher repository

- Make Aftonstjarna/riffapi-local-extended the primary combined ReRiff project,
  retaining upstream history, the server, launcher, client tools and installers.
- Move future client release checks to this repository and publish its combined
  server image at ghcr.io/aftonstjarna/riffapi-local-extended.
- Retain the standalone server entry point through the same maintained backend,
  with configurable storage, and carry forward the upstream API test suites.
- Provide this migration build through the previous ReRiff release repository so
  existing launchers can upgrade without manually changing their update source.

Existing server.ini settings, records, ghosts, and recorded backups are preserved.
When changing Docker images, retain the existing data volume or bind mount.

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
