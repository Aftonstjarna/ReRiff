# High-rate FLAC analysis duration mismatch

The user's vanilla-engine/offline-patch comparison reproduces the high-rate
FLAC timing problem observed with ReRiff. This is runtime/cache evidence,
not a byte-matching reconstruction or a completed decoder fix.

## Reproduction and results

Source: FLOW — GET BACK, stereo 24-bit FLAC, 96,000 Hz, 195.386667 seconds.
The comparison file was resampled from the source to 44,100 Hz while retaining
24-bit stereo and the same reported duration. The first attempted conversion
was 16-bit, but it was replaced with the 24-bit file before this comparison.

The user generated fresh tracks with original Game.exe plus the moylien offline
backend replacement. The provided patch's three DLL SHA-256 hashes match the
local reference checkout's release files at d07b0cac1751b348c86525d0cd33c782973e900b.
Game.exe reference SHA-256 is
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The result folders were supplied as the user's test output; the live execution
was not instrumented by this investigation.

| Input | Offline-patch song ID | Analysis span (s) | Beats | Packed `_v18` SHA-256 |
| --- | --- | ---: | ---: | --- |
| 44.1 kHz / 24-bit | 9WOzeJjKoq | 192.069656 | 640 | `7e2093c28cc3d933b61181dc22b728604b8477532eab0b7bafa67900433508f5` |
| 96 kHz / 24-bit | XCCPVwwEo0 | 418.150757 | 1282 | `58601692ce28ab7bda436ddb76f9f6f5f33415cb79a87c9973f8ec3a925ac5c9` |

The original ReRiff 96 kHz cache was `eXFoJ1V4AX_v18`, SHA-256
`2af3d3a0de0fb8f9baed881f095d580782ced552aab4a08450f8f23574e7478c`.
Its saved decoded analysis is exactly equal to the new 96 kHz analysis, including
all bars, beats, beat volumes, sections and track fields. Packed-file hashes
are different because profile data includes a different song ID; this report
does not claim the complete packed files are identical.

The earlier ReRiff 44.1 kHz cache `5tIhK47VDu_v18` had 192.069656 seconds,
640 beats, first beat 0.615329 and tempo 199.634537, matching the corresponding
fields in the new offline-patch result. Its packed-file hash was
`5cd09378ccac1280c7d736f05ecb58e509c0f1776f66146dd961ea85bdc29528`.
The old complete 44.1 kHz analysis was not retained for an all-fields comparison.

## Interpretation and limits

`418.150757 * 44100 / 96000 = 192.088004` seconds, within 0.018348 seconds
of the 44.1 kHz analysis span. This strongly supports a fixed-44.1-kHz time-base
assumption when analyzing higher-rate FLAC PCM. A beat span is not the full
container duration, so 192.07 versus 195.39 seconds is not by itself another bug.

The reported silent extended race and late audio restart agree with the wrong
analysis span. The precise audio restart trigger has not been traced. Existing
original FLAC decoder research locates the wrapper at VA 0x004f9080 and dispatch
at 0x0048d9c0; that research alone does not establish the exact missing resampling
instruction/path. No cache edits, original-file patches, resampling hooks or
matching-byte credit were made for this finding.

Evidence supplied locally: `DAT bug test/44/` and `DAT bug test/96/` under the
installed game; old decoded 96 kHz analysis retained at
`/tmp/reriff-track-timing/eXFoJ1V4AX_v18.json` during investigation.
