# Last.fm race playback observation

## Provenance

Investigated 2026-11-15 against the unmodified Windows depot reference:
`351991/1687655/Game.exe`, 8,635,392 bytes, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the RiffRacerDecomp recovery tree; used the original full CIL export
at `build/unicode-game-module.il` in the parent ReRiff checkout and verified
addresses against the depot executable. No recovery files were changed.
`python scripts/test_lastfm_binding.py PATH/TO/Game.exe` reproduces the binary
checks. These observations do not claim a reconstructed matching implementation.

## Verified original behavior

The `gaRace` vtable at RVA `0x7a399c` has entry slot 2 pointing to thunk
`0x21ef04` (`ff 25` followed by relocated address of `0x7a39f8`), and tick
slot 3 pointing to `0x21f01f` (`ff 25` + relocated `0x7a39fc`).

| Original method | RVA | CIL bytes | CIL SHA-256 |
| --- | --- | --- | --- |
| gaRace.enter | 0x21ede0 | 280 | 4521db221be8afca6d23b986d90c017a06e15b7695dbff2606c77c98b8266286 |
| gaRace.tick | 0x21ef88 | 139 | 4500257b516c84c81b5ee38a85b93246925539479c7a3250018430262c990ef1 |
| gaAudioManager.getMusicFileDuration | 0x2c8dc0 | 28 | 3c98c9c4f27e4aa12c24f622fd02104670edec280c4d588b69c60c664d48abb2 |
| gaRaceManager.setPaused | 0x21f3d4 | 23 | f99cc9dd54ac5527ee0828120158450a51709f801354326a5794f9f5754f733d |
| gaRaceManager.onNode0Hit | 0x21f358 | 112 | fe33f61dcff3e75caa8cf5df11e7f386babecac65c48aa0c393a4709be28e079 |
| gaSaveGame.writeSongCache | 0x2f6f18 | 722 | 52358576c6786b8e468afb8137477a2893c4def6f2b1e08364a367ce9de3bd06 |

`gaRace.enter` explicitly invokes `gaRaceManager.restart(true)` for prior state
IDs 45 and 50. Other entries must not automatically be classified as new plays:
they include return paths that retain race progress. The enum names have not
been independently recovered here.

`gaRaceManager.setPaused` writes byte +12. `onNode0Hit` sets byte +33;
`reset` clears it. Tick stores the previous race time at +24. `gaRace.exit`
(RVA `0x21ecf0`, 21 CIL bytes) calls `gaAudioManager.pauseMusic`.

`gaAudioManager` owns the music source pointer at +100. The original duration
getter calls its vtable slot 5 and returns zero without a source. Estimated
music position is float +160. `tickMusic` (`0x2c9030`) advances it only while
playing and +164 is false; `playMusic` (`0x2c95b4`) sets +164 from its
`_silently` argument. `pauseMusic` (`0x2c8e28`) sets +109, `resumeMusic`
(`0x2c8ea4`) clears it, and `stopMusic` (`0x2c8f08`) zeros position and +0.

`gaRaceConfig` stores the chosen SongID narrow string at +216. The save's local
song vector occupies +176/+180/+184, stride 200. `writeSongCache` serializes
strings at +0, +24, +112, +136, +160, +88, +48, then 29 scalar bytes.
The existing v19 catalogue projection identifies +112/+136 as title/artist.
The scrobbler reads that structured metadata; it never parses a filename.
Narrow strings use inline storage below capacity 16, otherwise a pointer, with
length +16 and capacity +20.

## Intentional ReRiff integration

The original Last.fm path supplies metadata lookup; no account-linking or
scrobble submission lifecycle was found. New API/session/queue code is therefore
separate, while playback and metadata come from the existing game objects.

After whole-file verification, `LastFmNative` binds the original singleton
fields and duration getter by exact name. `lastfm_scrobbler.h` guards the vtable
and both thunks, calls the originals unchanged, then samples at most four times
per second on the game thread. Completion reuses the existing guarded
`gaResultsTokens.enter` boundary in `progression_rewards.h`, independently of
singleplayer progression eligibility. Network and queue I/O run on a worker.

The policy excludes paused intervals, stalls over two seconds, and position
jumps exceeding elapsed wall time plus 0.5 seconds. It detects new SongIDs,
backward positions/timers, pre-start resets, and the verified restart entries.
These are conservative listening observations, not a new audio implementation.

## Validation and remaining uncertainty

Offline managed tests cover signing, UTF-8 forms, completion/percentage and
Last.fm minimums, pauses, seeking, duplicate suppression, config preservation,
queue restart/account isolation, backoff and invalid sessions. Native fixtures
check guard rejection, original forwarding, callback throttling and completed
race notification outside progression mode. No real scrobbles were fabricated.

Live browser linking succeeded on 2026-11-15. The game log recorded a scrobble
accepted at 22:34:07 UTC, before completed-race results at 22:34:57 UTC, with
the 80% setting; no queued tracks remained. Broader listening scenarios still
need live coverage. A hard exit
before the worker saves a newly eligible track may lose it; an ambiguous network
acknowledgement can cause a retry using its original timestamp. The observer is
intentionally conservative around stalls and corrupt/missing metadata. This is
an intentional ReRiff patch, not byte-for-byte original recovery.
