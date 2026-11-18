# Singleplayer prestige beta

For the local multiplayer candidate, platform-independent accounts and test
setup, see [MULTIPLAYER-PRESTIGE.md](MULTIPLAYER-PRESTIGE.md).

Available in **1.4.0-beta.1**, disabled by default. This beta adds isolated
singleplayer progression, permanent level tracking and a manual prestige reset.
Multiplayer prestige and configurable server balance are not included.

## Enable the experiment

1. Exit the game and back up its original save and any existing isolated profile.
2. Select **Singleplayer** in the launcher.
3. Add the following to the game folder's `singleplayer.ini`, then restart:

```ini
[progression]
isolate_singleplayer = 1
trace_enabled = 1
```

Enable **Diagnostics → Logging** in the launcher to collect observations in
`logs/ReRiff-engine-menu.log`. Tracing is optional; it does not enable prestige.

On first use, the experiment imports the existing native save once. It keeps
an account-specific local `reriff-sp-<hash>.bin` in the game's native DAT save
directory and a checkpoint bank under `ReRiff-progression/<profile>/`. It
requires an existing writable native save directory. Isolated sessions retain
the original serializer and do not replace the original Steam Cloud save.
Generated tracks and song caches keep their existing locations.

Set `isolate_singleplayer = 0` and restart to use the original shared save.
Progress earned in the experimental profile is not merged back automatically.
Back up or restore the **whole checkpoint bank and its working save together**;
do not mix a save from one generation with another generation's counters.

## Prestige

Earn all **38 original in-game achievements**, then open **Settings → Credits**.
The native confirmation asks whether to prestige. Accept to reset; Cancel or
Back preserves progress. If ineligible, Credits opens normally. Loading,
saving, completing achievements and restarting never prestige automatically.

A confirmed prestige clears coins, cycle XP, progression unlocks and the
original in-game achievements. It retains permanent main XP/level, the equipped
car and paint pack, race records, ghosts, song history and generated tracks.
Steam achievements remain earned. Progression unlocks use the original native
XP thresholds again from zero cycle XP.

Profile shows the native difficulty star beside the permanent level. The first
prestige uses a star alone, then x2, x3 and so on. It appears only on the level
section. First Prestige is stored in the originally hidden local achievement
slot; its logo-bearing achievement-list row is still pending.

Main levels continue beyond 40. Defaults reuse the original 250,000,000-XP
39→40 interval for every later level, with no configured cap. The cap, XP step
and growth are configurable in `singleplayer.ini` under `[balance]`; see
[Configuration](CONFIGURATION.md#main-levels-above-40). Already-earned permanent
XP counts immediately. Prestige-cycle unlocks retain their original curve.

## Rewards

Creator and race coin awards use **prestige count + 1**: ×1 initially, ×2 after
one prestige, ×3 after two. Quoted and credited amounts follow the same scoped
reward path. Purchases and unrelated credits are not multiplied.

Non-IAP paint packs use permanent-level discounts: 10% at level 10, 20% at 20,
25% at 30, then diminishing increases toward 30%. Free packs remain free and
paid packs cost at least one coin. Native signed counters are bounded.

## Validation and limits

Live testing verified the initial import, original-save isolation, native
race/save/restart persistence, a discounted purchase and restart, one confirmed
prestige, retention of permanent level/equipped items, cleared cycle progress,
and the native star's alignment and section visibility. A subsequent restart
retained the prestige checkpoint.

Automated tests cover reward boundaries, interrupted/reset transactions,
checkpoint corruption/domain checks, original native reset/ownership bindings,
confirmation refusal/cancellation and UI lifecycle. The extracted original-IL
fixture is created locally from a hash-verified original executable and is not
shipped. It is a test adaptation, not a byte-matching original reconstruction.

Live doubled creator/race payouts after prestige and their full-restart
persistence have now passed on the published beta.
Power-loss durability is not established. A transaction failure stops gameplay
so startup can recover the last complete checkpoint. There is no multiplayer
authority or anticheat implementation in this beta.

Native research and its corrections are recorded in
[the recovery report](../reports/PRESTIGE-NATIVE-BOUNDARIES.md).

## Local follow-up: Rank achievements

A post-beta candidate rechecks the four original Rank milestones against
permanent main level in completed-race results. At main level 33, that awards
the level 5/10/15/20 milestones together without waiting for level 34. The
native popup queue displays their awards normally. Repeated checks do not
repeat earned awards. Creation alone does not run this extra recheck.

The original global level getter and Rank CIL stay unchanged. Car unlocks,
cycle XP and level-up coins continue through the original cycle path. The
original achievement award method handles any normal achievement rewards.
This follow-up is not yet published. Live testing confirmed all four Rank
milestones after the completed race at main level 33 while cycle level was
still zero. Their saved flags, checkpoint hashes and native save checksum
matched; ordinary race rewards still doubled and the original Steam save
remained unchanged. Full restart after this specific Rank change is not yet
claimed.

## Local follow-up: Friend or Foe

A singleplayer normal-ghost win can also earn Friend or Foe. Beat the ghost's
score; ties and losses do not count. The original rounded score, achievement
award and popup paths are reused. Existing completion on a carried-over save
is retained and repeated wins do not duplicate it. This applies to ordinary
singleplayer too; prestige/isolation is not required. Multiplayer conditions
and the existing recommended-ghost achievement remain unchanged. This local
follow-up is not published and its live ghost-win test is pending.

The first ghost candidate failed a live win test. A follow-up includes both
recommended and leader ghost categories and checks completed-race results
even if versus entry is bypassed. It logs the actual category and rounded
score comparison. The recommended-only Can't Touch This requirement is
unchanged and still needs its own live singleplayer confirmation.

### Preserved native ghosts missing from Singleplayer

A native cached personal best can exceed a later Singleplayer race score. The
original save path then skips its ghost upload, leaving the new local database
without a corresponding ghost. Missing own records can now be imported on the
selector's normal query, using the original replay header and an unchanged
payload copy. Existing ghost/score records and native files are preserved.
Ambiguous owners and malformed or conflicting records are skipped. Leader
selection and replay downloads still use the original game paths. The first
native-queue prototype crashed at startup and was rolled back; the storage-only
revision passes SDK tests and still needs its in-game selector/playback test.
