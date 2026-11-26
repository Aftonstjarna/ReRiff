# Unreleased

## In-game summary

- Broken walls now clear; intact walls stay loaded.
- Track surfaces load from the start on long tracks.
- Distant buildings under the track are no longer retained.

## Full release notes

- Destructible walls now meet base expectations: an intact wall stays loaded by retention, and a broken wall clears exactly as in the vanilla game. `gaPropBase.forceBreak` marks the prop at `+493`; retention now respects that flag in both the visibility pass and the snapshot re-add, instead of checking the unused `+502` byte and forcing smashed walls back on screen.
- Raise the barricade and ramp release window from the vanilla `-30` to the retained behind window, so a ramp launch no longer recycles the walls beside the ramp while they are still visible.
- Leave type 13 distant buildings vanilla by default. The base game hides a building two units behind the car; the retired blocks no longer consume draw calls behind the track. `[graphics] retain_buildings=true` restores the retained behaviour for A/B testing, and the trace logs `Buildings: mode=..., strips=..., blocks=..., visible=...` so the two modes can be compared.
- Size the road capture buffer for the whole track and reuse it between races. The old 40000-vertex cap wrapped the road mesh on tracks longer than about 4400 blocks, so the end-of-track vertices overwrote the beginning and the road surface was missing until the car passed the wrapped range.
- Regenerate the custom unlock manifest from the composed `unlocks_pc` tables instead of copying a package's per-mod manifest. Two car mods that ship different manifests no longer abort car slot preparation, and the manifest always matches the composed tables.
- Accept car packages that ship their own `strings/tokens.dat`. Use that table as the composed base so the new car's `Car_<name>` token resolves from Common, while per-package overrides still supply names such as the 1967 car's `Baby`.
- For a new car package that writes its name on the donor token (Customs 0.3.6-beta.4 and later), create the native `car_<id>` token in the donor's Common group from that text. The stock donor keeps its own name, and the garage can resolve `Car_<id>`.
- Add a native string-manager alias for add-on car names. The composer registers the donor-derived name and `Car_<id>` resolves through a synthetic handle, so the garage shows the custom name without shifting native Common indices.
- Release the previous race's road chunk buffers when the track changes instead of leaking them.
- Rebuild the road capture when the live type-0 strip or the generated track length changes, not only when the track manager object changes. The menu preview and the race share the manager singleton, so a short preview track could leave its chunks in place for a full race and the road would disappear once the car passed the preview range.
- Validation: native retention, prop, track-pool, scrobbler and UI regressions.

# ReRiff 1.5.0-beta.7 — Track visual retention

## In-game summary

- Visual retention 0-10 keeps more road, wall and prop scenery loaded.
- Road chunks render from below; ramps and edge barriers stay loaded.
- Launcher labels and tooltips now use setting names.

## Full release notes

- Add the launcher and `client.ini` `[graphics] visual_retention` setting, 0-10. 0 is vanilla; higher tiers use a larger visual pool and a symmetric behind/ahead block window. Tier 10 uses the largest window the tier ladder drives. Restart required.
- Split the captured road mesh into full-track chunks, including the pre-start range. Each chunk carries both triangle windings so the road is solid from below, and the live road mesh is hidden while chunks are present.
- Retain the type 2/3 edge barrier behind the car by enlarging the wall vertex pool and skipping the triangle erase, so the barrier accumulates over the driven track.
- Keep type 8/9 block visuals and the active prop/prop-node vectors in the retention pass. Props and nodes removed by `gaPropManager` are re-added from a live snapshot with a vtable check; exploded props are excluded and broken prop-node pieces keep their hidden state.
- Clear the visual frustum-cull flag for retained blocks, props, ramps and sign bases, and force two-sided D3D9 rendering while retention is active, so ramp backs, wall backs and the road underside stay visible.
- Leave score-zone scrolling rails and type-10 special blocks vanilla in this beta; they are not part of the retention scope yet.
- Name launcher and installer controls by function: button and checkbox text is the setting name, tooltips state scope, values and restart requirements, and long policy text moved out of the UI.
- Validation: packaging release-metadata tests, client-update tests, UI asset exports, launcher/setup build, native retention regressions, and the release workflow's launcher, client and Windows-loader checks. Live testing covered free look and photo-mode road, wall, ramp, fan and breakable-wall retention; destructible prop coverage can still vary by track.
- Known limitations: prop and destructible-wall retention is best-effort; type-10 special blocks and score-zone rails remain vanilla; retention applies to the experimental channel only.

# ReRiff 1.5.0-beta.6 — Shuffle picker and permanent-level bar fix

## In-game summary

- In the song picker, **R3** or **Ctrl+R** shuffles: it picks a random file from the current folder and its subfolders, skips folders, starts the song, and avoids repeating the last random song when other candidates exist.
- The picker header now reads: `Search (Y / Ctrl+F)  Shuffle (R3 / Ctrl+R)`.
- The Profile main-level bar now uses the native fractional level, so it moves within the current level instead of appearing full.
- The beta.5 photo mode, complete HUD hiding, DOF/bloom toggles and launcher Controls/Graphics options remain.

## Full release notes

- Add R3/Ctrl+R shuffle to the native file picker. It uses the existing recursive picker-search snapshot (MyMusic, saved music roots and the currently open folder), filters to files under the current folder tree, and starts the chosen song through the original `onFileSelected` path.
- Folders are never selected. Search-filtered pickers shuffle within the current result context. If the chosen file is the same as the previous random song and another candidate exists, it re-rolls among the other candidates.
- Add the `Search (Y / Ctrl+F)  Shuffle (R3 / Ctrl+R)` header hint. Ctrl+R is consumed by the picker search window hook and triggers the same shuffle path as R3.
- Fix the permanent-level Profile bar. `gaDriverLevel.getDriverProfileLevel` returns the level plus fractional progress; the display had been looking up a separate table and could pin the bar at `1.000`. It now uses the native fraction directly. At 71,563,334 XP and level 33, the bar shows about 86% toward the 75,000,000 threshold instead of 100%.
- Keep the recursive shuffle scan asynchronous. R3/Ctrl+R retries for a short window while the library snapshot is still being built, then reports when no files exist under the current folder.
- Known limitation: the experimental track-culling switch still reports `volumes=0` on the tested tracks. The culling retarget is being worked on separately and is not claimed as fixed in this beta.
- Validation: native picker alphabet/search tests, Ctrl+R request test, managed ClientConfig build, launcher/native checks, and the existing beta.5 checks. The beta.5 photo-mode, HUD, post-effect and launcher notes remain in the section below.

# ReRiff 1.5.0-beta.5 — Photo mode, post-effect switches and HUD fixes

## In-game summary

- Photo mode: **L3+R3** (or **F9**) pauses the race without opening the pause menu, hides the complete HUD, pauses the music and detaches a free camera. The same chord resumes the race.
- Free camera: left stick/WASD move, right stick/mouse look, LB/RB or Q/E down/up, right trigger or Shift boost.
- Select/View now hides the whole HUD, not only the text elements.
- **L3+A** toggles depth of field; **L3+X** toggles bloom. **F10** and **F11** are keyboard fallbacks.
- The launcher Graphics tab exposes global Bloom and Depth of field defaults, plus the experimental track-culling switch.
- The Profile main-level bar now moves within the current level instead of showing a fixed level/40 fraction.

## Full release notes

- Add experimental race photo mode. It uses the race pause byte directly, so the world freezes without pushing the pause menu. It writes a free camera matrix after the camera manager tick, restores the normal camera on exit, and yields automatically when results or a finish camera takes over.
- Photo-mode controls: right stick or RMB/mouse look, left stick or WASD move, LB/RB or Q/E vertical, right trigger or Shift boost. FOV and camera distance remain the normal race values.
- Default photo-mode chord is `left_thumb+right_thumb` (L3+R3); default keyboard key is F9. Bindings, speed, HUD hiding and a separate photo-strafe inversion are in `[camera]` and `[controls]` in `client.ini`.
- Pause and resume race music with photo mode through the original music-source vtable. Photo mode only resumes music that it paused, so the normal pause menu remains authoritative.
- Separate right-stick look inversion from photo-mode left-stick strafe inversion. `invert_x`/`invert_y` affect look; `photo_invert_strafe` affects only movement. Defaults remain right-is-right.
- Select/View now applies the complete `gaHUDManager` owner mask, so the full HUD hides and restores instead of only text elements.
- Add global depth-of-field and bloom switches. The native layer hooks the `elGraphicsDevice` DOF and bloom vtable entries, validates that both original targets are executable, and forwards unchanged when enabled.
- Add in-game hotkeys: L3+A toggles DOF, L3+X toggles bloom. F10/F11 are keyboard fallbacks. Hotkeys toggle the current session only; the launcher checkboxes are the persistent defaults.
- Keep all client graphics settings in `client.ini`: resolution, display mode, anti-aliasing, UI scale, recent-track limit, track culling, bloom and depth of field. `server.ini` remains server and launcher configuration only.
- Add an experimental track-culling switch. The trace is controlled by Camera diagnostic trace in Diagnostics and logs volumes, valid volumes, forced volumes and pause state. The switch currently reports `volumes=0` on tracks that do not expose `gaTriggerCullingVolume` sections; retargeting remains experimental.
- Add a **Controls** tab after Graphics in the launcher with free-look camera settings, photo-mode settings, HUD and photo bindings, speed, strafe inversion and mouse look. The Diagnostics tab owns the camera/culling trace.
- Add the **Close launcher** checkbox next to Play. When checked, the launcher hides when the game starts and exits when the game closes; unchecked keeps the existing minimize behavior. The choice persists immediately in `server.ini` `[launcher] close_on_game_start`.
- Fix the permanent-level Profile bar. It previously used `level/40` below level 40 and appeared frozen between levels. It now uses the native threshold interval, so level 33 at 71,407,950 XP shows about 85.6% toward level 34 at 75,000,000 XP. The display log includes the fill value.
- Validation: native menu build; managed ClientConfig build; launcher build; launcher tab/save/controls fixture; native post-effect hook, photo pause/HUD state, culling visibility and progression fill tests. Existing livery-merge, car-slot, cache, save, client and progression checks remain in the beta.4 and earlier lines below.
- Known limitations: photo mode is race-only; the track-culling retarget is still experimental; hotkeys are session-only; no live-player gate is added for photo mode.

# ReRiff 1.5.0-beta.4 — Configurable free-look and HUD controls

## In-game summary

- Select/View toggles the HUD during a race.
- Free-look and HUD bindings are configurable for controller and keyboard.
- The launcher has a Controls tab after Graphics for camera and bindings.

## Full release notes

- Add `[controls]` settings in `client.ini`:
  - `free_look_button` (default `right_thumb`), `free_look_key` (default none), `free_look_mouse` (default `right`).
  - `hud_button` (default `back`/Select/View), `hud_key` (default none).
  - Button names include `a/b/x/y`, shoulders, sticks, start, back, and D-pad. Key names include letters, digits, F1–F24, arrows, space, enter, tab, escape, modifiers, numpad and `0xNN` virtual-key codes.
- Wire Select/View through the original `gaHUDManager.toggleUI`, independent of Last.fm/ListenBrainz and independent of free look.
- Keep the camera frame hook installed when no scrobbling provider is registered; the scrobble observer and enter callbacks are null-safe.
- Add a launcher **Controls** tab after Graphics. It edits the free-look camera switch, FOV offset, distance multiplier, X/Y inversion, diagnostic trace, free-look/HUD buttons, keyboard bindings and mouse look button. Values are written to `client.ini`; restart the game to apply them.
- Keep the preferred X/Y directions as the defaults; the invert checkboxes store `1` for the opposite direction.
- Finish/results cameras still take control automatically; Select HUD toggling is a race-time binding.
- Validation: native menu build, launcher build, and the launcher tab/controls fixture. Existing livery-merge, car-slot, cache, save and client checks remain in the beta.2 line below.

# ReRiff 1.5.0-beta.3 — Experimental race free-look camera

## In-game summary

- Press the right stick (R3) to toggle free look on the normal chase camera.
- Right stick X/Y and the mouse (hold RMB) look around the car.
- Free look automatically yields to the finish and results cameras.
- Tune FOV, distance, and X/Y inversion in `client.ini`.

## Full release notes

- Add an experimental native free-look layer for the normal race camera. It keeps the normal camera's already-ticked matrix, then applies yaw/pitch around the car, so the normal camera's distance and field of view are preserved while looking around.
- Toggle with the controller's right-stick click (R3). While enabled, right-stick X/Y and the mouse (right mouse button held, cursor recentered) control yaw and pitch. The camera recenters behind the car after a short idle; toggling off restores the normal camera immediately.
- Free look does not override game-controlled camera transitions. A pending camera switch is left alone, and the results entry disables free look before the end camera appears.
- Add `[camera]` settings in `client.ini`: `race_orbit` (opt-in), `free_fov_offset` (degrees added to the normal camera FOV), `free_zoom` (camera distance multiplier), `invert_x`, and `invert_y` (`0` is the preferred direction, `1` is inverted). `race_orbit_trace=1` enables the diagnostic trace.
- Install the race-tick camera frame even when no scrobbling provider is configured; the scrobble observer and enter callbacks are now null-safe. This keeps the camera independent of Last.fm/ListenBrainz.
- Keep the camera hook idle when free look is off: only the R3 poll runs, and the normal camera is restored once. The diagnostic trace is opt-in and off by default.
- Validation: native client build; in-game tests for normal camera, R3 toggle, right-stick and mouse yaw/pitch, FOV/distance matching, finish-line yield, and results-camera restore. Existing livery-merge, car-slot, save, cache and client checks remain in the beta.2 line below.

# ReRiff 1.5.0-beta.2 — Custom paint livery fix

## In-game summary

- Install livery mods for different paints of the same car at the same time.
- Each package now contributes its selected paint instead of replacing the whole paint table.
- Texture-only livery packages keep working, and custom car names stay intact.
- Switching from a livery paint to a plain paint now clears the previous livery on custom cars.

## Full release notes

- Discover livery packages from `livery-maker.json` at the mod root. A package with both `origin.txt` and livery metadata is refused as ambiguous; car-slot packages continue through the existing composer.
- Build the effective base from the stock archive, other enabled mods' configuration and strings, and the composed custom-car volume. Each livery package supplies only its `selected_pack_index` pack; every other pack is validated against the stock table and refused if the package changes more than one paint.
- Deltas are discovered against the stock table and applied to the effective base. Composed custom-car names such as the Impala's `Baby` are preserved, and the matching `paint_pc.luac` / `.luac64` chunks are regenerated from the merged table.
- Merge `strings/<bank>/<language>.str` edits index-by-index. Token order must match stock; `tokens.dat` edits are refused. Two packages editing the same name with different text are refused.
- Copy and verify every non-configuration resource against the package's `resources[]` SHA-256 list. Identical duplicates are accepted, differing duplicates are refused.
- Mount the merged livery volume after the composed car volume, so its complete paint table wins. The cache fingerprint covers package contents, base hashes and the composed car plan.
- Normalize negative `livery` values to the native `0` no-decal value when composing custom-car paint tables and when merging livery packages. Authoring tools use `-1` for "no livery", but the garage keeps the previous decal when a plain paint follows a livery pack. Stock tables already use `0`.
- Validation: new `LiveryMergeSmoke` covers two packs, string deltas over a composed base, decals, Lua emission, cache reuse, duplicate and multi-paint refusal, texture-only packages and decal invalidation. Existing car-slot, composer, cache, ghost and client checks must pass.
- Built on the 1.4.9 stable line; the retired 1.4.1-beta.7 branch content is unchanged below.

# ReRiff 1.4.9 — Custom car and paint names without shifting stock menus

## In-game summary

- Show custom car and paint names again, including the Impala's "Baby".
- Keep stock menus, achievements and result labels intact.

## Full release notes

- Reuse the authored car identity and its existing stock localization token when that identity is not part of the stock roster. The Impala package authors `dod`, so the composed car now keeps `car_dod` at its stock index and replaces only its text instead of adding a token in another bank.
- Reuse the paint pack's existing `packName` tokens. The composer no longer rewrites them to synthetic `modpack` tokens, and the localization pass maps their text from the car package. Stock token IDs and string indices do not move.
- Keep the empty-bank fallback only for authored identities that have no stock car token, so those caches still load. Names for such identities may not render, but menus never shift.
- Treat an unknown paint-pack token as unresolved instead of refusing the whole car cache.
- Accept retired authored identities in the car registry, so removing a mod does not invalidate `car-slots.ini`.
- Validation: managed client build plus car-slot identity, composer, cache and save-envelope regressions. An offline composition of the Impala package keeps all 651 stock `Common` tokens, adds no tokens, and yields `car_dod = Baby` with the package's paint names.

# ReRiff 1.4.8 — Persist legacy car migration immediately

## In-game summary

- Keep an accepted legacy car migration without waiting for a save.

## Full release notes

- Write the portable car identity trailer as soon as the explicit legacy migration is accepted. Previously the migration lived only in memory until a later save, so closing at the menu before a save made the next launch refuse the same save again.
- If the immediate write fails, for example while the save device is busy, the migration remains pending and the next normal save records it.
- Strict matching and the explicit `legacy_car_migration=1` opt-in are unchanged; mismatched or extra custom progress still refuses and preserves the save.
- Validation: managed client build plus car-save envelope and runtime regressions. The 1.4.7 localization fix is unchanged.

# ReRiff 1.4.7 — Custom-car localization fix

## In-game summary

- Keep stock menus intact when custom cars are installed.
- Read custom car and paint names from a separate bank.

## Full release notes

- Place added car and paint localization tokens in an empty native bank instead of `Common`. Existing token IDs and stock string indices no longer shift, so menus, achievements, tips and result labels keep their text when a custom car is composed.
- Keep the stock `Common` bank byte-identical; the car composer regression asserts this. Custom names still resolve by token name across banks.
- Validation: full client, launcher and installer checks plus car composer, cache, slot identity and save-envelope regressions. The explicit legacy migration from 1.4.6 is unchanged.

# ReRiff 1.4.6 — Legacy car-save migration and mod cache fixes

## In-game summary

- Load existing custom-car saves with an explicit migration.
- Accept mixed-case assets from imported car mods.
- Copy launcher and in-game error messages.

## Full release notes

- Add an explicit one-time migration for saves written before portable car identities. When every active allocated slot has matching progress, the selected custom slot has an identity, and no inactive custom slot carries state, set `legacy_car_migration=1` under `[diagnostics]` in `client.ini`, launch once and let the game save. The next save records the identities; remove the setting afterwards. Mismatched or extra custom progress is still refused and preserved, never guessed.
- Lowercase loose mod resource names when building caches. Stock VFS indexes are lowercase even when archives preserve mixed case, so mods containing `batchTextures.brres` no longer fail with "Resource names must be lowercase ASCII". Case-only duplicate assets are refused with a clear error.
- Replace launcher, updater and in-game error message boxes with a copyable dialog: selectable message text plus a Copy button, with a MessageBox fallback for older helper DLLs. Confirmation prompts remain native dialogs.
- Validation: managed client, native helper, launcher, updater and installer builds pass; car-slot identity, save-envelope, mod cache and slot allocation tests pass. A legacy save is not modified until the explicit opt-in and matching check succeed.

## Known limitations

- The migration opt-in requires the active car-slot registry to match the save exactly. Back up the save and remove the setting after the first successful save.
- Track audio is still read and decoded synchronously when a track is selected; the cache work remains planned for 1.5.0.

# ReRiff 1.4.5 — Custom-car unlocks, server caches and fallback cars

## In-game summary

- Unlock custom cars with configured artist and song rules.
- Add imported cars to the seven spare slots automatically.
- Match re-encodes to one leaderboard by artist and title.

## Full release notes

- Read artist and optional title unlock rules from a custom package's manifest-matched `unlocks_pc` CFG/Lua pair, extending the original Police/Monaco music check. Unmatched, malformed or independently changed pairs keep the original behavior.
- Key the launcher's song cache by the selected server identity. Any HTTP server, including the localhost/LAN server, is distinct; offline singleplayer uses its own cache and is never registered. The launcher now uses WinHTTP for identity sync, restoring reliable server registration.
- Read an optional `[fallbacks]` section from each loose car package's `origin.txt`. It names the stock car shown when the custom car's slot is unavailable, independently of the donor used for assets, configuration, saves and shared ghosts. Invalid or conflicting declarations are refused; the donor is used when no fallback applies.
- Match songs in the server catalog by normalized artist and title, retaining every observed SHA-256 as evidence. Differently encoded copies of one song, including future cache transcodes, share one leaderboard identity. A cache ID may gain a new encoding but cannot change metadata; audio with an empty title or artist still requires exact-source matching.
- Reuse the built picker index for ten minutes and report unreadable folders, so warm picker sessions no longer rescan an unchanged library on slow mounts.
- Report the stock ID that actually resolved a missing-car fallback, and add low-volume diagnostics for frame gaps, FLAC declared/decoded durations, picker refresh reasons, mod-volume preparation and refused ghost fixtures.
- Validation: launcher, client, Windows loader and installer checks pass with the native and managed regressions. The high-sample-rate FLAC analysis resamples to the 44.1 kHz time base; a 96 kHz / 24-bit source (35,744,790 samples) analysed to 16,420,263 frames with zero duration delta, resolving issue #2.
- Add automatic car-slot allocation for loose packages with `overwrite=0`. ReRiff composes the native roster, donor assets, configuration and localization into a mounted car cache, assigns slots 15–21 by stable package identity, and records assignments in `mods/car-slots.ini`. `overwrite=1` continues to replace the donor's stock slot without consuming a new one.
- Move paint selection, ownership, usage and the selected car by stable package identity when slots change, and retain removed cars for reinstall. Saves carry their own checked identity metadata so another client's unlabelled custom progress is refused rather than guessed. Shared ghosts use the stock donor and default paint.
- Track audio is still read and decoded synchronously when a track is selected; local caching for slow mounts and Navidrome downloads is planned for 1.5.0.

# ReRiff 1.4.1-beta.7 — Per-package missing-car fallback

## In-game summary

- Pick which stock car appears when a custom car is missing.
- Keep the donor separate for assets, saves and shared ghosts.
- Set the fallback per authored car in origin.txt.

## Full release notes

- Read an optional `[fallbacks]` section from each loose car package's `origin.txt`. Each entry is **authored car ID = stock fallback**; the `[origins]` donor is unchanged and still supplies copied assets, configuration, save identity and shared-ghost identity.
- Record the donor and fallback in the generated `car-origins.ini`, and retain them when the mod is disabled or removed so the original selected-car setter can select the fallback.
- Refuse unknown, malformed, duplicate or conflicting fallback declarations instead of guessing. An invalid or unavailable fallback falls back to the donor.
- Accept packages without `[fallbacks]` unchanged; they keep using their donor. Existing `origin.txt` policies and the Customs editor's **Missing-car fallback** control write the same format.
- Validation: native fallback planning and metadata tests cover declaration, conflict refusal, invalid IDs, removal retention and fallback-over-donor precedence. Existing launcher and client checks must pass before publication.
- The beta.4 automatic car-slot allocation experiment is still not part of this branch.

# ReRiff 1.4.1-beta.6 — Per-server song caches

## In-game summary

- Keep song caches separate for each server and offline mode.
- Register only the active server's cache with its leaderboard.
- Restore reliable song identity sync with multiplayer servers.

## Full release notes

- Key the launcher's song cache sidecar by the selected server's `ServerID`. Any HTTP server, including the localhost/LAN server, is a distinct server; offline singleplayer uses its own `cache.singleplayer.bin` and is never registered.
- Start a server cache from bundled records only, so singleplayer-generated IDs cannot appear on an alternate server's leaderboard. The game re-adds local songs with server-specific IDs, and the launcher registers that cache for the selected server.
- Replace the launcher's WinINet identity and registration requests with WinHTTP. This fixes the previous `Song identity sync: unavailable` result and lets the server catalog alias and rank the active cache.
- Preserve the beta.5 configured customs music unlock behavior and its manifest-matched unlock files.
- Validation: native cache split, bundled-only bootstrap and sidecar commit tests pass; the launcher was verified against two distinct server IDs and a real localhost server; a one-song registration reached the server and created its catalog record. Cross-client testing is the purpose of this beta.
- The beta.4 automatic car-slot allocation experiment is still not part of this branch.

# ReRiff 1.4.1-beta.5 — Configured custom-car music unlocks

## In-game summary

- Unlock custom cars with configured artist and song rules.
- Keep the original Police and Monaco music unlocks.
- Reject unlock configs that do not match their manifest.

## Full release notes

- Extend the original Track Summary Police/Monaco music check through the existing native `unlockCar` path; no parallel ownership system is introduced.
- Load artist and optional title rules from the active package's manifest-matched `unlocks_pc` CFG/Lua pair. The manifest must match both files exactly; malformed or independently changed pairs fall back to the original defaults.
- Match rules case-insensitively when Track Summary is entered and preserve the original The Police then Fleetwood Mac / Chain precedence when no customs manifest is active.
- Keep authored loose car packages on their existing layout. The beta.4 automatic car-slot allocation experiment is not part of this experimental branch.
- Kansas — Carry on Wayward Son unlocking Baby was confirmed in-game; random and control tracks did not unlock it. Other custom unlock rules remain subject to live testing.

# ReRiff 1.4.1-beta.3 — Missing font repair

## In-game summary

- Repair missing font files automatically during installation.

## Full release notes

- Redownload incomplete font packs, including missing legacy `vfs1` files and partially removed disabled packs.
- Preserve modified files and user settings.
- Font repair, migration and artwork recovery regressions passed.
- Existing beta.2 limitations remain unchanged.

# ReRiff 1.4.1-beta.2 — Loose mods and picker fixes

## In-game summary

- Load loose-file mods and merge archives when slots run low.
- Keep disabled mods and language packs in mods/.disabled.
- Accept searches with a click and display Steam friend names correctly.

## Full release notes

- Load loose assets from each mod's `assets/` folder, building reusable VFS caches locally. Merge excess archives in the base `mods/` folder when native slots run low. [Mod loader guide](https://github.com/Aftonstjarna/ReRiff/wiki/Mod-loader).
- Create `mods/.disabled` during installation and move disabled language packs there; the loader ignores this folder.
- Let left-click accept an active music search, matching Enter and controller A.
- Fix UTF-8 decoding of Steam friend names, including trademark symbols.
- Include opt-in custom-car diagnostics, startup name refresh, and tested stock-car fallback for the Pulse clone experiment. Custom-model sharing remains deferred.

### Validation and known limitations

- Automated regressions cover loose-mod packaging and caching, disabled language migration, search input, friend names, and custom-car fallback. Owner tests confirmed loose loading and the Pulse clone's naming and fallback behavior.
- General custom-car support remains experimental; no car models are bundled. Over-limit merging still needs live rendering validation. Steam virtual-keyboard input and M4A playback remain unsupported.

# ReRiff 1.4.1-beta.1 — Scrobbling, track history and graphics controls

## In-game summary

- Scrobble races to ListenBrainz or Libre.fm alongside Last.fm.
- Browse more recent tracks with difficulty stars and faster scrolling.
- Adjust graphics and track-history limits from the launcher.

## Full release notes

- Add ListenBrainz and experimental Libre.fm support under one Scrobbling tab, with protected credentials and independent queues.
- Add Recent Tracks limits of All, 20, 50 or 100, difficulty stars, accelerated navigation and a view that follows selection.
- Add a launcher Graphics tab; move Diagnostics last and show small restart notices where needed. [Settings guide](https://github.com/Aftonstjarna/ReRiff/wiki/Launcher-and-in-game-settings).
- Share graphics settings between launcher and game, using one fullscreen mode value. Add the Recent Tracks limit to both menus.
- Reuse recent library scans and tolerate malformed display metadata without weakening path or song-ID checks. [Search guide](https://github.com/Aftonstjarna/ReRiff/wiki/Music-search-and-beta-features).

### Validation and known limitations

- Automated checks cover scrobbling, search, settings, native menus and track lists up to 1,000 entries. ListenBrainz posting was confirmed live; Libre.fm linking and submission remain unverified. The latest selection-follow change still needs live testing.
- Steam virtual-keyboard input and M4A playback remain unsupported.

# ReRiff 1.4.0 — Mod loader, prestige, music search and Last.fm

## In-game summary

- Load asset mods in separate folders without replacing stock archives.
- Prestige with isolated saves and configurable progression rewards.
- Search your music library and scrobble your races to Last.fm.

## Full release notes

- **Mod loader:** add or override assets from separate mod folders while keeping stock archives intact. [Guide](https://github.com/Aftonstjarna/ReRiff/wiki/Mod-loader).
- **Prestige:** opt into isolated Singleplayer progression, permanent levels, configurable rewards and paint discounts, with the Re:Start achievement.
- Keep prestige rank milestones and reward displays consistent; restore missing own ghosts from valid caches and support Friend or Foe through Singleplayer ghost wins.
- Add opt-in multiplayer progression with automatic accounts, optional server passwords, isolated saves and recoverable checkpoints. [Progression and server guide](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer).
- Search cached and unimported music with Ctrl+F or Xbox Y, including kana and romaji matching; clear search with Esc/B before leaving the picker.
- Jump between A–Z groups with LT/RT and fix picker navigation after returning from a race. [Search guide](https://github.com/Aftonstjarna/ReRiff/wiki/Music-search-and-beta-features).
- Edit server addresses in-game with selection, paste, undo/redo and a blinking caret. [Settings guide](https://github.com/Aftonstjarna/ReRiff/wiki/Launcher-and-in-game-settings).
- Add Last.fm now-playing and configurable scrobbling, browser account linking and protected credentials. [Last.fm guide](https://github.com/Aftonstjarna/ReRiff/wiki/Last.fm).
- Simplify launcher controls, add hover explanations and progress indicators, and provide a direct Singleplayer shortcut.
- Install artwork and CJK/Latin fonts as independent mods; repair missing owned assets on reinstall while protecting modified files. [Mods guide](https://github.com/Aftonstjarna/ReRiff/wiki/Mods).
- Add **Diagnostics → Uninstall ReRiff**, restoring original DLLs while keeping user data, settings, unrelated mods and recovery backups. [Uninstall guide](https://github.com/Aftonstjarna/ReRiff/wiki/Restoring-original-files).
- Verify update packages by component, platform, size and checksum; keep optional server downloads separate and support older updaters.
- Speed up repeat launches and library searches, cache automatic update checks, and reduce archive-processing memory use. [Update guide](https://github.com/Aftonstjarna/ReRiff/wiki/Updates).
- Add timestamped diagnostics and client/account/server identifiers; separate connection, client and Singleplayer settings while preserving existing preferences. [Configuration guide](https://github.com/Aftonstjarna/ReRiff/wiki/Servers-and-singleplayer).

### Validation and known limitations

- Automated checks cover installers, updates, native controls, progression recovery and server persistence; earlier live Proton checks covered search, server editing and prestige. Broader multiplayer and display-scale testing remains needed.
- Steam virtual-keyboard input and M4A playback remain unsupported. Multiplayer progression trusts clients; it is not anticheat, and unacknowledged progress can be lost after disconnects.
- Search and mod loading have capacity limits; automatic mod merging and the separate livery editor are not included. See the linked guides for details.

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
  recorded in https://github.com/Aftonstjarna/ReRiff/wiki/Developer-Security-triage-2026-11-16; they are not client runtime
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
  font explanations for the new layout. See the [mod loader guide](https://github.com/Aftonstjarna/ReRiff/wiki/Mod-loader) for the mod format.

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
  are skipped. See the [mod loader guide](https://github.com/Aftonstjarna/ReRiff/wiki/Mod-loader#development-loose-assets-and-overflow-merge) for overflow behavior.
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
  the [multiplayer prestige guide](https://github.com/Aftonstjarna/ReRiff/wiki/Progression-and-balance#multiplayer-prestige). This is trusted-client progression, not
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
