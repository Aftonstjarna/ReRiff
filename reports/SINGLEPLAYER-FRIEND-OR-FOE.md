# Singleplayer Friend or Foe extension

## Original evidence

Reference: unmodified Windows Game.exe from depot 351991/1687655, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted local recovery native frame/PE notes. Achievement behavior below is
verified original CIL and read-only original asset inspection, not a matching
reconstruction.

The English string bank maps ACH_Title33 to Friend or Foe, with description
Beat a friend's ghost. Original Profile setup (token 0x060013EA, RVA 0x325BBC)
formats titles using stored achievement ID + 1, so this is storage ID32.
ACH_Title20 is Can't Touch This, the recommended-ghost achievement, ID19.
The original members used for this lookup have SHA-256:

- `strings/tokens.dat`: `69fd54fc98331f09fddddcca9b2d42d96d65801996edc689677040104376a05b`
- `strings/common/us.str`: `0f577c4ffab9b391d8de0da053fb711d98d6293ec20a7d4e4e954713d02ec3c6`

`gaResultsVersus.enter`, token 0x06000765, RVA 0x234260, obtains local player
zero from the vector at gaPlayerManager +48. It calls
`gaPlayerLocal.getScoreManager` (0x060005D4, RVA 0x2114F8; player +608), then
`gaScoreManager.getScore` (0x060007AC, RVA 0x23A18C). The score getter applies
System.Math.Floor to score-manager +4. Comparing the raw float instead would
allow false wins near an integer boundary.

At original body +0x464, race config +304 selects ghost kind. Kind1 compares
the player's rounded score against race config +308 and awards ID32 only on a
strict win. Kind2 performs the equivalent comparison and awards ID19. Both
use ble.un to skip losses, ties and unordered NaN comparisons. Normal ghost
selection (`gaGhostSelect.ghostSelected`, 0x0600143E, RVA 0x332898) stores kind2
at body +0x300 and the selected score at +0x30B. No ghost uses kind0.

The versus vtable is field 0x04000770, RVA 0x7A4048. Entry slot2 points to
thunk RVA 0x2347E0, `FF 25 C8 40 BA 00` at preferred base 0x400000, using
fixup RVA 0x7A40C8. The contiguous original scoring/award tail begins at
RVA 0x2346C2 and is 111 bytes through the final return. The extension guards
that exact tail and the relocated slot/thunk/fixup before forwarding entry.

Singleton field tokens: race config 0x040010B6, players 0x040005A0 and
achievements 0x04000DB1. Award methods remain the original isAchieved
(0x06000E81) and setAchieved (0x06000E80).

## Intentional ReRiff change

After original versus entry completes, a matched callback may award missing
ID32 for a singleplayer kind2 ghost win. It uses the original score accessors,
rounding and achievement award method. It does not change race configuration,
ghost selection, the original versus CIL, ID19's behavior, or multiplayer
achievement conditions. Native award handling retains its normal popup,
saving, reward and platform behavior.

Singleplayer scope follows the actual selected internal Singleplayer endpoint,
including fallback, rather than assuming all localhost servers are singleplayer.
It works with or without opt-in prestige/save isolation. Scope is selected
before the existing isolation early return and is cleared for multiplayer.
Existing completion is checked first and preserved: no relock, direct flag
reset, re-import or save rewrite is used to enable or test this extension.
The user's explicit prestige reset remains separate from this award extension.

This is an intentional singleplayer accessibility policy, not a claimed
restoration of the original friend requirement and not matching-byte recovery.

## Validation and limits

Independent `scripts/test_singleplayer_ghost_binding.py` compares the exact
original PE tail, vtable, fixup, score-call tokens and string-bank identity.
160 managed cases cover singleplayer/multiplayer, ghost modes, win/loss/tie,
rounding boundaries, NaN/infinity comparison behavior, retained completion,
other flags and repeat refusal. A local fixture executes the extracted
original score getter and accessor IL; only the reviewed BCL Math.Floor call
is allowed as a numeric external dependency. Award transport is explicitly
mocked, with no Steam or real-save calls in the fixture.

Native tests check entry forwarding before the callback, exact guards and
unchanged original award CIL. Separate endpoint-mode tests cover singleplayer
without isolation and multiplayer clearing. The existing 99 Rank cases still
pass. Live normal-ghost award and saved-flag validation remain pending.

## First live failure and category/lifecycle correction

The first candidate did not earn the achievement in the user's reported ghost
win. Its startup bindings/guards passed, but it logged no actual result
inspection. It accepted only recommended kind2 and depended on versus entry;
the trace does not establish the test's actual ghost category or comparison.

Further original ghostSelected inspection shows its Leader branch writes
kind3 at body +0x297 (RVA 0x332898) and its target score at +0x2A3. The earlier
normal-ghost label was too narrow: both kind2 recommended and kind3 leader
ghosts qualify for the requested new singleplayer Friend or Foe policy. Native
Can't Touch This remains limited to a strict kind2 win; no changed requirement
is claimed for that achievement.

The follow-up also invokes the guarded, duplicate-safe callback at the proven
completed-race token-results boundary, before coin scoping. This works without
isolation and does not depend on versus entry being shown. Trace records the
selected singleplayer mode, result boundary, ghost kind, rounded player score,
target score and skip/award result. Existing completion is still preserved.
200 cases now cover kinds0/1/2/3 plus unknown4. Live verification is pending.
