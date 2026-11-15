# ReRiff 1.4.0-beta.4 — Alphabet navigation and balance tuning

## In-game summary

- Jump between music-picker letter groups with LT and RT.
- Configure discounts to increase every chosen number of levels.
- See Re:Start in Singleplayer achievements after prestiging.

## Full release notes

### Music-picker controller navigation

- Tap RT to jump down to the next available A–Z letter group, or LT to
  jump up to the previous one. Empty letters are skipped and jumps stop
  at the alphabet ends. Release the trigger before the next jump.
- Extend the original keyboard letter-jump function so its scrolling,
  highlighting and row ownership remain native. Ordinary navigation and
  folder opening are unchanged. Non-ASCII initials still use ordinary
  navigation; this does not reintroduce the abandoned Up/Down wraparound.
- Validate the original method signature and CIL checksum before binding;
  refuse navigation outside the active picker phase or without valid focus.

### Singleplayer balance configuration

- Add `discount_mode = interval` to `singleplayer.ini` under `[balance]`.
  Configure `discount_start_level`, `discount_start`, `discount_interval`,
  `discount_increase` and `discount_maximum` without fixed level milestones.
- For example, start at level 10 with 0.20, increase by 0.05 every 5 levels,
  and cap at 0.50: 20% at 10, 25% at 15, 30% at 20, 35% at 25, 40% at 30,
  45% at 35 and 50% from 40 onward. Increases are percentage points.
- Preserve existing discount curves and balance revisions when interval mode
  is not selected. Invalid settings retain the existing whole-policy rejection
  behavior. Changing prices never resets progression; restart to apply.
- Document `paint_price_scale` with an example in future generated configs.

### Re:Start achievement

- Add the native achievement-list entry named **Re:Start**, described as
  **Start again**, for the existing first-prestige flag in isolated Singleplayer.
- Retain native row spacing, selection, scrolling and earned/locked treatment.
  Prestige eligibility still requires the original 38 achievements; the new
  entry is not an additional requirement to prestige again.
- Fresh installations use a borrowed native icon. The custom icon loader and
  source artwork are included for development, but automatic installation of
  its separate archive remains pending. Existing local test artwork can load.

### Validation and test scope

- Live Proton testing confirmed LT/RT letter jumps, highlighting and scrolling.
  Native regression checks cover trigger edges, letter gaps/endpoints, malformed
  lists, native callback dispatch and inactive/no-focus refusal.
- Run 3,642 managed level, discount, price, boundary, overflow and legacy-policy
  checks under the Windows CLR, plus achievement-row and UI lifecycle regressions.
- Preserve beta.3 explicit package metadata and older-updater compatibility.
  GitHub checks gate publication; native Windows and installer checks run there.
- Prestige remains opt-in, with existing save isolation and confirmation rules.
  Stable users remain on the stable channel unless experimental updates are enabled.

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

# ReRiff 1.4.0-beta.2 — Prestige progression and Windows updates

## In-game summary

- Keep prestige rewards, Rank milestones and ghost wins consistent.
- Configure main levels beyond 40 and Singleplayer balance.
- Fix Windows updates and choose individual diagnostic logs.

## Full release notes

### Prestige progression and ghosts

- Recheck the four original Rank achievements after a completed prestige-cycle
  race using permanent main level. A level-33 player can re-earn the level
  5/10/15/20 milestones together without waiting for permanent level 34.
- Earn Friend or Foe in Singleplayer by beating a normal Recommended or Leader
  ghost. Preserve completion carried over from the original save, and retain
  the original strict score comparison and achievement award path.
- Restore a missing own ghost record from a matching native version-18 replay
  cache when the isolated Singleplayer store lacks it. Preserve replay bytes,
  existing score records, conflicting files and the original native cache.
  This fixes the verified case where an older cached personal best prevented
  a lower new run from uploading a ghost into the new local store.
- Show the multiplied ghost-win bonus in the native versus message. At prestige
  1 the default 100-coin bonus displays and credits 200; payment occurs once.
  Creator and race rewards continue using their scoped original UI/payment paths.
- Remove the abandoned startup-crashing native ghost-queue experiment; the
  replacement works through local storage and the original query/download path.

### Permanent main levels and configurable Singleplayer balance

- Continue permanent main levels above 40. By default each later level costs
  250,000,000 XP, repeating the original 39→40 interval. Existing permanent XP
  counts immediately, and lowering a cap does not remove any saved XP.
- Configure `main_level_cap`, `main_level_xp_step` and `main_level_xp_growth` in
  `[balance]` in `singleplayer.ini`. Defaults are 0, 0 and 1: no configured cap,
  the original final XP interval, and a constant XP cost. Keep the native curve
  through level 40 and the original prestige-cycle car unlocks/rewards.
- Keep the native Profile level text, prestige star and fill bar. With a finite
  cap the bar shows level relative to that cap; uncapped mode keeps native fill
  below 40 and shows next-level progress from 40 onward.
- Configure earned coin rates, prestige multiplier growth/cap, paint price scale
  and permanent-level discounts in the same balance section. Invalid policies
  fall back to defaults; saved checkpoints record the effective balance revision.

### Configuration and diagnostics

- Split settings into `server.ini` for connections, `client.ini` for graphics,
  languages, launcher and logging, and `singleplayer.ini` for isolated
  progression/balance. Keep launcher-managed LAN settings in
  `localserver/server.ini`. Existing explicit values win during migration;
  preserve unknown settings and retain at most two config backups per file.
- Enable experimental isolation with `[progression] isolate_singleplayer = 1`
  in `singleplayer.ini`, then restart in Singleplayer. Legacy combined
  `server.ini` settings migrate automatically. Isolation remains opt-in.
- Add individual log-category checkboxes to launcher Diagnostics with a master
  logging switch. Keep category choices when logging is disabled, do not create
  a logs folder when disabled, and retain any existing logs.

### Windows installer and optional local server

- Distinguish client packages from optional server packages when reading release
  metadata. Ignore legacy `ReRiff-Server-*` packages rather than treating their
  `-windows.zip` suffix as a second client installer. Still reject genuinely
  ambiguous duplicate client packages.
- Name future Windows local-server downloads `ReRiff-Windows-Server.zip`.
  Update package generation, verified download/checksum selection and release
  assembly together, including cleanup of an obsolete generated archive when
  rebuilding a local release directory.
- Keep Flask and its dependencies in the optional Windows server package and
  its private Python runtime; the Windows local server does not require Podman.

### Validation and remaining beta work

- Live Singleplayer tests verified Rank re-awards, Leader replay download and
  playback, Friend or Foe, retained achievements, doubled ghost-bonus display
  and payment, and save/restart persistence. The extended-level build retained
  level 33, the prestige star, fill bar and all saved XP after a restart.
- Original-method execution fixtures, 3,598 curve/policy checks, native layout,
  configuration, logging, checkpoint and package-selection regressions pass
  locally. A real in-game level-41 promotion and native Windows gameplay with
  this exact beta still need testing; Wine/CLR fixtures are not that retest.
- Recommended selection may be empty when only one own best replay exists;
  this release does not add a personal-best recommendation fallback.
- The First Prestige flag remains stored, but its logo-bearing achievement-list
  row is still pending. Multiplayer prestige and server-enforced balance remain
  separate future work. None of these tests reset the user's saved XP.

See `docs/PRESTIGE.md`, `docs/CONFIGURATION.md` and `docs/LOGGING.md` for setup,
policy details and rollback boundaries. Experimental updates require opt-in.

# ReRiff 1.4.0-beta.1 — Singleplayer prestige base

## In-game summary

- Try opt-in singleplayer prestige with a manual Credits confirmation.
- Keep your main level and equipped car and paint after prestige.
- Earn more coins after prestige and see a star beside your level.

## Full release notes

### Opt-in singleplayer prestige

- Select Singleplayer in the launcher and enable the experimental profile with
  `[progression] isolate_singleplayer = 1` in the game folder's `server.ini`.
  Restart after changing this setting. It is disabled by default.
- Import an existing original save once into an account-specific local profile.
  Native serialization, race records, ghosts, song history and generated tracks
  are retained; isolated sessions do not replace the original Steam Cloud save.
- With all 38 original in-game achievements completed, open Settings → Credits
  and accept the native confirmation to prestige. Loading, saving, reaching
  completion, Cancel and Back do not trigger a reset.
- Reset coins, cycle XP, progression unlocks and cycle achievements. Retain the
  permanent main XP/level, equipped car and equipped paint pack. Steam
  achievements remain earned. Cycle progression uses the original XP thresholds.
- Display the native difficulty star beside the permanent Profile level. The
  first prestige uses a star alone; subsequent counts add xN. The badge follows
  the level panel and hides on other Profile sections.
- Track native saves and prestige counters together using checksummed two-slot
  checkpoints with interrupted-transaction recovery. Retain the original save
  and back up the complete isolated profile before experimenting or reverting.

### Rewards and discounts

- Multiply creator and race coin awards by prestige count + 1; purchases and
  unrelated credits keep their original behavior. Match quoted and credited
  amounts, and bound native signed counters.
- Discount non-IAP paint packs using permanent level: 10% at 10, 20% at 20,
  25% at 30, then diminishing increases toward 30%.

### Validation and remaining beta work

- Live tests verified original-save isolation, race/save/restart persistence,
  discounted purchase persistence, manual prestige, retained equipped items,
  original achievement reset, and the star's alignment and section visibility.
- Automated checks cover reset refusal/confirmation, interrupted checkpoint
  recovery, reward math, original method bindings and native UI lifecycle.
  Live doubled creator/race payouts after prestige still need validation.
- First Prestige is stored in the originally hidden achievement slot; its
  logo-bearing achievement-list entry is not implemented yet.
- This beta does not add multiplayer prestige, per-server progression authority
  or configurable server balance. Those remain separate experimental work.
- The independent launcher/client checks run in parallel with release validation.
  Release installers retain the latest stable setup and pinned font downloads.

For setup, rollback and test boundaries, see `docs/PRESTIGE.md` in the repository.

# ReRiff 1.3.2 — Simpler setup and singleplayer controls

## In-game summary

- Windows setup now uses one window with options and a progress bar.
- Enable or disable singleplayer with a launcher checkbox.
- Launchers share the saved singleplayer setting with the game menu.

## Full release notes

### Windows setup

- Collect the game folder, CJK and Latin font options, diagnostic logging,
  private Python download consent and optional GitHub visit in one window.
- Keep setup responsive while installation runs, with an animated progress
  bar and status messages for Python preparation and game installation.
- Show completion and failure in the setup window; allow retry after failure.
- Preserve the command-line setup interface used by automatic updates.

### Singleplayer controls on Windows and Linux/Proton

- Replace the Singleplayer button and inverse Use servers checkbox with one
  Enable singleplayer checkbox in the shared launcher UI.
- Save the game's existing server.networking_enabled setting immediately;
  checking singleplayer disables network server selection on the next start.
- Preserve primary and secondary server addresses and unrelated unsaved edits.
  Continue reflecting settings saved from the in-game menu in the launcher.
- Retain automatic singleplayer fallback when configured servers do not respond.

# ReRiff 1.3.1 — LAN quick deploy

## In-game summary

- Share a launcher-installed server with other PCs on your LAN.
- Update an existing local server without losing scores or ghosts.
- Find refreshed setup instructions and screenshots in the README.

## Full release notes

### Launcher-managed LAN server

- Install and start the optional server from the launcher in two clicks.
  Python, Flask and server dependencies are supplied automatically: a private
  runtime on Windows and a self-contained server AppImage on Linux/Proton.
- Accept game connections on IPv4 LAN interfaces as well as localhost. Start
  reports detected LAN addresses; other PCs use the host's LAN address and
  port 5144. Allow that port through the host's private-network firewall.
- Share profiles, scores, tracks and ghosts between LAN and localhost clients.
  Reuse the backend's address matching to return reachable ghost-file links
  without rewriting stored records.
- Keep launcher ownership/status/stop endpoints and the admin page accessible
  only from localhost. LAN requests cannot read the launcher's control token.
- Offer **Update local server** for an installed, stopped server from an older
  client release. Stop it, update it, then start it to enable LAN access.
  Package downloads are verified; server data and configuration are preserved.
  Updating the client alone does not replace the optional server application.
- Keep existing automatic local-server start/stop behavior and unrelated-port
  protection. The dedicated server's Public/LAN configuration is unchanged.

### Documentation and validation

- Refresh README feature highlights, launcher/language screenshots, Japanese
  metadata examples, News previews and platform dependency instructions.
- Keep Podman references in the dedicated Docker server instructions.
- Check LAN/localhost data identity and file links, non-loopback listening,
  host-only control boundaries, graceful shutdown and the existing server
  identity regressions. Release checks cover Windows/Linux packaging and
  installation as well as the native launcher controls.

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

### Validation and known limitations

- The 1.2.3 launcher has a 64 MiB download limit, below these installers' size.
  Download and run the installer manually for this upgrade. Existing settings
  and saved data are preserved; subsequent updates use the newer launcher.
- Validate native UI input and lifecycle, Auto/manual sizing, missing font-pack
  combinations, independent font installation, shared archive ownership,
  installer rollback, update diagnostics and Python-free Linux operation.
- A beta.6 startup black-screen report stopped reproducing after installing the
  Latin pack and remained resolved when it was disabled. An isolated CJK-only
  launch at scale 55 also succeeded. Its original cause is unconfirmed; this
  release does not claim a fix for that report. Auto remains available at 0.

# Historical 1.2.4 beta series

These Windows and Linux/Proton test builds were superseded by stable 1.3.0.
The consolidated stable notes above describe the shipped behavior; the entries
below retain each beta's main changes and relevant testing limits.

# ReRiff 1.2.4-beta.6 — UI scaling, Latin fonts and update diagnostics

## In-game summary

- Keep manual UI sizing consistent and scroll crowded menus.
- Add optional Polish and extended Latin font support.
- Read update errors from the launcher's Diagnostics page.

## Full release notes

Add independent Latin/European fonts, scale-only refreshes, native main-menu
scrolling, Graphics layout corrections and persistent update-error details.
Default the Linux music link to `~/Music`, retaining custom paths. Remove
Servers during races and withdraw the crashing music-picker wraparound probe.
Independent font installation, rollback and native layout checks were added;
broader visual testing was still required at this beta.

# ReRiff 1.2.4-beta.5 — Diagnostics and tidier installs

## In-game summary

- Toggle diagnostic logging in setup or the launcher.
- Font support now uses one extra archive pair.
- Keep original files and just two backups of each config.

## Full release notes

Add logging controls with explicit preferences retained across updates;
disabling logging preserves existing logs without creating a fresh logs folder.
Consolidate CJK support into one extra VFS pair. Migrate backups to `.bak`, keep
verified originals and two config snapshots, and restrict binary/font history
to development builds. Remove obsolete example INIs and game-root shortcuts.
Check logging defaults, backup rotation and font migration.

# ReRiff 1.2.4-beta.4 — Optional CJK fonts and Languages controls

## In-game summary

- Add optional Chinese, Japanese and Korean font support in setup.
- Install or disable CJK fonts from the launcher's Languages page.
- Keep UTF-8 song titles and artists readable across game screens.

## Full release notes

Introduce optional CJK installation and native Languages controls, with
checksum-verified downloads and offline re-enabling. Add size-selected glyph
textures and field-scoped UTF-8 metadata corrections while preserving original
archives and unrelated settings. This adds glyphs, not translated menus or
regional typography selection; existing question-mark metadata is not repaired.
Check archive ownership, conflicts, rollback and native rendering.

# ReRiff 1.2.4-beta.3 — Local server startup and Windows Flask package

## In-game summary

- Run the Windows local server without Podman or system Python.
- Install Flask and the backend separately from the launcher.
- Linux server startup reports failures instead of staying busy.

## Full release notes

Replace Windows launcher-managed Podman with an optional verified Flask/Waitress
package and private Python runtime. Preserve existing backend data, verify
process ownership before stopping it, and leave previously running servers
alone. This beta's local backend was loopback-only; 1.3.1 later added LAN access.
Run the Linux server AppImage without FUSE and report helper failures. Validate
Windows package/process behavior under Wine and Windows CI.

# ReRiff 1.2.4-beta.2 — Python-free Linux setup and menu testing

## In-game summary

- Install and update on Linux without a Python runtime.
- Download and manage an optional Linux server from the launcher.
- Settings confirmations preserve scrolled graphics-menu rows.

## Full release notes

Make Linux client installation and updates self-contained, with Flask/Python
kept in the optional server AppImage. Add checksum-verified server management
and migration of existing launcher-managed data. Windows still used Podman in
this beta. Add private-runtime download consent, publication-time experimental
update selection, clearer edit borders and native Graphics focus/confirmation
fixes. Controller remapping remained research only; real-machine desktop and
in-game graphics retests were outstanding.

# ReRiff 1.2.4-beta.1 — Display and input testing

## In-game summary

- Test corrected windowed clicks and fullscreen mode selection.
- Save prompts keep input focus while settings stay inactive.
- High-rate FLAC analysis uses the original 44.1 kHz resampler.

## Full release notes

Correct windowed dimensions and pointer mapping, fullscreen arrow hitboxes,
activation/capture handling and overlay input ownership. Resample higher-rate
FLAC analysis through the original 44.1 kHz resampler without changing playback;
affected cached tracks need regeneration. Server identity, alternate addresses
and backed-up LAN-to-public link migration were published separately in the
experimental container. Display transitions and input still needed game tests.
