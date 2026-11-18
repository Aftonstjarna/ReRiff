# Prestige using original progression and garage systems

Research date: 2026-11-13. Original Windows reference:
`/var/home/afton/Applications/SteamDepotDownloader/depots/351991/1687655/Game.exe`.
SHA-256 verified during this investigation:
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Evidence: local original CIL surveys in `analysis/native-port/il/` and
`analysis/native-port/decompiled/Module.cs`. Consulted recovery project's
`Decomp/native/WORKER.md`; no prestige implementation was located there.
This is feasibility research, not a matching reconstruction or tested patch.

## Requested behavior

Prestige wipes progression except the equipped car and paint. Count completed
wipes separately for each isolated save domain and show `*x2` beside the level
after two wipes. Singleplayer seeds once from the original save; multiplayer
with and without anticheat must have distinct progression domains. Anticheat
is prospective: no server policy protocol or trust enforcement is established
by this research.

## Verified original integration points

- `gaSaveGameChunkProgression.reset`, token `0x06001254`, RVA `0x2FC230`,
  clears progression flags, packed unlocks, car-use counters, XP at object
  +280, currency at +284, and the 64-bit value at +288. It sets +276 and +8
  to -1 and marks the chunk dirty at +4. Names/semantics of every cleared
  flag and the 64-bit field have not been established.
- `gaSaveGame.reset`, token `0x060011E9`, RVA `0x2F56A8`, dispatches reset to
  three chunks and marks them dirty. It is broader than the requested
  progression reset and should not be used blindly for prestige.
- Settings store the selected car at object +324. Original
  `setChosenCar`, token `0x0600127C`, RVA `0x2FD39C`, updates it and marks
  settings dirty. Chosen paint-pack indices are at +236 + 4 * car index:
  getter `0x0600127A` / `0x2FD360`, setter `0x0600127B` / `0x2FD37C`.
  `gaVehiclePaintManager.getPackIndexForCar` delegates to this settings getter.
- Original progression `setCarUnlocked`, token `0x0600125B`, RVA
  `0x2FC7BC`, and `setPackUnlocked`, token `0x0600125E`, RVA `0x2FC818`,
  provide native unlock operations rather than requiring a replacement format.
  See `PROGRESSION-COIN-SAVE-FIELD.md` for version-6 serialization and checksum.
- `gaDriverLevel.getPlayerXP`, token `0x06001037`, RVA `0x2CBCC4`, reads
  progression +280. `getDriverProfileLevel`, token `0x06001036`, RVA
  `0x2CBCFC`, derives the fractional level from an XP threshold vector and
  clamps at its final threshold. Prestige should keep this XP/level calculation.
- `gaDriverProfileMenu.setupDriverLevelWindow`, token `0x060013E6`, RVA
  `0x324080`, calls that level calculation (body offset +0x32A in the survey)
  and builds the native profile display. Its progress-bar factor uses 40;
  that display constant alone does not prove the progression cap is 40.

## Proposed ReRiff extension (not original behavior)

Capture selected car and pack before resetting only progression. Restore their
ownership using the original unlock functions and preserve their selection in
settings. Verify that pack unlock granularity actually represents the equipped
paint; retaining an entire pack may retain multiple paint variants. Validate
indices and entitlement/free-item behavior before committing a wipe. Do not
interpret resetting achievements in local progression as revoking Steam
achievements; platform achievements require separate examination.

No prestige counter was identified in the inspected progression serializer.
Use explicitly versioned ReRiff metadata scoped to the isolated profile rather
than reinterpreting an unknown original field. A reset and count increment
must form one recoverable operation: interruption must not increment twice or
reset progress without recording the wipe. Steam Cloud routing and profile
selection must be proven isolated before applying either change. Servers with
anticheat would need server-authoritative prestige validation; a local counter
alone is not trustworthy.

Extend native level text with the literal `*xN` suffix, preserving original
font/layout. A graphical star requires verifying native artwork or font glyph
availability. Profile, race-results and any other level displays need a caller
audit; the profile window is a verified starting point, not complete coverage.
Check supported resolutions/scales and reuse native layout/scrolling for any
new prestige action and confirmation prompt.

## Remaining acceptance checks

No game/save changes were made or executed in this investigation. A prototype
must demonstrate save/reload persistence, the same car/paint equipped and
usable after reset, expected XP/currency/unlock defaults, untouched settings
and unrelated save chunks, count recovery after interrupted writes, isolation
between all save domains including Steam Cloud, and native display fit. The
meaning of all progression flags and the desired treatment of scores, ghosts,
track caches and platform achievements remain to be resolved before a full
implementation can claim to match the requested wipe semantics.
