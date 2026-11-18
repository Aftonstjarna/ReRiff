# Original ghost-win message and prestige reward quote

Reference: Windows Steam depot 351991/1687655 Game.exe, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the local RiffRacerDecomp recovery documentation; this specific
message/payout split was not documented there. Evidence is original CIL,
metadata and vtable bytes, not a matching reconstruction claim.

## Original dynamic message

`gaResultsVersus.initScreens`, MethodDef `0x06000769`, RVA `0x232cec`,
12-byte fat header, 1541 CIL bytes, SHA-256
`7458430ba32cf2d9ee57c3aa25bbddeeb7ac8ef7cfb6ae50746fc6a473f6c1ac`.
The original vtable at RVA `0x7a4048`, slot 15, points to thunk `0x2333dc`;
its original preferred-base bytes are `ff 25 c0 40 ba 00`, fixup `0x7a40c0`.
The signature is native thiscall: menu, copied screen-SSP pointer, copied
screen-overlay-SSP pointer; return void.

After a strict ghost-score win, CIL +0x511 resolves localized
`RESULTS_VSBeatOpponent`, then +0x52b reads field `0x04000747`,
`?s_tokensBeatOpponentGhost@gaResultsTokens@@2HB`, and +0x530 calls the original
vararg `elStringManager.c99_snprintf`. The original raw text item is stored
at menu +292. This is dynamic localized text, not baked into artwork.

The field has RVA `0x5301d0`, int32 initial value 100, attributes Assembly,
Static, HasFieldRVA, with `modopt(IsConst)` but neither Literal nor InitOnly.
The original read at RVA `0x233223` is exactly `7e 47 07 00 04`.
A private 32-bit Microsoft CLR fixture copied those field attributes/type
modifiers/initial bytes and confirmed a no-inline getter sees 100, 200, 300,
and restored 100 across repeated calls. This tests RVA/JIT behavior, not the
entire game formatter or every CLR implementation.

## Independent original payment

`gaResultsTokens.enter`, MethodDef `0x06000752`, RVA `0x22e880`,
970 CIL bytes, SHA-256
`c59f45536fc924d7d3f43b33a6395f54c6da4091eacf01437db7506725f49f49`.
Its ghost-win branch adds the embedded `ldc.i4.s 100` at CIL +0xad
(`1f 64`), not a read of the versus display field. A strict score comparison
is retained. The reward is therefore 100 in the original game, not 50.

Local live prestige-1 trace: original combined reward320, adjusted race440
plus bonus200, credited640, previous balance2260, saved balance2900.
The versus banner still advertised100 before this correction.

## Intentional ReRiff extension and validation

A guarded original-versus initScreens wrapper temporarily quotes that field
while the native method builds the localized message, then restores its value
and page protection. No replacement text, artwork or screen is introduced.
The managed callback uses the race coin rate, prestige growth, multiplier cap
and floor rounding; the creator rate is separate. Inactive/unready domains
forward original values. Payment remains the existing single scoped transform.
The advertised bonus is nominal, as in the original: near the signed-int bank
ceiling the subsequent combined payment may be limited by remaining capacity.

Native regressions cover repeated 1x/2x/3x quotes, restoration on normal return,
read-only page protection, rejected quotes, inactive/unready bypass, argument
forwarding and altered original-byte/value refusal. Managed tests compare the
quote with the race bonus at default and fractional/capped custom policies.
The original-field CLR fixture and production builds pass. The new banner
still requires an in-game live check; no appearance claim is made yet.

## Live verification

The user fully restarted Singleplayer and confirmed the versus message shows
200 at prestige1 after a Leader win. Trace quoted original100 to200; the later
original150 combined award was transformed once to300 (race100 plus bonus200),
from balance3200 to3500. Checkpoint21 and native checksum-valid progression
contain3500 coins, cycleXP1110642, permanentXP62985806, prestige1 and retained
Friend or Foe. Manifest, active snapshot and working save hashes match; the
original Steam save remains hash-identical. This supersedes the pending live
banner check above. Custom balance policies and other platforms were tested
in focused fixtures, not in this live session.
