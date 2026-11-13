# Shared Unicode text and glyph tracing

## Provenance

- Original reference: Steam depot 351991/build 1687655, `Game.exe` from
  `/var/home/afton/Applications/SteamDepotDownloader/depots/351991/1687655/`.
- SHA-256: `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
- Addresses below are original preferred VAs (image base `0x00400000`);
  ReRiff uses the corresponding RVAs at the actual loaded base.
- Evidence: original PE imports and x86 instruction disassembly, followed by
  tests executing the original glyph lookup in an isolated Wine process.
  Function labels here describe observed roles, not certified reconstructed
  source names. This note makes no matching-byte reconstruction claim.

## Verified original behavior

1. The text setter at `0x00450930` calls the shared font layout routine at
   `0x00455520` through the original `E8 87 4B 00 00` call at `0x00450994`.
   The layout entry starts `55 8B EC 83 E4 F8` (three complete instructions).
   Its stack arguments include an output pointer at `[EBP+8]` and text at
   `[EBP+0xC]`; the font object arrives in ECX.
2. At `0x00455790`, `0F B7 04 41` reads a 16-bit character from
   `[ECX+EAX*2]`. The call at `0x00455797`, `E8 C4 16 00 00`, passes this
   character and an optional byte-output pointer to `0x00456E60`.
3. The glyph lookup entry begins `55 8B EC 56 57` (four complete
   instructions). It takes the font in ECX, a character in the low 16 bits of
   `[EBP+8]`, and the optional byte-output pointer at `[EBP+0xC]`; it returns
   a glyph pointer in EAX and pops eight argument bytes (`C2 08 00`).
4. Glyph records span 44 bytes (`0x2C`) and begin with the character WORD.
   The primary record vector is at font offsets `+0x28/+0x2C`. If no glyph
   matches and the byte-output pointer is supplied, the routine also searches
   the font referenced at `+0x68`, setting the byte output to one on a match.
   Otherwise it returns the default glyph pointer at `+0x34`. CR/LF return
   the special record at font `+0x38`.
5. Original imports relevant to the initial source trace include
   `FindFirstFileW` (IAT VA `0x0092D078`), `FindNextFileW`
   (`0x0092D068`), and `MultiByteToWideChar` (`0x0092D10C`). Their presence
   does not prove every metadata or filename path uses them.

The renderer therefore already accepts UTF-16 code units and has a native
alternate-font lookup route. This does not establish Japanese glyph coverage
or correct decoding at every upstream text source. In particular, the narrow
byte-oriented cache update later in the text setter is not evidence that the
layout routine itself accepts a narrow string.

## Intentional ReRiff diagnostics

`ExperimentalMenu/unicode_trace.h` adds opt-in observation only. It forwards
the original text, glyph results, import arguments and return values unchanged.
Native entry hooks check exact instruction bytes on the hash-supported game.
The font hooks preserve flags, general registers and x87/SSE state around
logging; Win32 wrappers preserve the original call's last-error value.
Main-module IAT replacements require the existing resolved import to match the
expected kernel32 export. Other modules and unmatched imports are left alone.

Enable in the game folder's `server.ini`:

```ini
[diagnostics]
unicode_trace=1
```

Restart the game. The trace is written to `logs/ReRiff-unicode.log`, recreated
on each enabled launch. Set the value to zero and restart to stop tracing.
No font or encoding fixes are applied. This diagnostic is disabled by default
in shipped configuration; local test installation explicitly enables it.

Records cover interesting filename enumeration results, conversion input
bytes/code pages/output UTF-16, shared layout input, and requested versus
returned glyph character codes. Non-ASCII text and literal question marks
are recorded. Strings are bounded to 256 UTF-16 units, byte captures to 128
bytes, duplicate records suppressed, and output capped at 4 MiB/4096 records.
Records contain local filenames and visible text and are local diagnostics.

Interpretation:

- A source filename containing `U+65E5` followed by layout text containing
  `U+003F` indicates loss before layout; more tracing can isolate that conversion.
- Layout/glyph requests containing `U+65E5` with a returned `U+003F` glyph
  identify missing glyph coverage for that font path.
- A matching requested/selected character still leaves artwork, metrics and
  texture rendering to inspect if the visible result is wrong.
- Absence of a source record is not evidence of correct decoding: managed
  conversions and internal CRT conversion paths are not all instrumented.
- Supplementary characters remain UTF-16 surrogate units in these records;
  support for surrogate-pair shaping has not been established.

## Validation

The dedicated native harness loads the original executable without resolving
its startup graph and executes the actual glyph lookup with controlled font
records. It checks primary, alternate and default glyph returns; optional
output-byte behavior; exact-byte hook refusal; layout argument/return forwarding;
SSE and last-error preservation; UTF-8 conversion and conversion failure;
Japanese filename enumeration; and duplicate suppression. An entry-compatible
stub checks layout forwarding without initializing the renderer or drawing.
The existing menu regression suite checks behavior with diagnostics disabled.

Actual text-source coverage, selected font objects and missing Japanese glyphs
must still be established by an in-game trace on the affected content. No
game-wide Japanese rendering fix or native Windows trace has been claimed.

## First in-game trace: 2026-11-11, Proton

The user loaded `BUCK‐TICK - くちづけ.mp3`. The local trace snapshot is retained
privately under ignored `build/unicode-traces/`, SHA-256
`049732d38d04d03d0d0c555a2d2c152d74c0475cf2ec6136ed64a1720d259cdf`
(8,823 bytes, 58 lines). The complete log contains local filenames and is not
part of this submission. Source tags were inspected read-only from the MP3;
ID3v2.4 text encoding byte 3 identifies UTF-8 for TIT2, TPE1 and TALB.

- Source title and album tags retain `U+304F U+3061 U+3065 U+3051`.
  Source artist retains `U+2010` between the Latin band-name components.
- Trace line 26 records the complete filename from FindNextFileW with all
  these UTF-16 units intact. Line 28 records the same filename at shared
  layout, also intact. Thus filename decoding is not the first failure for
  these characters in this test.
- Lines 18–21 record requests `U+304F`, `U+3061`, `U+3065`, `U+3051`, each
  returning the glyph whose character is `U+003F`. Equivalent failures occur
  for several other font objects in the Now Playing and song-information
  text paths (lines 33–36, 44–47, 50–53). This establishes missing glyph
  coverage on those tested paths, rather than pre-layout loss of the kana.
- The filename's `U+2010` also reaches layout intact but returns a `U+003F`
  glyph (line 13). Separately, artist labels at layout already contain a
  literal `U+003F` in place of that hyphen (lines 32, 48, 54), despite the
  source TPE1 tag retaining `U+2010`. This establishes an additional loss on
  the metadata-to-display path; its exact conversion site remains unknown.
- ACP was 1252 and OEMCP 437. The trace contains no MultiByteToWideChar
  records for this sample. This does not identify or rule out internal or
  managed metadata conversions. It is insufficient evidence to claim a
  specific ACP conversion is responsible.

This sample exercises hiragana and a Unicode hyphen. Katakana, kanji,
supplementary characters, other UI paths and native Windows remain untested.
The native alternate-font mechanism is a candidate integration point for a
future glyph-coverage patch; no such patch is included in these diagnostics.

## Second in-game trace: kanji and katakana

The user next loaded `BUCK‐TICK - 月下麗人.mp3` and
`BUCK‐TICK - ドレス.mp3`. The second private snapshot has SHA-256
`ab36580c6e748c616ca77baf26f50b4efeab5def256e5263d5a10d11f1c76a52`
(12,302 bytes, 84 lines). It is retained under ignored `build/unicode-traces/`.

- Katakana `U+30C9 U+30EC U+30B9` survives filename enumeration (line 27)
  and shared layout (28). Each returns the `U+003F` glyph (29–31).
- Kanji `U+6708 U+4E0B U+9E97 U+4EBA` survives enumeration (56) and layout
  (58). Each returns `U+003F` (59–62).
- Both titles also reach Now Playing and song-information layout intact.
  Equivalent missing-glyph records occur on those paths (35–52 and 63–81).
- Artist display again contains literal `BUCK?TICK`. No interesting imported
  MultiByteToWideChar call was observed in this trace.

Together the two traces establish missing glyph coverage for tested hiragana,
katakana and kanji across the picker, Now Playing and song-information fonts.
They do not establish coverage of every game screen, the entire Japanese
character repertoire or native Windows behavior. The preceding paragraph's
katakana/kanji limitation describes the first sample only and is superseded
for these specific characters and paths by this second sample.
