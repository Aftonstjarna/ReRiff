# ReRiff 1.4.0 — Library search, prestige and Last.fm

## In-game summary

- Search your music library and edit server addresses in-game.
- Prestige with isolated saves and configurable progression rewards.
- Link Last.fm and install fonts and artwork as separate mods.

## Full release notes

### Music picker and launcher

- Search cached tracks and unimported music with Ctrl+F or Xbox Y. Search
  filenames, paths and available title/artist metadata, with Unicode-width,
  hiragana/katakana and romaji matching. Esc/B clears search before leaving.
- Jump between available A–Z groups with LT/RT. Preserve the native picker’s
  rows, scrolling and selection, and repair navigation after returning from a race.
- Edit server addresses in-game with selection, paste, undo/redo and a blinking
  caret. Save persists pending settings; Cancel leaves the existing value intact.
- Simplify launcher controls, group diagnostic settings, add checkbox hover help
  and show startup/update progress. The Singleplayer shortcut selects the mode
  without enabling a LAN server.

### Prestige and server progression

- Add opt-in isolated Singleplayer progression, permanent levels beyond 40,
  configurable rewards and paint discounts, and the Re:Start achievement/artwork.
  Prestige uses the native Credits confirmation after all 38 original achievements.
  It preserves permanent XP, equipped items, race records and generated tracks.
- Keep Rank milestones tied to permanent level after prestige. Support Friend or
  Foe from normal Singleplayer ghost wins, recover missing own ghost records from
  valid native replay caches, and keep displayed and credited reward bonuses aligned.
- Include matching client and server support for authenticated multiplayer
  progression. Servers opt in with RERIFF_PROGRESSION_MODE=trusted; legacy
  servers keep their existing behavior. Create accounts automatically, support
  optional server-password admission, and isolate saves by server/account UUID.
- Store acknowledged progression in recoverable checkpoints. Preserve original
  and Singleplayer saves; importing an original save into an empty server account
  requires an explicit action. Configure server balance through RERIFF_BALANCE_*
  environment settings, separately from Singleplayer balance.

### Last.fm, installation and updates

- Add opt-in Last.fm now-playing and scrobbling for completed qualifying listens.
  Link through the browser, configure the listening threshold, and protect saved
  credentials with Windows user-scoped DPAPI. Preserve settings if credentials
  cannot be decrypted; changing API credentials requires linking again.
- Load named mod archives from immediate mods subfolders using native VFS
  behavior. Install ReRiff artwork and optional CJK/Latin fonts independently,
  with ownership checks, backups and rollback. Repair a completely removed mods
  directory on reinstall while preserving stock archives, settings and saves.
- Use explicit component/platform/architecture identities, sizes and checksums
  for update packages. Retain compatibility with older updaters and keep optional
  server packages separate from client downloads.
- Cache unchanged song hashes and successful automatic update checks, reduce
  repeated library-search work, and stream archive checks/extraction in bounded
  chunks. Manual update checks still contact GitHub.
- Add timestamped diagnostics, installation/account/server identifiers and stage
  timings. Split connection, client and Singleplayer settings into their own
  files while preserving existing explicit values during migration.

### Validation and limitations

- Release checks cover native client/launcher behavior, Windows and Linux
  installers, update metadata, progression/checkpoint recovery, server identity,
  persistent storage and authenticated multiplayer restart recovery.
- Earlier live Proton tests covered library search, alphabet navigation, server
  editing and Singleplayer prestige. The combined 1.4.0 build still needs broader
  multiplayer and display-scale testing; performance measurements are not FPS
  or startup-time guarantees.
- Multiplayer progression trusts the client; it is not anticheat. Back up private
  account credentials and checkpoint banks. Unsaved progress since the last
  acknowledged checkpoint may be lost after a disconnect.
- Steam virtual-keyboard input remains unresolved, and M4A playback is unsupported.
  Music search displays up to 200 results and scans up to 100,000 matching files.
- The livery editor is separate and is not included in these server/client packages.

# ReRiff 1.4.0-beta.13 — Faster library search and clearer diagnostics

## In-game summary

- Search large music libraries with less repeated work.
- Speed up repeat launches and reduce update memory use.
- Identify clients and ReRiff accounts in timestamped logs.

## Full release notes

### Startup, search and updates

- Cache unchanged audio-file hashes for repeat launcher song registration.
  Validate opened-file identity, size and timestamps; expire entries after
  24 hours. Keep registration before game startup and preserve server fallback.
- Reuse successful automatic no-update checks for 15 minutes. Manual update
  checks still contact GitHub; failed checks are never cached as successful.
  Defer Windows Python setup until an update actually requires installation.
- Reuse HTTP connections where supported for song probing and registration.
- Reuse normalized search text across partial library snapshots, cancel obsolete
  queries and coalesce rapid typing into one worker per search session.
- Enumerate folders once using cached entry attributes. Preserve linked-root
  support, nested-link exclusion, cancellation, scan limits and song identity.
- Stream archive verification and extraction in bounded chunks. Preserve archive
  path/link checks, expanded-size limits, preflight validation and rollback.

### Diagnostics and mod loading

- Add UTC millisecond timestamps to new human-readable diagnostic lines,
  including multiline errors. Retain existing logs and machine-readable formats.
- Add client diagnostic headers with a persistent installation ID, ReRiff account
  UUID and server UUID, player name when available, installed version, platform
  and process. Distinguish current identity from last-known identity; omit tokens,
  passwords and API keys. Existing logs receive new session headers without
  deleting earlier history.
- Add startup/update stage timings and hash-cache hit/read counts to diagnostics.
- Refresh the native archive manager before reserving mod volume slots. Document
  verified original precedence: named mod archives can already override stock
  assets, and later ordinal mod folder names take priority. Stock archives remain
  unchanged; no replacement resource lookup is introduced.

### Validation and known limitations

- Offline validation includes 228 picker checks, 24 mocked-native bridge checks,
  19 updater regressions, native timestamp/cache/mod-loader tests and isolated
  server registration checks. Diagnostic validation covers 58 Python tests
  (seven optional fixture skips), 25 Linux-helper tests (two optional fixture
  skips), and native/managed logging smoke tests. Release CI builds and validates
  the final packages before attaching them to this draft.
- Synthetic 100,000-entry partial-index rebuilding fell from 1,217 ms to 554 ms
  on the test machine. Chunked extraction substantially reduced Python allocation
  peaks. These are offline measurements, not guaranteed startup or FPS gains.
- Latest combined changes have not received a live in-game retest. Rendering and
  Steam-provided display-name capture still need live confirmation.
- Player names may be unavailable before login or without a supported name
  provider. ReRiff UUIDs, not names or SteamIDs, identify accounts in diagnostics.
- Automatic Steam account linking is not implemented: no trusted server-side
  ticket verifier is configured. Existing account ownership remains unchanged.
- Steam virtual-keyboard input remains unresolved; M4A playback is unsupported.
- This beta is prepared as a draft for review. Stable and public experimental
  download aliases remain unchanged until a separately requested publication.

# ReRiff 1.4.0-beta.12 — Reinstall missing mod folders

## In-game summary

- Recreate a removed mods folder when reinstalling ReRiff.
- Restore selected fonts using the existing installer checkboxes.
- Restore bundled artwork without touching the stock archives.

## Full release notes

### Installer repair

- Treat a completely removed mods directory as a reinstall when named-mod
  ownership records remain. Restore checked CJK and Latin font packs using
  their existing installer controls and clear stale records for unchecked packs.
- Restore bundled ReRiff artwork when its named-mod directory tree was removed.
  Keep settings, saves and original game archives unchanged.
- Retain strict checks for partial installations, modified files, unsafe paths
  and links. This recovery applies to a completely absent mods directory, not
  arbitrary damaged or edited archives.
- Support stale font-record removal in both the native Linux installer and
  Windows installer staging. No manual receipt cleanup is needed for this case.

### Build dependency security

- Update build-only Pillow to 13.0.0 and fontTools to 4.60.2, including the
  experimental image-build workflows. These address the dependency advisories
  recorded in docs/SECURITY-TRIAGE-2026-11-16.md; they are not client runtime
  Python dependencies.

### Validation and known limitations

- Local checks cover removed-tree reinstalls with one or both font packs,
  unchecked-pack cleanup, staging without writes, artwork restoration and
  existing conflict/rollback protections. Native installer checks passed
  (25 tests, two optional full-package/original fixtures skipped locally).
- UI artwork, prestige artwork, CJK atlases and Latin glyph/metrics tests passed
  with the updated build dependencies. Release CI validates Windows helpers,
  native clients and final installer packages before publication.
- Steam virtual-keyboard input remains unresolved; M4A playback is unsupported.
- This is an opt-in prerelease; stable remains the default update channel.

# ReRiff 1.4.0-beta.11 — Named mod folders and independent fonts

## In-game summary

- Load extra game assets from named folders inside mods.
- Install CJK and Latin fonts as separate, independent mods.
- Keep bundled ReRiff artwork separate from the stock archives.

## Full release notes

### Mod archives and fonts

- Discover unnumbered vfs.dat/vfs.zip pairs in immediate mods subfolders,
  reusing the game's native archive loading and resource lookup. Existing
  numbered archives retain their native volume slots.
- Install bundled artwork into mods/reriff-assets, CJK fonts into
  mods/noto-cjk-font and Latin fonts into mods/noto-latin-font. Keep stock
  vfs.dat/vfs.zip untouched. Font updates and disabling remain independent.
- Migrate verified legacy artwork and shared font archives with ownership/hash
  checks, backups and rollback. Retain conflicting or modified files for review
  rather than silently replacing them. Preserve settings and saved progress.
- Update Windows and Linux installer staging/removal handling and launcher
  font explanations for the new layout. See docs/MODS.md for the mod format.

### Documentation

- Document successful stock Proton Experimental song generation using bundled
  Wine Mono without a manual Microsoft .NET install. Record the tested builds,
  prerequisite reproduction and limitations alongside the Proton 10 stall.

### Validation and known limitations

- Focused checks cover mod migration, repeat installs, conflicts, symlinks,
  rollback, independent font packs, artwork ownership and stock preservation.
  Release CI also checks native launcher/client builds and Windows helpers.
- The native engine has 31 extra volume slots; numbered archives reserve slots
  first. Only immediate mod folders are discovered, and linked files/folders
  are skipped. See docs/MODS.md for overflow behavior.
- Beta.10 and older loaders cannot discover the new named folders. Keep the
  migration backups when reverting. Native mount tests do not establish that
  arbitrary third-party mods render correctly.
- Steam virtual-keyboard work remains paused; use a physical keyboard with
  Steam's keyboard closed. M4A playback remains unsupported.
- This is an opt-in prerelease; stable remains the default update channel.

# ReRiff 1.4.0-beta.10 — Protected credentials and launcher cleanup

## In-game summary

- Save Last.fm credentials securely from the launcher.
- Link Last.fm automatically after approving access in your browser.
- Enjoy a cleaner launcher with clearer sections and fixed redraws.

## Full release notes

### Last.fm

- Add masked API key and shared secret fields. When no key is configured,
  Get API key opens Last.fm application registration in the browser.
- Replace separate save buttons with one Save settings action for credentials,
  scrobbling enabled state and listening percentage. Validate before replacing
  the config and preserve unrelated settings.
- Protect the API key, shared secret and linked session with user-scoped Windows
  DPAPI. Saving migrates existing plaintext credentials; failed protection does
  not fall back to writing plaintext. Changing credentials clears the old account
  link and disables scrobbling until linked again.
- Remove Finish linking and Open authorization buttons. While the Last.fm tab is
  open, check approval automatically every five seconds; Link account becomes
  Reopen authorization while waiting. Temporary failures retry after 30 seconds,
  fatal errors reset the attempt, and waiting times out after ten minutes.

### Launcher

- Match the window background to native label backgrounds and add thin section
  separators. Center the footer separator and local-server address, retaining
  readable spacing with a slightly taller window.
- Fix tab-switch redraws that could hide controls: retain native tab headers and
  let the parent window paint the page without overwriting child controls.
- Move View update details above the shared status area to avoid overlapping the
  progress bar. Clear the leftover marquee segment when progress returns to idle.

### Validation and known limitations

- Native regressions cover encrypted storage, invalid-save preservation, changed
  credentials, tab visibility, automatic-link timing/retries/timeouts, repeated
  tab switches, progress reset and update cancellation. Isolated Proton previews
  were inspected for the launcher layout and empty idle progress bar.
- 41 managed checks passed including native-launcher to managed-client DPAPI
  interoperability with dummy credentials. The matching launcher and client
  helper were built and locally installed without changing user configuration.
- Existing Last.fm linking/scrobbling was verified in beta.9; the new automatic
  browser-approval flow still needs a live end-to-end retest.
- Keep client.ini and the Windows profile or Proton prefix private. DPAPI does
  not protect against programs running as the same user; Proton is not guaranteed
  to provide Windows' security boundary. Moving to another profile or prefix may
  require re-entering credentials and linking again. Old plaintext backups are
  not rewritten. Beta.9 cannot read the new protected values, so install the
  launcher and matching client helper together.
- Each installation still needs its own Last.fm application credentials. Restart
  the game after settings changes. The existing scrobble queue and abrupt-exit
  limitations remain; Steam keyboard work is paused and M4A playback unrestored.
- This is an opt-in prerelease; stable remains the default update channel.

# ReRiff 1.4.0-beta.9 — Last.fm scrobbling

## In-game summary

- Link Last.fm directly from the launcher's new tab.
- Scrobble completed races or a configurable listening percentage.
- Retry pending scrobbles automatically after temporary failures.

## Full release notes

### Last.fm

- Add opt-in account linking, disconnect and listening settings directly in a
  native launcher tab. Approve access in your browser; no Last.fm password is
  entered into ReRiff. Supply your own application API key and shared secret in
  the local client.ini [lastfm] section before linking.
- Scrobble tagged songs after a completed race or the configured listening
  percentage (80% by default). Enforce Last.fm's additional requirements: tracks
  longer than 30 seconds and listening for half the song or four minutes,
  whichever is shorter. Pauses, seeks and long stalls do not count as listening.
- Reuse the original race lifecycle, audio position/duration and song catalogue
  metadata. Preserve pause/resume progress, detect restarts, and avoid duplicate
  submission when a track already qualified before the results screen.
- Send Now Playing at race playback start. Handle network requests on a worker,
  persist up to 500 pending scrobbles for the linked session, and retry temporary
  failures with backoff. Disabling or disconnecting stops sending; pending
  scrobbles are not replayed into a different linked session.
- Accept Last.fm's URL-safe authorization tokens, including hyphens and
  underscores. Keep credentials and sessions out of logs and release artifacts;
  use the existing game diagnostic category rather than adding another log.

### Validation and known limitations

- Live account linking succeeded. The game log confirmed Last.fm accepted a
  scrobble before the race finished with the 80% setting, leaving no queued tracks.
- 38 managed checks passed for signing, token validation, listening rules,
  configuration preservation, queue persistence, account isolation and retries.
  Native fixtures passed for the inline tab, settings, race hook guards,
  lifecycle forwarding and existing progression rewards. Original hook addresses
  were checked against the pinned unmodified game executable.
- Each installation currently needs its own Last.fm application credentials.
  Keep client.ini private: it contains the application secret and account session.
  Tracks without usable artist/title metadata are skipped instead of guessing
  from filenames. Restart the game after linking or changing listening settings.
- An abrupt exit can lose a newly qualified track before the worker saves it.
  A lost API acknowledgement can cause a retry using the original timestamp.
- Steam virtual-keyboard input remains unresolved in the tested Proton setup;
  investigation remains paused. M4A playback is not restored.
- This is an opt-in prerelease; stable remains the default update channel.

# ReRiff 1.4.0-beta.8 — Post-race picker fix and launcher progress

## In-game summary

- Reopen New Track after racing a song selected through search.
- Follow update progress directly in the launcher.

## Full release notes

### Music picker

- Remember a search result's parent folder using the original game folder setter
  before selecting the track. This restores the folder handoff used by New Track
  on the results screen, addressing the blank picker after racing a searched song.
- Retain the original short-path conversion and native string ownership. Add a
  regression checking the remembered parent and balanced allocations.

### Launcher

- Add a progress bar between the status message and local-server address using
  the same native control and styling as automatic updates. Display download
  progress when available, animate work without a percentage, and reset when
  work finishes. Keep the indicator visible across launcher tabs.
- Preserve the Force singleplayer label, hover explanations and grouped logs
  introduced in earlier betas.

### Validation and known limitations

- The user confirmed the local search-to-race-to-New Track retest works. The
  managed bridge regression passed 17 checks with native UI calls mocked.
- Native development builds succeeded and were installed locally. Launcher
  progress across all operations and display scales still needs live validation.
- Release publication runs client, launcher, native, installer, update and
  rollback checks before publishing the packages.
- Steam virtual-keyboard input remains unresolved in the tested Proton setup;
  automatic summoning is disabled and investigation remains paused.
- M4A search results remain supported, but M4A playback is not restored.
- This is an opt-in prerelease; stable remains the default update channel.

# ReRiff 1.4.0-beta.7 — Cleaner launcher controls

## In-game summary

- Hover over launcher checkboxes to read their explanations.
- Force singleplayer clearly skips servers without disabling fallback.

## Full release notes

### Launcher

- Rename Enable singleplayer to Force singleplayer. Clarify that unchecking it
  tries configured servers first, then automatically falls back to singleplayer
  when servers are absent or unavailable. Saved addresses and behavior remain
  unchanged; status messages describe what happens on the next game start.
- Move checkbox explanations into standard Windows hover tooltips across Servers,
  Languages and Diagnostics. Shorten checkbox labels and remove persistent help
  blocks while retaining status messages and native checkbox navigation.
- Keep font download, restart, retention and language details in the font tooltips.
  Keep logging retention, restart and sharing guidance in diagnostic tooltips.
- Refresh README screenshots and setup guidance, distinguish stable and beta
  features, and explain the repeated older commit history.

### Validation and known limitations

- The native development build compiled successfully and was installed locally;
  the supplied screenshot confirms the simplified Servers layout. Full hover
  behavior across platforms and display scales still needs live validation.
- Release publication runs the client, launcher, native and installer regression
  checks, including update and rollback validation.
- Library search and grouped diagnostics from beta.6 remain included. Steam
  virtual-keyboard input remains unresolved in the tested Proton configuration;
  automatic summoning is disabled and investigation remains paused.
- M4A files can appear in search, but M4A playback remains unsupported.
- This is an opt-in prerelease. Stable remains the default update channel.

# ReRiff 1.4.0-beta.6 — Library search and grouped diagnostics

## In-game summary

- Search your music library, including tracks not yet imported.
- Open music search with Y, Ctrl+F or a click.
- Find related diagnostics together in fewer log files.

## Full release notes

### Music-library search

- Add an inline search field to the native music picker. Xbox Y, Ctrl+F and
  clicking the field begin editing; Enter/A finishes editing, and Esc/B clears
  search before native Back can leave the picker. Retain the editor across
  keyboard focus changes and wait for Back to be released after cancellation.
- Recursively search the Music folder, saved favourites, the current folder and
  cached-track locations, including unimported audio files. Match filenames,
  paths and available cached title/artist metadata, with case-insensitive,
  Unicode-width, hiragana/katakana and romaji matching.
- Publish cached tracks and progressive filesystem snapshots instead of waiting
  for the entire scan. Ignore an invalid optional song catalogue and continue
  searching files; retain worker errors rather than displaying Searching forever.
- Build results through the original picker list and selection routines. Keep
  native spacing, artwork, clipping and scrolling, and preserve original folder
  entries for cancellation. Picker wraparound remains excluded.

### Diagnostic logs and launcher

- Reduce the launcher from thirteen diagnostic categories to nine. Combine
  client/network/news into ReRiff-connections.log, menus/progression/singleplayer
  into ReRiff-game.log, and Unicode/metadata into ReRiff-text.log. Prefix grouped
  records with their component and coordinate native/managed appends.
- Preserve older per-component preferences until a group is explicitly changed.
  Move superseded logs into logs/archive without changing their contents.
- Reduce the outline around the ReRiff launcher button logo.
- Show development build in the game and launcher for unpublished local builds.
  Official release packages display their beta version; updater version metadata
  remains separate from the development label.
- Include scoped text-input diagnostics that count queue/editor messages without
  recording typed text, plus direct Unicode-character input support.

### Validation and known limitations

- Live Proton testing confirmed working library search. The tested library
  contained 6,533 tracks and completed enumeration in about 1.5 seconds; timings
  depend on storage and library size. Automated checks cover cache failure
  recovery, matching, unimported files, native result ownership, cancellation,
  shared text editing, grouped log preferences and diagnostic archive retention.
- Search displays at most 200 result rows and scans at most 100,000 files that
  match supported audio extensions. Unreadable folders or the limit produce a
  partial scan; nested links/junctions are not followed. These can be selected as
  roots. High UI scales and layouts beyond those tested need further live checks.
- Steam's virtual keyboard remains unable to deliver input in the tested
  desktop/Proton configuration. Physical typing works when that keyboard is
  closed. Automatic keyboard summoning is disabled, and investigation is paused.
  No Steam keyboard fix is claimed by this beta.
- M4A files can appear in search, but M4A decoding remains unsupported. This
  release does not restore M4A playback.
- Existing multiplayer prestige, server/account isolation and coin-message
  changes remain included. This is an opt-in prerelease; stable remains the
  default update channel.

# ReRiff 1.4.0-beta.5 — Server editing and multiplayer prestige

## In-game summary

- Edit server addresses in-game with undo, redo and a blinking caret.
- Keep prestige progress on compatible experimental multiplayer servers.
- Get Re:Start artwork automatically with updated launcher branding.

## Full release notes

### Native server-address editing

- Edit Primary Server and Secondary Server through the original confirmation
  dialog, retaining its buttons, input handling and menu lifecycle. Accepting
  updates pending settings; the existing Save action persists them. Cancel
  discards the edit.
- Support Unicode typing and paste, caret movement, selection, Home/End,
  deletion, Ctrl+A, Ctrl+Z, Ctrl+Y and Ctrl+Shift+Z. Consecutive typing groups
  into undo steps; opening a field starts a separate bounded history.
- Display up to 48 UTF-16 units before horizontal scrolling is needed, keeping
  ordinary server addresses visible. Blink the caret using system timing and
  reset it after editing/navigation. Hide routine tips beneath the input line.
- Preserve native row spacing and reel scrolling for the additional server
  rows. Correct native toggle construction and saving Skip Launcher to its
  owning client configuration, including rollback after a partial save failure.

### Experimental multiplayer prestige

- Include the tested multiplayer client and matching server implementation.
  Compatible servers opt in with `RERIFF_PROGRESSION_MODE=trusted`; ordinary
  legacy servers retain their existing behavior. The experimental server image
  already carries this protocol; bundled server packages now carry it too.
- Discover server identity and create an account automatically, with optional
  server-password admission. Store private account credentials locally and
  separate native saves by immutable server/account UUIDs.
- Save progression in server checkpoints, recover acknowledged progress on
  reconnect, and preserve the native Credits confirmation/reset flow for
  prestige. Handle interrupted resets and lost acknowledgements without
  duplicate resets. Initialize fresh profiles through native save allocation.
- Keep original and singleplayer saves separate. An explicit one-time import
  into an empty test account remains available; no automatic import occurs.
- Include policy, protocol, account-cache and server-setup documentation in
  `docs/MULTIPLAYER-PRESTIGE.md`. This is trusted-client progression, not
  anticheat verification. Losing private account credentials can lose account
  access; gameplay since the last acknowledged save may be lost on disconnect.

### Reward messages, artwork and installer

- Make the level-up coin popup use the same configured prestige multiplier as
  its reward. Preserve the native message, localization and reward calculation;
  unrelated legacy or purchased-currency messages retain their native amounts.
- Bundle Re:Start artwork independently of optional fonts, ready on first
  launch. Install it in a separate owned VFS pair, adopt identical prototypes,
  and refuse to overwrite unrelated archives or modified owned artwork.
- Add supplied icons and branding to the launcher and setup. The launcher logo
  uses a native button that opens the repository, with transparent outlined
  lettering and normal keyboard/focus behavior.

### Validation and known limitations

- Live Proton testing confirmed server editing and the wider blinking-caret
  field. Automated checks cover undo/redo, clipboard, UTF-16 boundaries, native
  overlay ownership, save rollback and menu/reel behavior.
- Validate the integrated native/managed client with multiplayer checkpoint,
  server/account isolation, reconnect and level-up callback regressions. Release
  publication is gated by native Windows, managed client and installer CI.
- Kana/romaji matching is included and tested, but music-picker search UI and
  filtering are not connected yet. Kanji readings remain deferred. Picker
  wraparound remains abandoned; existing LT/RT alphabet jumps are retained.
- Full multiplayer earning/prestige/reconnect playthroughs and visual checks
  across all resolutions, UI scales and IME configurations remain beta testing
  work. Server observations are trusted, not independently verified gameplay.
- This release is an opt-in prerelease. Stable 1.3.2 remains GitHub Latest and
  the default download/update channel.

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
