# Native coin-message audit and level-up quote

Original reference: Windows depot 351991/1687655 Game.exe, 8,635,392 bytes,
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the recovery project's native UI notes and existing ReRiff reward
research. The following observations come from that hash-pinned executable's
CIL and metadata, not a matching reconstruction or proof of every native-code
caller. Local inspection output is kept under ignored build storage.

## Level-up message

`gaResultsXP.tick`, MethodDef `0x06000785`, RVA `0x235758`, formats the localized
`RESULTS_XPLevelledUp` message. Its 12-byte fat header is
`03300600c102000084070011`: MaxStack 6, 705 code bytes, locals `0x11000784`.
Original body SHA-256 is
`0a3a1a812c196786aab6b19137775d55eec39cd4e74b5a64bd9107fa6660836c`.
At CIL +0x1e4 it loads updated level, field `0x04000796`, for the first format
argument. At +0x1e9 the following 12 bytes supply the second argument:

```
11 11 7e 96 07 00 04 28 34 10 00 06
```

This loads local 17 (driver), loads the updated level again, then calls
`gaDriverLevel.getTokenRewardAtLevel`, MethodDef `0x06001034`, RVA `0x1e1514`.
The getter's complete 11-byte CIL is `031a5a021f0c584a584a2a`; it reads the
level-indexed int32 reward table through driver +12. The message formatter is
MemberRef `0x0a000018` at +0x1f5; native text replacement follows at +0x205.

ReRiff intentionally replaces only the 12-byte argument producer with
`dup; ldc.i4 callback; conv.i; calli 0x11000854`. That existing StandAloneSig
is cdecl int32(int32), including the original C++ calling-convention modifier.
The callback receives the duplicated level, calls the unchanged original
reward getter, and applies the same nominal coin-bonus quote as the ghost-win
message. It preserves the first level argument, localization, text item,
MaxStack, locals, method length and all following instructions. Exact header,
original argument, preceding level load and following formatter call are
checked before the startup-only memory patch. Disk game bytes are unchanged.

The quote uses the configured coin rate, prestige growth/cap and floor rule.
Like the original ghost advertisement, it is nominal: the final combined
results award can be bounded by remaining signed-int bank capacity, and native
aggregation occurs before combined bonus rounding. No payment is added here.

## Other native messages and credit sources

The audit enumerated all nine managed-CIL call sites for `incTokens`.

| Message/source | Existing multiplier handling |
| --- | --- |
| ResultsTokens collected / bonus / new balance | Original fields +388/+392 are quoted together before credit and display. Includes level and ghost bonuses. |
| ResultsVersus opponent-win message | Scoped quote of the original mutable 100-coin field during native formatting. |
| TrackCreator creation reward | Scoped creator-rate quote of the tuneable used by both payment and native text. |
| ResultsXP level-up notification | New display-only quote described above. |
| ResultsShare.giveReward / share and confirm messages | Native sharing tuneable +16; currently unmultiplied. Keep native text/payment together. |
| ResultsTokens.onResume | Separate legacy resumed-award path; currently unmultiplied. |
| DriverLevel.addPlayerXP | Optional immediate level-award path, separate from race-result accumulation; currently unmultiplied. |
| AdOverlay.tick / AD_Query / AD_Thanks / PURCHASE_AdText | Legacy advertisement rewards; currently unmultiplied. No ad service is restored by this change. |
| RetentionPopup.handleScreenInput / RETENT_Level1 | Legacy static reward field and localized promotion; currently unmultiplied. |
| PurchaseMenu.handleScreenInput and tick | Debits / purchased currency, not prestige-scaled earned rewards. |

Do not multiply arbitrary positive deltas or every numeral in localized text:
that would also alter purchased currency, prices, XP, levels and statistics.
Any future expansion of the earned-source policy must pair its native message
with its payment, including any legacy static/promotional text. This inventory
does not claim those services are active or tested in the current client.

## Validation

Native fixture checks changed headers, changed reward calls and changed
formatter calls are refused without modifying surrounding CIL. It compares the
same guards against the original depot image. A 32-bit CLR fixture exercises
160 level/quote combinations across the native cdecl callback, preserving both
format arguments. Live level-up appearance remains to be retested after restart.
