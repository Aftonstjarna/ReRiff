# Car capacity and save-state traces

Original reference: Steam depot 351991/build 1687655 `Game.exe`, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the RiffRacer recovery project and earlier ReRiff progression notes.
The findings below come from original metadata/CIL, not a matching reconstruction.

## Verified original capacities

- `gaVehiclePaintManager.create`, RVA `0x2D7260`, allocates 796 bytes.
  Metadata also declares the struct size as 796.
- Its constructor, RVA `0x31ABD4`, constructs **22** 12-byte vector objects
  at +0 and **22** 24-byte native narrow strings at +264. Count is +792.
  Constructor IL_0001..0004 is `1F 0C 1F 16` (12-byte stride, 22 entries);
  IL_0017..0020 is `20 08 01 00 00 58 1F 18 1F 16` (264, add, 24, 22).
- `setupCarNames`, RVA `0x31A038`, reads `carNames`, stores its count at
  +792 and assigns the strings beginning at +264 with a 24-byte stride.
  The observed assignment loop does not clamp the configuration to 22.
  A 23rd name reaches +792 and overlaps the count/end of this allocation.
- `getNumPacksForCar`, RVA `0x319684`, uses `carIndex * 12` to address the
  vector and divides `(end - begin)` by **176**, the PaintDef record size.
  It does not bounds-check the car index. Index 22 points at the name array.
- Settings paint indices are 22 int32 values at +236 through +323;
  chosen car is int32 at +324. Getter `0x2FD360` and setter `0x2FD37C`
  index directly; `setChosenCar` is `0x2FD39C`. These are numeric indices,
  not stable internal-name strings.
- Settings `save`, RVA `0x2FCD08`, 941-byte body, serializes 22 paint
  choices followed by chosen car. `load`, RVA `0x2FD450`, 1249-byte body,
  is the corresponding versioned reader.
- Progression `PackedUnlocks.getCarUnlockState`, RVA `0x2FC128`, rejects
  unsigned car indices above 21. `getCarPackUnlockState`, `0x2FC18C`, also
  rejects pack indices above 8. Setters `0x2FC154` and `0x2FC1C4` use the
  same limits: **22 cars and nine packs per car**, despite two physical
  bytes being reserved for each car's pack mask.
- PackedUnlocks starts at progression +53: three bytes of car ownership,
  then 22 two-byte paint masks at +56. Car-use int32 counters start at
  +188; index 22 reaches +276, the next field. `incCarDriven`, RVA
  `0x2FC844`, indexes these without a bounds check.
- The progression v6 serializer writes 22 groups, each with ownership,
  a 16-bit counter and nine paint bits. See
  [original progression serialization](PROGRESSION-COIN-SAVE-FIELD.md).

The hash-pinned original `scripts/config/vehicleSelect.cfg` has 16 carNames
entries: `"", alf, bui, cor, rs1, vau, fab, hon, cad, rol, mer, mot, pha, che,
f1r, pl1`. Its SHA-256 is
`10bc4a00de3a14435df8d34c8de240000ac8ea584dcbefc349dcf32b149c2825`.
`unlocks.cfg` and `unlocks_PC.cfg` contain the same ordered list; both hash to
`4a7e34b7a1c61f6489aca1ce81cc69ac834ee3537b6b171681c76de4a30e1c40`.
These were read using the existing recovery GFCF layout in
`NativeLinux/archive.hpp` (20-byte header, 32-byte records, shared string pool).
The serialized list includes an empty entry, but this is not a loaded car slot:
the live trace below establishes 15 occupied slots and seven unused slots
(15–21). The earlier static-only estimate of six candidate slots was conservative
and incorrectly treated the empty serialized entry as runtime index 0. This does not establish that all garage, unlock,
localization, model, or removal paths support them. Live loaded occupancy
must also be checked because modded configuration can differ.
Appending beyond 22 is unsafe without extending all related storage and saves.
Reordering or removing entries can reinterpret saved numeric selections.

## Intentional ReRiff diagnostics

Built into `ReRiff.Notify.dll`, disabled by default. In `client.ini`:

```ini
[diagnostics]
logging_enabled = true
log_game = true
car_capacity_trace = 1
car_save_trace = 1
```

Use the launcher's diagnostic master switch and **Menus, progression and saves**
category if the existing INI uses different grouped logging selections. The
observer checks the normal `engine_menu` category gate and these two new flags.
Restart the game after enabling. Output shares `logs/ReRiff-game.log`.

- `Car capacity:` records configured count, every fixed slot's internal name,
  vector validity, used and allocated PaintDef counts. Counts outside 0..22
  produce `UNSAFE_COUNT` and skip indexed inspection. More than nine loaded
  paints are flagged `UNSAVABLE_PACKS`. Vector capacity is not save capacity.
- `Car save state:` records selected car, the 22 chosen paint indices,
  ownership/paint masks, car-use counters and original dirty/enabled flags.
  Existing read-only save-device/file-name diagnostics identify native save
  slots. These logs do not serialize, reset, unlock, allocate cars or save.
- Sampling is every 500 ms for at most 30 minutes; only changed snapshots are
  logged. Reads use ReadProcessMemory and bounded local copies, never native
  mutating getters/setters. Sampling can miss short-lived transitions and
  observe non-atomic intermediate states. Dirty flags are not a disk-save receipt.

## Live procedure

1. With unmodified garage configuration, launch and open Garage. Capture the
   capacity snapshot to establish which of the 22 slots are occupied.
2. Select another existing car and paint, leave Garage normally, then quit.
3. Restart and compare selected car/paint, ownership and counters in the new
   initial snapshot. Preserve the logs from both runs.
4. Do not append the Pulse clone yet. Confirm an unused supported slot and
   stable index policy first; test additions/removal only against isolated saves.

`test_car_capacity_trace.c` covers all 22 slots, last-slot masks/counters,
invalid counts, malformed vectors, excess paint counts, missing pointers,
output-buffer bounds and non-mutating capacity reads. The client DLL is built. The observer itself does not modify saves.


## Live results — 2026-11-19 UTC

Evidence: local `logs/ReRiff-game.log`, sessions beginning 04:19:38 and
04:21:30 UTC, development DLL SHA-256
`25d9a52ff5dc1a7aef347ccc5cd764f589dcd76aed4896f5af822fcb73a1cd0c`.
The user operated and restarted the game; diagnostics only observed memory.

- Both sessions report configured=15, fixed_slots=22. Runtime indices 0–14
  are `alf,bui,cor,rs1,vau,fab,hon,cad,rol,mer,mot,pha,che,f1r,pl1`.
  Slots 15–21 have empty names and empty valid vectors: seven unused candidates.
  This corrects the static serialized-list interpretation above.
- Loaded paint counts are respectively `9,8,8,7,9,9,9,9,7,7,9,8,9,1,1`.
  All vectors validate; none exceeds the nine-pack save limit.
- Selected car changed from 5 (`fab`) to 0 (`alf`). Slot 0 paint changed
  from 0 through preview choices to 2. Its ownership mask changed from
  `001` to `005`, consistent with unlocking paint index 2.
- At 04:20:36.851, selected car was 0, paint was 2 and both dirty flags
  were clear. At 04:21:30.648 after restart, selected car and all 22 slots'
  paint choices, ownership masks and use counters exactly matched that final
  snapshot. Thus existing-car selection and the observed paint unlock persisted.
- No car-use counters changed during these sessions; race counter increments
  were not tested. No extra car was added. Seven unused storage slots are
  **not** proof that adding seven cars is safe through every native subsystem.
  The next experiment remains a single appended clone with isolated saves.

## Next live experiment prepared — Pulse clone

`scripts/build-car-clone-test.py` builds local-only assets from the pinned depot;
its generated package must not be redistributed. The stock US localization
maps `car_alf` to **Pulse** and `car_fab` to **Photon**. This test copies **alf**.

The experiment appends the already-defined but unloaded `dod` identifier at
runtime slot 15. It does not insert or reorder existing garage entries. Reusing
that identifier avoids introducing new localization-token infrastructure during
this capacity experiment; arbitrary new identifiers remain untested. All Pulse
model, collision, shadow, texture, decal and HUD resource bytes are copied to
corresponding dod resource paths without editing their internal geometry.

Overrides cover vehicleSelect, unlocks/unlocks_PC and paint/paint_PC. The new
car and second paint cost one coin through the original purchase mechanism.
Two paints use native color parameters (magenta and mint); localization reuses
`car_dod` and unused tenth-pack tokens `alfpack10` / `fabpack10` to show
Pulse Test / Test Magenta / Test Mint. Existing active-car paints are unchanged.
This reuse is only a local experimental convention, not a general mod ID policy.

Both GFCF and Lua 5.1 architecture variants are supplied. Lua is emitted using
only LOADK, NEWTABLE, SETTABLE, SETGLOBAL and RETURN, preserving the stock
little-endian float32 headers and 32/64-bit size_t variants. Independent test
bytecode evaluation matches the rebuilt GFCF trees, including their zero-index
array entries; original native script execution remains a live-test requirement.
The existing NativeLinux archive verifier decoded all five generated configs
and validated the 33-resource ZIP. Regression checks prove original car order
and active paint definitions are retained and cloned model bytes match stock.

Local test root: `build/car-clone-test/`. `Launch-Pulse-Test.sh` starts its own
`game/Game.exe` under its own `compat/` prefix cloned from the prior isolated
Proton Experimental test. Server routing forces embedded Singleplayer and
`singleplayer.ini` enables native isolated progression/cloud suppression.
No normal-install save directories, credentials or accounts were copied into
the test game directory. The normal installation is unchanged. The user will
launch and close the game; it has not been launched as part of preparation.

Live acceptance: find Pulse Test after Police; buy it, buy/select Test Mint,
race, exit normally and restart via the same script. Confirm slot15/name dod,
two paint definitions, selection15/paint1, ownership/mask and race-use counter
persist. Then switch to a stock car before a later removal experiment. Stop on
missing assets or a blank garage and inspect logs before making further changes.

### First clone launch failed; array correction prepared

User reported startup crashes at 2026-11-19 04:28 UTC. Native trace reached
progression isolation initialization but never produced a capacity snapshot.
Proton log included a stream NotSupportedException and invalid-window exceptions,
without enough stack information to attribute the exit conclusively.

Inspection found an independent defect in generated Lua: unnamed paint-pack and
RGB arrays started at zero. Original paint bytecode uses SETLIST and indices
1..9 for Pulse packs and 1..3 for RGB components. The GFCF car-name/unlock
sentinel convention must not be applied to those arrays. The builder now starts
non-sentinel arrays at one. Regression validation executes the original stock
paint bytecode's static table-building operations and compares key ranges with
the generated Lua, rather than relying only on generated CFG/Lua agreement.
Both architectures and all five configs pass; native ZIP/config verifier passes.

Corrected package-v3 is installed only in the isolated test game. Failed v2
package and logs are preserved under build/car-clone-test/failed-launch-v2.
The launch script now enables Wine SEH and Mono exception traces. A new user
launch is required to establish whether this correction resolves the crash;
the array defect alone is not proof of the logged exception's cause.

### Garage crash localized; display-order correction

The 04:30:58 UTC trace confirms configured=16, slot15 name=dod with two valid
paint records. Startup succeeded with v3. Entering Garage then threw a
NullReferenceException in gaMenu.createTextBacking at IL_01c5, called from
 gaVehicleSelect.initScreens at IL_0fb7. Original initScreens passes the begin
pointer of its vector<elVec2> at self+324; the caller later assumes this vector
contains a first item. Its population uses the config `default` array.

Original vehicleSelect.luac ends with NEWTABLE, GETGLOBAL for the ordered car
IDs, SETLIST, SETGLOBAL default. Its default is a numeric sequence, despite the
GFCF representation retaining symbolic car names on those entries. The test
builder incorrectly emitted a name-keyed Lua table, yielding no sequence entries
and leaving the garage vectors empty. v4 explicitly emits default as a one-based
numeric array. Regression asserts its 16 keys and original order plus appended
dod. Also corrected unlockedAtStart emission: native Lua false must be LOADBOOL,
not float zero (which Lua considers true). v3's clone became unlocked without
purchase; that observation cannot validate the intended purchase path.

v4 archive/config checks pass and it is installed only in the isolated game.
The previous mod and detailed logs are retained in failed-garage-v3. No live
launch was performed by the agent. Garage rendering/racing/persistence remain
pending; paint-manager storage capacity is now directly observed for slot15.

### v4 garage success — 2026-11-19 04:33 UTC session

User screenshot shows the sixteenth entry between Police and Pulse, rendering
in Garage. It still displays **Goliath**, not the intended Pulse Test label:
localization override is not validated and remains unresolved. The screenshot
alone does not prove all model resources are the intended clone.

Trace confirms slot15=dod, two valid paints, ownership 0→1, then pack mask
001→003 and chosen paint switching between 0 and 1. Native purchase logging
shows one-coin deductions and progression checkpoints generations 2 and 3.
Latest Proton log contains no fatal/unhandled exception or NullReferenceException.
First-chance stream, missing optional configuration fields and missing test
client-version.txt exceptions still appear; these did not abort this session.

At inspection the native selected car remains index5 and slot15 driven remains
zero. No subsequent restart is present yet. Therefore garage browsing, purchase
and paint preview are observed; active race selection, driving and restart
persistence of the added slot are still unverified. No further install changes
were made during this log review.

### Name refresh observation

User reports fresh startup shows Goliath in Garage, but entering the pre-race
car selection changes it to Pulse Test and that name remains on returning to
Garage. Screenshot confirms Pulse Test. This supersedes a blanket statement
that localization overrides fail: replacement text is valid and becomes visible
within the same session. Initial localization caching/mount timing is the
current hypothesis, not yet an instrumented causal finding. mods_vfs.h installs
at managed engine-menu initialization and explicitly supports mounting after
the original initial archive pass; original CIL has several native
elStringManager.load/unload lifecycle calls. No forced reload or startup hook
change has been installed. Investigate the original load boundary before
changing it; retain the ongoing race/persistence test configuration.

### Experimental startup localization refresh

User confirms the added car selection survives restarting; only the initial
name reverts. Native inspection explains a cached-load barrier:

- `elStringManager.load`, token060016BF, RVA0x24EF0. At RVA0x24F2F,
  `83 FF 14 75 1A 8B 3B` selects current language for sentinel20.
  At 0x24F51..0x24F58, matching current language (self+0) and loaded bank mask
  (self+12) causes return without reading archives. Already-loaded individual
  banks also skip at 0x24FB7..0x24FBE.
- `elStringManager.unload`, token060016C0, RVA0x25370, frees selected bank
  buffers and clears their token lookup pointers. At0x2543C the exact bytes
  `8B 45 08 F7 D0 21 47 0C` clear those bits from self+12.
- Singleton is base+0x7FCB34. Original gaLoadSaveScreen.enter RVA0x3147DC
  IL0093..009B uses native load(mask1, language20). Original gaStateBoot.tick
  RVA0x2FDEE4 loads mask3 with getIPLLanguage.

Intentional ReRiff change: ExperimentalMenu/mod_localization.h uses those two
native thiscall functions once after an observed increase in mounted archives
on the existing late-mount path. It preserves the current explicit language
and bank mask; it does not edit strings or change the native resource lookup.
The experiment is opt-in with [diagnostics] mod_localization_refresh=1.
It refuses non-null save-manager state (base+0x7ED7B8), missing string manager,
invalid/uninitialized language or empty/unknown bank masks. Native byte guards
and existing pinned executable validation precede calls. No arbitrary runtime
reload or worker-thread call is introduced. Initial-mount path is unchanged.

Regression covers unload-before-load, exact language/mask preservation, and
startup/invalid-state refusal. A freshly started Garage is required to validate
the visible result; no live launch is performed by the agent.


## Local removal fallback regression — November 18, 2026

The user confirmed that the localization fix works, selection persists across
restarts, and a track loads with Pulse Test. A completed race is not established
by that report. Their compatibility requirement is stock-base representation
for clients without a custom car, including older ReRiff/vanilla; matching mod
clients may show the custom identity. Shared replay/leaderboard transport is
**not implemented by this local removal experiment**.

Additional verified original evidence, same pinned Game.exe SHA-256 above:
`gaSaveGameChunkSettings.setChosenCar`, token `0600127C`, RVA `002FD39C`,
26-byte CIL body:
`03 02 20 44 01 00 00 58 4A 2E 0E 02 20 44 01 00 00 58 03 54 02 1A 58 17 52 2A`.
It compares settings+324, changes the selected integer only when necessary, and
sets dirty byte settings+4. It does not change paint slots or ownership.

Intentional ReRiff changes: `ClientConfig/CarFallback.cs` validates this exact
body and uses the existing NativeReelBridge call adapter to register a rooted
Cdecl callback to the original setter. `ExperimentalMenu/car_fallback.h` runs
on the existing main-menu init/tick thread, with readable-memory and bounded
count/selection guards. A retained `car-origins.ini` maps `[slot_15] base=alf`
for this isolated test. Only inactive custom slots 15..21 qualify; active,
reused, malformed, duplicate-origin, absent-origin and disabled-settings states
are refused. Supported stock IDs are resolved against current loaded names.
This is not a general registry migration or arbitrary-slot-reordering solution.
No background thread writes saves. The original save lifecycle persists the
setter's dirty state. No paint, ownership or progression write is added.

Automated validation passed:
- `test_car_fallback.exe`: missing custom selection, installed/reused slots,
  invalid counts/indices, missing/duplicate stock origins, read-only planning.
- `CarFallbackSmoke.exe` against `ExtractCarFallbackFixture` output from the
  pinned original: production call adapter and native Cdecl round-trip execute
  original setter IL; only selected index and dirty byte change; all 22 saved
  paints remain intact; repeating the selection does not dirty the save.
- Managed client and development native DLL builds; `git diff --check`.
The first broad fixture attempt referenced .NET 8 types and could not execute
under Wine Mono. The setter-only extractor has no external runtime calls and
passed. These are pointer-normalized execution fixtures, not original binary
reconstruction or matching-byte credit.

Installed only into `build/car-clone-test/game`. Receipt:
`build/car-clone-test/car-fallback-install.json`. Existing DLLs, isolated save
and progression checkpoints were backed up before installation. Pulse Test was
moved reversibly to `game/mods/.disabled/zz-pulse-clone-test`. Normal Steam game
installation is unchanged. Live restart/garage/track-loading checks with the
mod absent remain for the user; do not claim those have passed.


## Vanilla car physics inspection — November 19, 2026

Same pinned original Game.exe SHA-256 recorded above. Original IL evidence:
- `gaVehiclePhys.initFromParams`, RVA `0x289058`, takes a vehicle visual
  pointer and starts by reading the shared `s_paramsBase` profile.
- `gaVehiclePhys.phys_interpolateParams`, RVA `0x2829B0`, computes a clamped
  interpolation factor from `calcHorSpeed` and the two parameter profiles, then
  calls `gaVehicleParams.setFromLerp` at IL005D.
- `gaVehiclePhysBase.init`, RVA `0x28ADA4`, IL0122..013A constructs
  `Vehicles/%s/collision` from the visual model's token at +152. IL014A..017B
  checks the resource and calls `elPhysBody.setShape_file`; IL0188 is the
  alternative `loadHzascCollision` path. Therefore collision selection is
  model-dependent, not just a rendered cosmetic asset.
- Archived vehicles.cfg and vehicles_PC.cfg have shared base/fast profiles,
  not separate named stock-car tuning tables. This alone does not prove every
  aspect of driving is identical. Collision geometry can matter; no measured
  advantage or fastest-car ranking has been established.

Original vfs.zip collision resource provenance (unmodified depot):
- `graphics/vehicles/alf/collision.hzc`: 2530 bytes; SHA-256 `2024a534830bcf4d62a37705a1e98ee34d2fb2c1cb321e32042f97fba549a564`.
- `graphics/vehicles/f1r/collision.hzc`: 3436 bytes; SHA-256 `d91503dbe8a28aa64ed45bda355e8c8b2a52bad786e268c6f99549df38ee9faa`.
- `graphics/vehicles/mer/collision.hzc`: 3064 bytes; SHA-256 `8a4b55d7afe3a7fc826aa4e0f95bfc4346b6a850c0d1b13df71e7e238ebd0a89`.


## Stock collision mesh width comparison — November 19, 2026

Read original depot vfs.zip collision.hzc files: vertex count uint16 at +2,
triangle count uint32 at +4, packed float XYZ vertices at +20, followed by
uint16 triangle indices. All triangle indices were validated against vertex
counts for the 15 selectable stock cars. Width below is referenced-vertex
max(X)-min(X) in unscaled asset units, not a measured runtime broadphase AABB
or gameplay clearance. Bullet margin, orientation and collision response are
not measured here. Original US localization maps mot=Harrington, alf=Pulse,
rs1=Comet, fab=Photon. Unused dod is excluded.

| ID | Mesh width | Original HZC SHA-256 |
|---|---:|---|
| mot | 1.892506 | `2bac0d0c537bef64ec0f885c82f0300c8c1d9247877013086536e1827420c75a` |
| alf | 1.902730 | `2024a534830bcf4d62a37705a1e98ee34d2fb2c1cb321e32042f97fba549a564` |
| rs1 | 1.965159 | `b635e44956591dee01a793627c62306d5fdb5b53b1b0110f978c280a77f58d5c` |
| rol | 2.032395 | `225486c328f767576bd2fcec722c191ef0f6cf1d5089ed373d303fd5c5261d1d` |
| pha | 2.049212 | `8b1c59a379b6c4467f30d40e806c357644e82552bda309f7a60aa27f3aaa4f20` |
| fab | 2.052179 | `be15102cb9af2919e6ba700a5a54417d730ad7ff4ae3f1bdee7edf8aab148bef` |
| f1r | 2.055439 | `d91503dbe8a28aa64ed45bda355e8c8b2a52bad786e268c6f99549df38ee9faa` |
| cor | 2.081879 | `b13e709daddce1032176ce3a52470191a1db36166159e836ca67d210b3689449` |
| pl1 | 2.115412 | `ebd1c788d1dc2390e49e6fbaf9b65a160e531cb2aef4af79fae7f4755906ee76` |
| bui | 2.157183 | `9ba235d94842c79ab9904c0ca19f539bc8439f7ea67eed09804a8b33e1dd9a7c` |
| che | 2.186788 | `81b5f349e8ef0161598810c6abca2a41354454f75624f506e5670bc8b383d948` |
| vau | 2.197418 | `54effceedc2de59f1aac7f78e21daa76e025098cae8ebf50128e83e28c867023` |
| cad | 2.220668 | `28caa313e45c4bef13dc1f0ecc732d03e176409ce3faf3460d9c12a1949518f9` |
| mer | 2.220668 | `8a4b55d7afe3a7fc826aa4e0f95bfc4346b6a850c0d1b13df71e7e238ebd0a89` |
| hon | 2.422241 | `eb60489878e6f7016765c89bc6918c0c4a2282cd0b19aa5fc0f67375123d0daf` |


## Local removal result and shared-ghost test build — November 19, 2026

User reported the removal regression works. Logs at 12:53:21 UTC confirm
missing slot 15 selected `alf` slot 0 through the original setter; the 22
paint/ownership/driven snapshots were identical before and after. Settings
became dirty and later clean. No fatal/unhandled markers were found in the
latest Proton log. This is evidence of the local selection fallback, not
shared replay compatibility.

Original replay evidence, same pinned executable SHA-256:
- `gaReplay.writeToFile`, token `0600068F`, RVA `0021FE00`, body SHA-256
  `0f19584fd1cd34ac7134c88677a14303bfe59719e53a402d24590b38ddb250e4`.
  IL0159..0165 writes the explicit recorded payload size, then closes.
- `gaReplay.readFromFile`, token `06000690`, RVA `00220C40`, body SHA-256
  `96157b9677cf9b0750271dbbd6e7c96b36292211a9ea4729088f26ecc0ee977e`.
  IL007E..0092 reads car and pack. Header-only returns after closing at
  IL009F..00B1. Full read uses the declared payload count at IL0138..0144,
  closes at IL0145..0155, and resets the replay streams. It does not consume
  trailing extension bytes. This code inspection supports the proposed
  extension; an actual vanilla-game playback remains a live test, not a claim.

Intentional opt-in experiment (`car_ghost_test=1`, expected archive SHA-256 in
`car_ghost_mod_sha256`): only the known Pulse Test `dod`/two-paint layout is
supported. After successful native local-device write/close, the managed test
changes the three-byte car header to stock `alf`, retains pack 0/1 (valid Pulse
paints), and appends a 92-byte extension. The extension records the custom ID,
paint, original length, expected mod archive SHA-256 and canonical body SHA-256,
with fixed markers. This checksum is integrity/identity matching, not sender
authentication. All original stream header and movement/scoring payload bytes
are preserved. Publication uses an atomic replacement and keeps an original
recording backup. Existing staging files cause refusal, not overwrite.

On read, the original file stays canonical. Only when native slot 15 is loaded
as `dod` with two packs AND the active Pulse Test archive matches the expected
hash does the native read wrapper overlay the custom car/paint bytes in the
caller's header buffer. Arbitrarily split reads are supported. Missing/wrong
mods and invalid extensions retain the stock header. Legacy metadata `car`
values are mapped to `alf` at the existing Parse text bridge under this flag.
No race simulation, recorded motion or score changes are introduced.

The test extends the already guarded local save-device I/O in
`progression_domain.h`; its extra read hook is only installed if the test
callback is registered. Existing save isolation and native serialization stay
in place. This build is restricted in scope to the isolated test setup with
progression isolation active. It is not a general custom-car distribution or
registry solution, and public-server, preview UI and older-client playback
remain unverified. Conversion refusals are logged; do not distribute a test
recording until the stock header and extension have been checked.

Validation passed under the isolated Wine fixture prefix:
- `test_ghost_car`: every read chunk size, original I/O forwarding, publication
  after close, failed writes/opens, disabled hook and offset bounds.
- Existing `test_progression_domain`: original save routing/copy safeguards.
- `GhostCarFormatSmoke`: stock header accepted by existing CachedGhost parser;
  same/different/absent mod, corrupt extension, bounded/truncated input,
  idempotency, unchanged source and payload.
- Real stock Pulse cached ghost (219134 bytes) copied read-only into the build
  fixture: temporary identity changed in memory, encode/restore reproduces
  every original body byte. Original user's cache was not modified.
- `GhostCarIoSmoke`: atomic replacement, preserved original backup, correct
  native patch layout, read-only restoration and failed-write preservation.
- Native DLL and both managed helpers built without warnings/errors.

Staged only in `build/car-clone-test/game`, with saves, INIs and replaced DLLs
backed up. Receipt: `build/car-clone-test/ghost-car-test-install.json`. Pulse
Test is re-enabled; no prior ghosts existed in this isolated cache. Next live
step: select Pulse Test and finish an unplayed race (or beat its personal best)
to trigger native ghost recording. Then inspect the produced header/extension
before testing the same recording with the mod absent or restoration disabled.

## 2026-11-19: shared-model experiment deferred; stock fallback only

The isolated live run completed a race with slot 15 (`dod`, Pulse Test).
It recorded score 156120 and saved the selected car cleanly. The first embedded
singleplayer FileData ghost had stock `alf` plus a valid 92-byte identity trailer.
A second copy differed only at car bytes 47–49 (`dod`), invalidating that trailer's
body digest. The local cached replay then acquired a second 92-byte trailer.
Thus the low-level read overlay also affected upload/cache reads; it was not
scoped exclusively to rendering. This invalidates any broad claim of safe
live model sharing from the earlier format and hook fixtures.

The user chose stock fallback only and deferred model sharing. The active
opt-in Pulse Test adapter now canonicalizes `dod` to stock Pulse `alf`, never
emits an identity trailer, and never returns a custom-car read overlay, even
when the same mod is installed. Metadata retains its stock mapping. Local
selection/model support and missing-mod save fallback are separate and retained.
This remains the known Pulse Test mapping, not a general custom-car registry.

Old experiment trailers can be stripped only after checking their markers,
length, paint and canonical-body SHA-256. Nested trailers are validated one at
a time; unknown/corrupt trailers are refused. Source test files remain untouched
by the automated validation. The deferred implementation snapshot is retained
locally in `build/ghost-fallback-only/deferred-sharing/`.

The new helpers build for .NET 4.0. Format and file-I/O regressions cover no
identity restoration, idempotent stock publication, unchanged replay payload,
backup preservation, before-open canonicalization and existing staging-file
protection. The actual local and embedded-server files from the live run are
additional read-only regression inputs. A new live fallback-only run remains
pending; no legacy-client rendering or public-server playback is claimed.

## 2026-11-19: fallback-only live validation completed

This section supersedes the pending-live-test statement above. After installing
fallback-only helpers in the isolated test, the owner completed two fresh races.
Native logs record completed-race checkpoints at 14:08:31 and 14:13:25 UTC.
Both locally cached ghosts and both corresponding embedded-server FileData
copies have car `alf`, paint 0, and no `RRGCAR1` markers. The local/server copies
match byte-for-byte. Comparing each file to its backed-up native `dod` recording
shows exactly three changed car-ID bytes, with identical length and unchanged
replay payload. Slot 15 remained selected; native save dirty flags returned to 0.

| Score | Bytes | Canonical local/server SHA-256 |
| --- | --- | --- |
| 26900 | 61644 | `fb98f1007cde781fb32fda98a4f806cb7dd8e76c39cc992f916373e877e4a10a` |
| 115057 | 172294 | `bed460900cede6ece18f2dda1b24d4d17c01d4ac296033695597e656533d1448` |

Local audit: `build/car-clone-test/fallback-only-latest-results.json`.
These are generated test artifacts, not original game reconstruction bytes.

The owner subsequently ran Pulse Test in the normal game against the test server
whose address ends in `.211`. Client logs record fallback publication at
14:24:32 UTC and completed-race checkpoints at 14:24:38 and 14:26:45 UTC.
This confirms client-side publication ran during a networked test; the remote
server's stored file and another/older client's playback were not independently
inspected, and are not claimed verified.

The Goliath startup-name regression in the normal copy was traced to installation
configuration, not packed-versus-loose asset loading. The isolated INI had
`[diagnostics] mod_localization_refresh=1`; the live INI omitted it. Enabling the
existing guarded original unload/load refresh corrected the name according to
the owner's live retest. The live-test install script now retains that setting.
No new native function or byte-match finding is asserted by this configuration
correction. Model sharing remains explicitly deferred.
