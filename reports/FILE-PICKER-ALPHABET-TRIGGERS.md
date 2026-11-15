# Music-picker alphabet triggers

## Provenance

Original Windows Game.exe from SteamDepotDownloader depot 351991/build 1687655:
SHA-256 `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`, 8,635,392 bytes. Recovery notes and the previous failed wraparound investigation were consulted. This is research and an intentional ReRiff integration, not a byte-matching reconstruction.

## Existing keyboard path

`gaFileSelectInterface.jumpToSelection`, token `0x06001053`, RVA `0x2CEC68`, has a 12-byte header and 518 code bytes. Original CIL SHA-256: `27868fc02f3bdb306399a688ccbc08628148349407a3a5f98702affd6d8a6545`. It finds the first entry matching an uppercased initial and owns reel movement, visibility and native focus assignment. Its caller is `tickInput` (`0x06001055`, RVA `0x2CFC00`, 763 bytes; CIL SHA-256 `9ffadb98e2b68f220dd17e63df92a5d56c60362d63809c105817f17c467ae32b`), whose A-Z keyboard loop calls this method at CIL +0x2E5. Successful keyboard jumps stop further base input for that frame.

The source-entry vector is at picker +276, with 28-byte directory entries. The first 24 bytes are native wide strings: length +16, capacity +20, inline wchar storage when capacity <8, otherwise pointer +0. Native row SSP vector is +324. Screen is +184, focused-item pointer screen +20, action ID item +8; directory row IDs start at 4. Reel is +248. These offsets were compared with the original CIL, not inferred from screenshots.

## Trigger path

Original input-manager singleton at image +0x80D410: vtable slot1 gets device0, device slot19 gets its X360 interface, controller slot8 reads a trigger. Getter RVA 0xF8180 is thiscall float(void*, unsigned), index0 LT/index1 RT, exact bytes `55 8b ec 8b 45 08 d9 44 81 38 5d c2 04 00`. This agrees with the earlier native manual-speed trigger research.

## Intentional integration and validation

ReRiff binds the unchanged managed jump method only after checking its token, signature and complete original CIL digest. It observes triggers through the existing picker tick integration. A neutral-before-press latch emits one event per press; simultaneous triggers do nothing. LT selects the previous available ASCII initial, RT the next, skips absent initials and stops at ends. Non-Latin/nonletter groups remain accessible with original navigation. No list reordering, folder opening, direct focus clearing or custom reel movement is introduced.

The current build has navigation compile-disabled and only logs context and candidate targets. Native regression tests passed under Wine for wide-string layout, available-letter masks, gaps/ends, trigger thresholds, held/both-trigger handling and malformed source guards. This does not yet establish live controller layout, active menu phase, callback invocation safety or navigation behavior. A live observation pass is required before enabling the callback.

## Live observation and navigation candidate

The observation-only build loaded the verified keyboard callback successfully. A live controller test recorded LT 0.913..1.000 and RT 0.618..1.000, separate correct directions, and native phase 1 with valid focus IDs. The active folder changed between 10, 22 and 490 entries; for focus32 the current initial was A and RT selected B. At C with no earlier alphabet group LT selected no target. No focus or list mutation was performed in this pass.

The next candidate enables the original jump callback after each original picker tick, only for observed phase1 and a valid focus ID. Tests cover dispatch, transition/no-focus refusal and invalid letters with a synthetic callback. Actual native callback invocation and its visual result still require an in-game test; no success is claimed yet.

## Live navigation result

The enabled candidate was fully restarted and tested in the music picker. The user confirmed letter jumps, highlighting and scrolling work. Both LT previous and RT next were recorded in navigate mode. Original Steam save hash remains unchanged. This verifies the native callback handoff in the tested Wine/Proton client; Windows hardware validation and non-ASCII group navigation are not claimed.
