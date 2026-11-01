# Original M4A support: older versus current Game.exe

## Result

The current executable retains the M4A file-dialog filter but has removed the
older native worker's M4A dispatch branch. The older build's M4A decoding wrapper
is absent from that path in the current build. Changing the file filter or
renaming an M4A file cannot restore this implementation.

This is a verified binary comparison, not an implemented or validated M4A patch.
Neither executable was modified.

| Build | SHA256 | Bytes | PE linker timestamp (UTC) |
| --- | --- | ---: | --- |
| Supplied older EXE | `ba9913af9c788b68f30defd311edfa5b9c0dc07d19edf4b34c4123f23bdb98ce` | 8,756,736 | 2017-02-07 12:41:06 |
| Current supported EXE | `2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2` | 8,635,392 | 2017-03-13 14:00:12 |

Linker timestamps identify the supplied files; they do not establish public
release numbers or the developer's reason for removing support.

## Verified changes

- Both files contain the picker strings `Audio files (*.mp3, *.ogg, *.wav,
  *.m4a, *.FLAC)` and `*.mp3;*.ogg;*.wav;*.m4a;*.FLAC`.
- The older file contains native decoder RTTI `wsAudioDecoderM4A` and an isolated
  `.m4a` extension constant. The current file lacks both markers while retaining
  the MP3, Ogg, and FLAC decoder class markers.
- The native decode worker starts at VA `0x0048D9C0` in both supplied builds.
  In the older build, `0x0048DA5F` pushes `.m4a` at `0x00A1912C`, performs a
  case-insensitive comparison, and branches to `0x0048DB29` on a match.
  The call at `0x0048DB45` invokes the M4A wrapper at `0x004F91F0`.
- In the current build, the same dispatch position starts by comparing `.flac`
  at `0x009FBE0C`; subsequent branches handle `.fla`, `.mp3`, and `.ogg`.
  There is no `.m4a` branch. Unmatched extensions reach the worker's completion
  path without invoking an M4A decoder.
- The older wrapper at `0x004F91F0` checks for an MP4 `ftyp` header, opens the
  in-memory container, selects a compatible audio track, initializes a decoder,
  and decodes its packets. Its callers and native instructions were checked
  alongside Ghidra's output. Ghidra's inferred prototypes are incomplete.
- The old conversion loop at `0x004F94B0` takes signed 16-bit samples from the
  first channel, advances by the channel count, and multiplies by the float at
  `0x00A23780`: exactly `1 / 32768`. It returns mono float PCM, a frame count,
  and extracted artist/title fields. This channel policy is not an average of
  the channels.

The shared DLL import names are the same in these two files. The evidence does
not support diagnosing this as a missing newly introduced external codec DLL.
Some AAC-related text remains in the current file; this report does not claim
that every AAC-related routine anywhere in the executable was removed.

## ReRiff implementation path

Add a source-built native audio compatibility module to the experimental client:

1. Check the current EXE hash and exact dispatch bytes before installing any
   runtime hook. Keep Game.exe on disk unchanged, preserving the existing menu
   hook's supported-file checks and the separate matching-recovery baseline.
2. Extend the native audio dispatcher with an M4A path. Preserve the existing
   decoders, worker events, and buffer cleanup for every other format.
3. Restore the older container/AAC functionality from recovered source and
   suitable codec sources. Return the game's expected mono float buffer, frame
   count, and metadata, with allocation/free behavior compatible with its CRT.
4. Match the old channel selection and 16-bit-to-float normalization before
   claiming compatibility with old track generation. Validate AAC packet delay,
   sample counts, supported rates, metadata, and failure behavior on real files.
5. Compare the same AAC-in-M4A inputs against the older EXE, then verify import,
   playback, caching, and original track generation in the current game.

The old wrapper takes input through registers and output/metadata arguments on
the stack; the caller cleans 28 bytes. A normal guessed `__fastcall` prototype
is insufficient. The adapter requires an explicitly verified x86 ABI shim and
safe installation while the audio worker can be active.

Copying the old wrapper's bytes into the current EXE is insufficient: its calls,
data tables, runtime imports, and decoder dependencies refer to the old image.
External conversion to a different source file also does not demonstrate this
patch's decoding or track-generation compatibility.

## Evidence and coverage

ILSpy was used for managed metadata/file-filter comparisons. Ghidra exports and
original-byte disassembly for both factories and the old wrapper are retained
locally under `build/m4a-comparison/`. The older file was imported into an isolated
Ghidra project; the current recovery database was opened read-only.

Verified matching reconstruction remains 64,760 / 8,635,392 bytes (0.749937%).
Research output and the supplied older bytes add no matching-byte credit.
