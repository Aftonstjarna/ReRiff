# Recent Tracks catalogue limit (2026-11-18)

## Original evidence

Reference: Steam depot 351991/build 1687655, `Game.exe` SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Read-only dnlib 4.4.0 inspection of the pinned original, consulted alongside the
RiffRacer recovery project's original strings and current recovered source.
No original-game reconstruction files were modified.

`gaRecentTracksMenu.applySongListFilter`, token `0x0600159E`, RVA `0x363878`:
fat header `03300400c7000000c7140011`, body RVA `0x363884`, 199 bytes,
body SHA-256 `dbd6cfbaaf2e6bd637f2ed5066d2196ebe979856356ab4274ebefb293aa6989f`.
The exact body bytes are retained in ReRiff's `ExperimentalMenu/recent_tracks.h`.

- IL 0000 loads `s_showBundled` (field token 040016F2).
- Bundled=true executes IL 0007–0055: retain records with byte +198 set,
  erase the other records using native vector.erase (06001220), then return
  at IL 00C6. This path does not run the 20-entry trimming loop.
- Bundled=false branches to IL 0057: `ldc.i4.s 20; stloc.0`.
- IL 005A–007E sorts the localSong vector by `sortByLastPlayed`
  (field token 040016F5, sort method 060013F5), with 200-byte records.
- IL 0083–00C4 traverses that vector, decrements local 0 once per entry,
  and erases entries after the first 20. The cap is presentation filtering.

`gaRecentTracksMenu.setupRecentTracksList`, token `0x060015A0`, RVA `0x36394C`:
IL 0000–0012 copies the `gaSaveGame` vector at +176 into a local vector;
IL 0016 calls the filter above and IL 001E calls removeDuplicates (0600159F).
The menu then applies its native sort selection, fills dynamic song/UI vectors
(push_back at IL 042A and 0949), and adds rows with gaMenuReel.addOption at 0986.
The 20-entry erase therefore does not remove entries from the saved catalogue.
It cannot recover songs already absent from that catalogue or missing audio files.

## Intentional ReRiff change, not original reconstruction

At body +0083 (RVA 363907), change `12 04` (ldloca.s local 4) to `2A 00`
(ret; nop). Return immediately after the native recency sort, bypassing the
trimming loop. Keep all offsets and the bundled branch unchanged. Original
sorting, duplicate removal, native row creation and reel handling remain in use.
The supported executable guard and full 199-byte method-body comparison must
both succeed. Changes apply in memory before opening this menu, never to Game.exe
or catalogue files on disk. This is not a claim of matching recovered source.

## Validation and remaining uncertainty

The original whole-file/body hashes and exact bytes were compared. Native tests
exercise every single-byte guard mismatch, exact two-byte mutation, repeated
installation and rejection of unknown/prepatched bytes. The development DLL
build succeeds. In-game scrolling/selection and very large catalogue performance
still require live testing; no arbitrary replacement cap has been introduced.
The native menu creates a row for each retained entry, so loading a very large
catalogue may take longer even though the reel supports scrolling.

## Correction after live crash (2026-11-18)

The initial two-byte return patch above is withdrawn. Live opening under Proton
Experimental produced `System.InvalidProgramException` in applySongListFilter,
reported at IL 0087 (`stind.i4`). Mono validates the unreachable remainder after
the inserted return, where the removed local address load left an invalid stack.
Native byte/guard tests did not catch this; the previous DLL was restored.

The corrected patch changes only body +0099 (RVA 36391D) from `17` (ldc.i4.1)
to `16` (ldc.i4.0). The original counter starts at 20 and now subtracts zero,
so the original keep branch always wins. All original instructions and control
flow remain valid; the bundled branch is unaffected. The guard still compares
the entire original 199-byte body before changing anything.

`RecentTracksSmoke` executes extracted original and corrected CIL under Wine
Mono, substituting native sort/erase dependencies with memory-vector stubs and
using native-width iterator locals. It tests 24 combinations: original/corrected,
bundled/non-bundled, and counts 0, 1, 20, 21, 113, 1000. Original truncates to 20;
corrected keeps every entry; bundled filtering is unchanged. The sort stub checks
its range/call count, not native comparator behavior. This validates the filter,
not complete menu rendering or end-to-end in-game navigation.

## Configurable launcher limits (2026-11-18)

The Graphics tab now exposes All tracks (default), 20, 50 and 100 using
`client.ini` `[graphics] recent_tracks_limit`. Other values fall back to All.
The original `ldc.i4.s 20` at IL 0057 has its signed-byte operand at 0058.
For a finite selection ReRiff writes that operand to 20, 50 or 100 and keeps
the original decrement `ldc.i4.1` at 0099. For All it retains counter 20 and
uses the previously verified zero decrement. The full original method guard
still applies. Instruction lengths, branch targets and stack behavior stay
unchanged. This is an intentional runtime patch, not recovered original code.

The CIL execution fixture passed 48 cases across all four choices, bundled and
normal lists, and 0, 1, 20, 21, 113 and 1000 entries. Bundled filtering remains
unchanged. Native patch tests cover configured choices and invalid-value fallback.
