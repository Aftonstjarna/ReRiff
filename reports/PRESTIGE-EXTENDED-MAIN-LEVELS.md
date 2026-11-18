# Extended permanent main levels above the original cap

Original reference: Windows Steam depot351991/build1687655 Game.exe,
8,635,392 bytes, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the local recovery documentation and the previous verified ReRiff
`PRESTIGE-FEASIBILITY.md` and `PRESTIGE-RANK-ACHIEVEMENTS.md` findings.
This is an intentional ReRiff extension, not original game behavior or a
matching-byte reconstruction claim.

## Original boundary and retained methods

`gaDriverLevel.getDriverProfileLevel`, MethodDef0x06001036, RVA0x2cbcfc,
uses the threshold vector at driver+0/+4/+8 and caps at its last threshold.
The original Profile display calls that helper and uses a fill denominator40
(see `gaDriverProfileMenu.setupDriverLevelWindow`, MethodDef0x060013e6,
RVA0x324080, and the previous feasibility report).

Read-only live observations of the initialized original threshold vector list
41 increasing int32 values. The final thresholds are750,000,000 and
1,000,000,000 XP (levels39 and40), so the final native interval is250,000,000.
These are observed loaded data, not a claim about hardcoded CIL constants.
The extension validates vector length/order and reads the actual final two
values. Below the final threshold it still calls the original getter using
explicit permanent XP. No global native getter or threshold data is patched.

## Intentional extension

The existing unsigned64 permanent-XP checkpoint remains authoritative for main
identity. Extra cycle XP continues adding once to that total. After the original
maximum, the profile/discount helper derives an extended main level from an INI
policy: `main_level_cap` (0=no configured cap), `main_level_xp_step` (0=original
final interval), and `main_level_xp_growth` (default1=constant cost). The kth
extra level costs step*growth^(k-1); cumulative thresholds determine promotion.

The extended calculation uses bounded decimal geometric sums and binary
search rather than walking every level. Identity is bounded by signed32
representation, while saved XP remains unsigned64. Lowering a configured cap
changes display only and preserves all stored XP for a later cap increase.
The original native Profile text, star layout and fill-bar setter remain in use.
Configured-cap bars show level/cap; uncapped mode keeps native fill below40,
then shows progress toward the next level.

Rank achievements still use the original four level5/10/15/20 thresholds and
award methods. Their input is saturated at40 because no additional Rank is
being introduced. Vehicle unlocks and native level-up coin rewards keep using
the unmodified prestige-cycle getter and original table. No Steam API, native
save layout or reset behavior is changed by this extension. Singleplayer only;
server-controlled policies are separate future integration work.

## Validation

The updated hash-pinned original-method extraction fixture passed under the
32-bit Microsoft CLR in an isolated Wine prefix, including native pre-40
calculation, post-40 constant interval, a configurable40 cap and unchanged
cycle memory. The fixture remaps signatures/tokens and stubs saving; it is an
execution fixture, not original byte matching. Existing checkpoint tests show
permanent XP above both the original cap and signed-native range survives
restart without duplication. Pure managed tests cover default thresholds,
fractional/custom curves, caps, extreme XP/policies, independent iterative
threshold comparisons, fill behavior, INI parsing and balance revision.
No user save XP was raised or reset to test a level above40. In-game extended
identity appearance and a real level41 promotion remain untested.

The completed focused run passed3598 curve/policy checks on both Mono and the
32-bit Microsoft CLR,99 original Rank scenarios, and native badge regressions
for41/100/1000/max-int identities at five tested text scales. Config-split and
checkpoint persistence checks also pass. The local helper and three INI defaults
were installed with all saved XP/coins/achievement/checkpoint hashes unchanged.
This installation is still awaiting a user restart/layout check; no actual
level41 promotion is claimed.
