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
