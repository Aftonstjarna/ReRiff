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
