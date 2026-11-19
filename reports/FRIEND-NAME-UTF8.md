# Friend-list UTF-8 decoding

Original reference: Steam depot 351991/build 1687655 `Game.exe`, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.

The local Unicode trace captured `HungryShark\u00E2\u0084\u00A2` at
font layout instead of `HungryShark\u2122`. E2 84 A2 is the UTF-8 encoding
of the trademark character; this is byte widening, not missing-glyph data.

Original metadata and IL:

- `gaOnline.getFriendsList`, token `06001325`, RVA `0030D81C`,
  12-byte header, 675-byte body.
- In its leaderboard-backed friend branch, IL_0194 obtains the narrow name.
  IL_01B0 loads the destination, IL_01B2 the source, IL_01B4 the capacity
  1024, and IL_01B9 calls `elStringManager.strncpyChar` (`060016E8`,
  native RVA `00025C40`). The resulting wide name is copied into FriendInfo.
- `gaFriendsMenu.setupFriendList`, RVA `0032F970`, later copies that wide
  name, clips it at IL_0265 and creates native text at IL_02D3. Correcting
  the name upstream retains native sorting, layout, clipping and ownership.
- The original buffer decoder `util.utf8towcs`, token `060012EA`, RVA
  `0030A294`, has a 12-byte header and 359-byte body. Its third argument
  is UTF-16 word capacity, including the terminator.

Guarded image SHA-256 values:

- Friend method: `aeff41b5d6c74ff0232a6f9d226e7303a37c1d13fd7a6e41ad332ba22b1a62d9`.
- UTF-8 decoder: `d4252a387606004d3a141fb564e07ce53a08f0c4e534e82a96864fc130c15e5f`.

## Intentional ReRiff patch

Replace ten bytes at `0030D81C + 12 + 01B4`:

```
Original: 20 00 04 00 00 28 E8 16 00 06
Patched:  18 1E 62 28 EA 12 00 06 26 00
```

The replacement computes `2 << 8` (512 UTF-16 words), calls the original
UTF-8 decoder, discards its returned length and pads with a nop. The original
1024-byte buffer, subsequent leave instruction, branch targets, exception
regions and account-ID copying remain unchanged. Both full method images
(header plus body) are guarded against the original bytes before patching.
This is a runtime behavior change, not a byte-matching reconstruction.

## Validation and limits

`FriendUtf8Smoke` re-emits and executes the embedded original decoder IL with
ASCII, empty, trademark, accented and Japanese names and capacity-boundary
inputs, checking a guard after the 1024-byte destination. The native fixture
mutates every guarded byte in turn, checks refusal, then verifies that only
the intended ten-byte site changes and repeated installation is harmless.
The client DLL builds with the patch. Actual friends-list rendering still
needs a live retest. Supplementary-plane Unicode behavior in the original
decoder was not changed or validated by this fix.
