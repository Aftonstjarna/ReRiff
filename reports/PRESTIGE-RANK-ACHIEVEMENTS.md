# Native Rank achievement checks and prestige integration

Reference: unmodified Windows Game.exe, depot 351991/1687655, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted local recovery `native/PE-MENU-RELOCATIONS.md`; progression-specific
behavior below comes from the hash-pinned original CIL, not a matching rebuild.

## Original evidence

`gaDriverLevel.checkLevelsAchievements`, metadata token `0x06001039`, RVA
`0x2CBDAC`, has a 12-byte fat header `03 30 03 00 A0 00 00 00 00 00 00 00`.
Its 160-byte body has SHA-256
`b87e6da4ed34b9ef3c8142aa74d10283cf163678c5fa5c689e73000b1aee3ee7`.
It has MaxStack 3, no local signature and no exception clauses.

Four repeated blocks test achievement IDs 10, 11, 12 and 13 against integer
levels 5, 10, 15 and 20 respectively. Each first checks
`gaAchievements.isAchieved` (token `0x06000E81`), skips an already earned flag,
then calls `gaDriverLevel.getDriverProfileLevel` (`0x06001036`) with null
explicit-XP input. `conv.r8; conv.i4` converts the result before a lower-bound
comparison. A satisfied block calls `gaAchievements.setAchieved`
(`0x06000E80`) with true. The manager comes from field `0x04000DB1`.
The four level calls start at body offsets +0x10, +0x37, +0x5F and +0x87.

`gaDriverLevel.addPlayerXP`, token `0x06001038`, RVA `0x2CBEA4`, first samples
the original cycle level, adds native XP, calls the Rank checker at body
+0x1E, then samples level again and optionally awards native level-up coins.
Those level calls are at +0x6 and +0x25. Rank checks therefore need not wait
for a whole level increase. This is an award-time check, not a new level-up
notification requirement.

The existing ResultsTokens enter lifecycle (vtable RVA 0x7A3F10, slot 2,
thunk RVA 0x22ECEC, fixup RVA 0x7A3F90) is already guarded by ReRiff's scoped
race-reward integration. It provides the completed-race results boundary
without adding a new event or timer.

## Intentional ReRiff policy

The user requested Rank milestones follow permanent main level after prestige
and be rechecked after a completed race rather than waiting for the next main
level. For example, main level 33 qualifies for all four original milestones.

The candidate leaves the original Rank CIL and global level getter untouched.
It re-emits the verified Rank checker into a separate dynamic method, preserving
its thresholds, existing-flag checks and original award calls. Only its four
level-getter calls select permanent XP in a ready, isolated post-prestige
profile. Original cycle XP is never temporarily replaced by permanent XP.

The existing native race-results hook invokes that recheck before its coin
payment context. Creation alone does not invoke the extra recheck. Subsequent
race checks are idempotent through the original isAchieved tests. Non-isolated
profiles and prestige zero skip the extra recheck. The normal native XP,
unlock, level-up coin and Rank checks remain unchanged for all callers.
Original achievement award handling may still grant its normal rewards.

The callback registration is tested through a cdecl native/managed round trip
on a sentinel pointer without invoking achievements. Exact original Rank
header/body guards are checked before the race hook may call the helper.
This is a ReRiff extension, not matching-byte original reconstruction.

## Validation

`scripts/test_prestige_rank_binding.py` independently reads the hash-pinned
original PE and compares both source guards, milestone call sites and the
original addPlayerXP consumers. Managed regression cases exercise boundaries,
prestige zero, absent isolation and repeated checks. An optional local fixture
uses the extracted original level-calculation IL; its award transport is
explicitly mocked and does not call Steam or touch real saves. Native tests
cover callback registration/ABI, unchanged original checker bytes, inactive
profile refusal, no creator recheck, and the post-race/pre-coin-scope order.

Live Rank re-award passed: the user confirmed all four original milestones
after a completed race at permanent level 33 while cycle level remained zero.
The trace shows one post-race recheck, normal native achievement XP awards,
and the next complete save containing all four flags. Native checksum and
manifest/snapshot/working hashes matched; original Steam save was unchanged.
The ordinary doubled race payout remained correct. A full restart after this
specific correction has not yet been validated. The preceding
published beta's doubled creator/race rewards and their saved balance have
passed a separate live race and full-restart test.
