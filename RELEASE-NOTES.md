# ReRiff 1.3.0 — Language fonts, portable clients and UI improvements

## In-game summary

- Add optional CJK and Latin/European fonts across the game.
- Run the Linux client without a separate Python installation.
- Improve UI scaling, display controls and update diagnostics.

## Full release notes

Stable release for Windows and Linux/Proton, including the tested 1.2.4 beta
series and the subsequent missing-font and Auto-scale regression checks.

### Language fonts and song metadata

- Add optional Chinese, Japanese and Korean fonts, plus a separately selectable
  Latin/European pack with missing letters such as Polish ł, ą and ę.
- Install or enable either pack during setup or from the launcher's Languages
  page. Both packs share one additional VFS pair. Disabling a pack keeps its
  files available for offline re-enabling; original game archives are preserved.
- Retain Unicode filenames, song titles and artists across the picker,
  leaderboards and pause screen, including mixed-script spacing, selection
  colours and higher-resolution glyphs.

### Graphics, menus and track analysis

- Refresh manual UI scale independently of resolution. Keep 50 at the fixed
  1080p reference and 0 (Auto) on the game's built-in sizing path.
- Scale main-menu rows and icon hit areas together; use native scrolling for
  overflow. Give Graphics the available space below the header and keep its
  scrollbar clear of Apply. Preserve separator and toggle artwork and keep the
  scale slider rail visible when rendering above the window resolution.
- Improve native fullscreen-selector input, initial row focus, Apply highlighting
  and windowed pointer handling. Retain native menu navigation and key repeat.
- Remove Servers from settings opened during a race. Music-picker navigation
  retains its original boundaries; the withdrawn wraparound probe is excluded.
- Resample higher-rate FLAC PCM to 44.1 kHz before track analysis using the
  original resampler, correcting the extended-track timing problem. The source
  file and playback rate are unchanged. No speculative audio-distortion patch
  is included.

### Installation, local server and updates

- Make the Linux installer and client self-contained without a user-installed
  Python runtime. The optional Flask server runs in its downloadable AppImage.
- Run the optional Windows local server with a private Python runtime in the game
  folder, without Podman. Keep server downloads separate from the client install.
- Add Diagnostics → View update details in the launcher, including retained
  temporary failure messages and bounded log excerpts after reopening it.
  Clarify download limits, missing files, write failures and preflight errors.
- Add logging controls to setup and the launcher. Fresh stable installs default
  logging off; existing explicit preferences are retained. Disabling logging
  keeps an existing logs folder and does not create a new one.
- Use ~/Music for the Linux music link by default, retaining saved custom paths.
- Keep verified originals and the last two backups per configuration. Use .bak,
  reserve binary/font version history for development builds, and keep helper
  scripts and optional server files out of the game root.
- Keep experimental updates opt-in and stable as the default update channel.

### Existing server integration

- Retain main's persistent server identity and permanent Public/LAN aliases,
  including request-matched file URLs and offline LAN access to the same store.
- Combine that routing with optional local-server logging without replacing
  stored profiles, scores, ghosts or uploaded files.

### Validation and known limitation

- Validate native UI input and lifecycle, Auto/manual sizing, missing font-pack
  combinations, independent font installation, shared archive ownership,
  installer rollback, update diagnostics and Python-free Linux operation.
- A beta.6 startup black-screen report stopped reproducing after installing the
  Latin pack and remained resolved when it was disabled. An isolated CJK-only
  launch at scale 55 also succeeded. Its original cause is unconfirmed; this
  release does not claim a fix for that report. Auto remains available at 0.

# ReRiff 1.2.4-beta.6 — UI scaling, Latin fonts and update diagnostics

## In-game summary

- Keep manual UI sizing consistent and scroll crowded menus.
- Add optional Polish and extended Latin font support.
- Read update errors from the launcher's Diagnostics page.

## Full release notes

Experimental client test build for Windows and Linux/Proton. Stable 1.2.3
remains the default download and update channel.

### Menus and graphics

- Refresh manual UI sizing without requiring a resolution change. Keep 50 at
  the fixed 1080p reference size and retain the game's built-in sizing for Auto.
- Scale the main menu's original rows and icon hit areas together. Use the
  native scrolling reel when the list exceeds the available screen space.
- Start Graphics below the header to use the available height, retain native
  row spacing, and reserve room above the scaled Apply button for its scrollbar.
- Preserve separator artwork height and keep the UI Scale rail visible when
  rendering above the window resolution. Align the rail and thumb vertically.
- Preserve native toggle-thumb shading when scrolling or changing focus.
- Remove Servers from settings opened during a race.
- Leave music-picker navigation at its original boundaries. The experimental
  wraparound prototype was withdrawn after a crash during testing.

### Optional language fonts

- Add separate Latin/European font checkboxes to setup and the launcher's
  Languages page, including Polish letters such as ł, ą and ę.
- Publish the Noto Sans component separately as `ReRiff-Latin.zip`, about
  350 KiB. CJK remains independently selectable. Both packs share one extra
  VFS pair, preserve original game archives, and can be re-enabled offline.
- Retain UTF-8 metadata when either language component is enabled and preserve
  existing CJK glyph priority, selection colours and high-resolution metrics.

### Setup and updates

- Add Diagnostics → View update details in the launcher. Show retained
  temporary failure messages and bounded log excerpts in a scrollable window,
  including after reopening the launcher, without enabling diagnostic logging.
- Report download size limits, missing downloads, disk write failures and
  preflight errors more clearly. Preserve existing destination files on failure.
- Default the Linux music link to `~/Music` rather than the whole home folder;
  keep saved custom paths and the option to skip linking.

### Validation

- Add native regressions for scale-only refreshes, main-menu overflow and input
  ownership, scaled icon hit bounds, scrollbar clearance, separator sizing,
  slider pixel alignment, toggle shading and pause-menu isolation.
- Check independent font installation in either order, combined archive
  ownership, disable/re-enable behavior and refusal to overwrite modified packs.
- Check readable UTF-8/UTF-16 update details, download failures, installer
  controls, retained settings and Python-free Linux installation.
- The latest layout and rail changes still need visual testing on more systems.

# ReRiff 1.2.4-beta.5 — Diagnostics and tidier installs

## In-game summary

- Toggle diagnostic logging in setup or the launcher.
- Font support now uses one extra archive pair.
- Keep original files and just two backups of each config.

## Full release notes

- Add a diagnostic-logging checkbox to Windows/Linux setup and a native
  Diagnostics tab in the launcher. Logging defaults on for beta/development
  builds and off for stable builds; an explicit choice is preserved.
- Disabling logging stops client, launcher and updater diagnostic output.
  Restart a running local server to apply its logging change. A fresh disabled install creates no `logs` folder;
  existing logs and their folder are retained.
- Install the startup-report collector under `logs` when logging is enabled;
  keep its offline template under `localserver/diagnostics`.
- Consolidate base and high-resolution CJK textures into one additional native
  VFS pair. Glyph textures, metrics and paths remain unchanged. Existing owned
  font volumes migrate with rollback protection; unrelated mods and original
  game archives are retained. The font download is about 105 MiB.
- Use `.bak` for backups and migrate the former `bak` folder. Preserve verified
  original game files and the two most recent snapshots per configuration file.
  Keep binary/font version history only for experimental/development builds.
  Stable updates remove verified obsolete history after successfully applying.
- Remove the redundant example INI and game-root GitHub shortcuts from new
  installs, retiring existing copies during upgrades. Setup retains its GitHub
  link.
- Verify stable/beta logging defaults, native checkbox behavior, no disabled
  log-folder creation, retained existing logs, config rotation, original-file
  restoration and single-volume font migration on Windows and Linux helpers.
  The local in-game test confirmed filenames, leaderboards and pause text.

# ReRiff 1.2.4-beta.4 — Optional CJK fonts and Languages controls

## In-game summary

- Add optional Chinese, Japanese and Korean font support in setup.
- Install or disable CJK fonts from the launcher's Languages page.
- Keep UTF-8 song titles and artists readable across game screens.

## Full release notes

Experimental client test build for Windows and Linux/Proton. Stable 1.2.3
remains the default download and update channel.

### Optional CJK component

- Add an optional CJK font checkbox to Windows and Linux setup. Keep the choice
  when updating an existing enabled installation; unchecked disables support
  without deleting the installed font files.
- Add a native Languages tab to the launcher with a CJK install/enable checkbox.
  First installation downloads the checksum-verified component for the exact
  client release; re-enabling an installed component works offline.
- Bundle the font component in the installers as well as publishing it separately
  as `ReRiff-CJK.zip`. The current pack is about 107 MiB and includes the Noto
  Open Font License. Players need no system font install or extra Python runtime.
- Extend the game's native text and glyph rendering for missing CJK characters,
  retaining native Latin text, selection colours, layout, alignment and lifecycle.
  Reuse the tested file-picker, leaderboard and pause-screen corrections.
- Select 32/64/96px glyph textures automatically for the rendered text size,
  retaining fixed layout metrics and fallback when a larger tier is unavailable
  or the native visual budget is exhausted.
- This component adds glyph support, not translated menus. It uses the tested
  Japanese Noto glyph forms; regional typography selection is not included.
- Restart the game after enabling or disabling support. Routine use does not
  enable the filename/metadata diagnostic traces.

### Installation and metadata safety

- Install owned additional native font archive pairs without modifying the
  original `vfs.dat` or `vfs.zip`. Choose unused slots around other mods and
  record file hashes for subsequent updates.
- Adopt only the exact known local prototype font archives. Refuse modified
  owned files, unowned font conflicts, invalid packages and linked destinations.
- Stage font files and configuration in the installer's existing rollback
  transaction. Preserve unrelated server settings, scores and cached tracks.
- Apply the verified, field-scoped UTF-8 metadata write/read and equality-query
  patches from original game DLLs. Recognize the tested local prototype hashes
  for upgrade while requiring a hash-verified original backup.
- Retain the guarded in-memory Last.fm artist-text correction. Previously stored
  question-mark metadata is not rewritten and may need a separate repair.

### Validation

- Check component checksums, archive collisions, ownership, settings preservation,
  offline enable/disable, changed-file refusal and rollback on write failure.
- Exercise the native launcher Languages page and Linux setup checkbox flow.
- Verify native text spacing, colour, alignment, quality selection and original
  archive checksums using the recovered game functions and isolated fixtures.
- Test fresh installation and the local prototype upgrade before publication.
- Existing server data and the separate experimental server branch are unchanged.

# ReRiff 1.2.4-beta.3 — Local server startup and Windows Flask package

## In-game summary

- Run the Windows local server without Podman or system Python.
- Install Flask and the backend separately from the launcher.
- Linux server startup reports failures instead of staying busy.

## Full release notes

Experimental client test build for Windows and Linux/Proton. Stable 1.2.3
remains the default download and update channel.

### Windows local server

- Replace the launcher's Podman local-server path with the existing private
  Python runtime and a bundled Flask/Waitress backend. No system Python, pip,
  Podman, PATH or registry changes are required on the player's machine.
- Download the optional `ReRiff-Server-windows.zip` package through Install
  local server. Check the release SHA-256 manifest before installation and
  run a package self-test before replacing an existing application.
- Preserve saved data through install, start, stop and restart. Reuse existing
  launcher-managed Podman data and its configured port in place; stop the old
  container before starting the replacement server.
- Use `http://127.0.0.1:5144` in Secondary server with the default configuration.
  The local backend binds only to loopback; shared remote servers are unchanged.
- Start an installed, configured localhost backend with Play. Stop it after the
  game closes only when that launch started it; leave an already running server
  alone.
- Stop the Windows server cooperatively after checking its process identity
  and per-session token. Leave unrelated processes and occupied ports alone.
- Keep configuration under `localserver/native/`, the optional backend under
  `localserver/windows-server/`, private Python under `localserver/windows-python/`,
  and server diagnostics under the game's `logs/` folder.

### Linux server startup

- Retain the separate Python/Flask AppImage; Linux client installation and
  updates still require no system Python or separate .NET runtime.
- Request AppImage extraction when launching the helper so it runs without FUSE.
- Capture helper output in `logs/ReRiff-localserver-helper.log` and report missing
  completion results instead of leaving the launcher checking server settings.

### Validation

- Exercise Windows package verification, replacement safety, process ownership,
  native launcher start/stop, loopback requests, persistence and port conflicts
  with the private Windows Python runtime under Wine.
- Run Windows process and package unit checks on the Microsoft Windows CI runner.
- Verify Linux client setup/update safety and the optional AppImage lifecycle.
- Check installer manifests and release checksums before publishing assets.
- Retain font/Unicode rendering and metadata probes as local research; they are
  not enabled or packaged in this beta. This release does not deploy the separate
  experimental server branch.

# ReRiff 1.2.4-beta.2 — Python-free Linux setup and menu testing

## In-game summary

- Install and update on Linux without a Python runtime.
- Download and manage an optional Linux server from the launcher.
- Settings confirmations preserve scrolled graphics-menu rows.

## Full release notes

Experimental client test build for Windows and Linux/Proton. Stable 1.2.3
remains the default download and update channel.

### Linux setup and optional local server

- Use the existing self-contained patcher executable for Linux installation
  and updates. The client needs neither Python nor a system .NET installation.
- Keep Flask and Python inside the optional server AppImage. Archive the previous
  client Python runtime and obsolete helpers during upgrades.
- Offer a separate, checksum-verified local-server AppImage through the launcher.
  Linux users can install, start and stop it without Podman. Windows retains
  its existing Podman integration.
- Start an installed Linux server with Play when its localhost address is
  configured. Stop servers started by that launch after the game closes;
  leave previously running servers alone.
- Use `http://127.0.0.1:5144` in Secondary server for the default local server.
  Primary is tried first. Preserve local data and reuse existing launcher-managed
  Podman data and its configured port when migrating.
- Write local-server diagnostics under the game's `logs/` folder. Verify process
  ownership before stopping a server; keep unrelated listeners and processes
  untouched.
- Use a private native helper for Linux update preparation and application,
  retaining checksum, backup and rollback checks. The server download stays
  separate from the client installer.

### Launcher updates

- Windows setup and Python-dependent launcher actions ask before downloading
  a verified private Python runtime into the game folder. No system Python,
  PATH or registry changes are made; installers no longer bundle Python.
- Use standard edit borders for server and timeout fields under Wine, avoiding
  the extra themed focus outline while retaining text editing and Tab navigation.
- Select experimental releases by GitHub publication time. Stable updates use
  GitHub Latest; release metadata supplies the package version and installer.
- Handle escaped release tags, pagination and channel changes while retaining
  checksum and package checks. Skip matching versions unless changing channels
  or requesting a test reinstall.

### Menus and documentation

- Keep fullscreen-selector arrows white while highlighting the selected text.
- Focus Resolution when Graphics opens so Down moves straight to Fullscreen.
- Clear Resolution's highlight and arrow shortcuts when Apply is selected.
- Preserve the graphics-menu scroll viewport while native confirmation dialogs
  own input. Hidden slider rows stay hidden and underlying controls stay inactive.
- Audit native selector, slider, Apply and save-prompt integration against local
  decomp findings; add a regression for confirmation-time row visibility.
- Simplify the README and clarify installation, update channels and Linux setup.
- Record controller-input and remapping research. This build adds no controller
  remapping or acceleration changes.

### Validation and testing

- Local checks cover installer safety, update preparation, native menu navigation,
  UI scale, save prompts, display handling and optional-server lifecycle.
- Tests exercise Linux setup, updates and local-server controls without Python
  on the client PATH. The server AppImage runs without host Python or FUSE.
- Test the complete client on Windows and Linux/Proton. SteamOS/Bazzite and NixOS
  desktop integration still need real-machine testing. Graphics and confirmation
  behavior still need an in-game retest.
- This release does not publish or deploy the separate experimental server branch.

# ReRiff 1.2.4-beta.1 — Display and input testing

## In-game summary

- Test corrected windowed clicks and fullscreen mode selection.
- Save prompts keep input focus while settings stay inactive.
- High-rate FLAC analysis uses the original 44.1 kHz resampler.

## Full release notes

Experimental test build. Display transitions, Alt-Tab, confirmation controls
and windowed pointer alignment need in-game testing on Linux and Windows.
Server ID/migration support is published separately in the experimental Docker
image; updating the client does not update an existing Docker server.

- Restore the selected windowed client dimensions when leaving borderless.
- Map absolute windowed pointer coordinates to the actual render dimensions;
  retain native relative and normalized input paths.
- Remove overlapping fullscreen-label directional activation shortcuts.
- Keep added settings controls and scrolling inactive beneath native overlays.
- Normalize native activation messages and release capture on deactivation.
- Publish persistent server IDs and remember alternate server addresses.
- Add an explicit, backed-up one-time LAN-to-public file-link migration.
- Highlight each fullscreen arrow individually and keep its hitbox separate.
- Resample non-44.1 kHz FLAC analysis PCM with the original Ogg resampler.
  Source audio/playback remain unchanged; regenerate affected cached tracks.
