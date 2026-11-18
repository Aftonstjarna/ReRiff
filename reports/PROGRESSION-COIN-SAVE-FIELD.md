# Original progression coin field and checksum

Reference Game.exe: Steam depot 351991/1687655, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the local recovery native notes; progression serialization is not
covered there. Evidence here is original managed CIL, not matching reconstruction.

`gaSaveGameChunkProgression.incTokens` (CIL RVA 0x2FC948, 56 bytes) adjusts
signed int32 at object +284, clamps negative results to zero, marks +4 dirty.
`load` (RVA 0x2FC2B4, 528 bytes) accepts versions 5 and 6.
`save` (RVA 0x2FC4D8, 451 bytes) emits version 6 followed by 39 flags,
one flag, 22 groups of (one flag, 16 bits, nine flags), object +280 (32 bits),
then +284 (32 bits), +288 (64 bits), +276 (32 bits), +8 (32 bits).
Thus the version-6 coin field starts at bit 660 and the body is 820 bits.
These are relative to the progression chunk, not universally to save.bin.
Version-5 differences are not covered by these offset claims.

`gaBitWriter.writeBit` (CIL RVA 0x2F55DC, 75 bytes) stores bits least-significant
first within each byte. Its accumulator starts at 1 in gaSaveGame.save2;
for each bit: shift left, OR the bit, truncate to 32 bits, then XOR 0x04C11DB7
if the resulting high bit is set. save2 appends the current accumulator as
32 least-significant-first bits. load2 compares against the accumulator before
reading the stored checksum. This differs from substituting a standard CRC API.

A read-only private version-6 save inspection located a progression chunk at
byte 352, checked its coin count and reproduced its stored checksum exactly.
A hypothetical zero-balance edit was calculated in memory only, preserving
adjacent packed bits and updating the checksum at chunk-relative bit 820.
No actual save was changed or loaded in-game. File offsets are sample-specific;
chunk discovery and end-to-end edit acceptance remain to be validated.

## Save-editor prototype validation

The same original serializer writes 22 car groups starting at bit 56. Each
26-bit group contains car ownership at +0, an existing 16-bit per-car value
at +1 (preserved), and nine pack flags at +17. Native
`gaSaveGameChunkProgression.isPackUnlocked` delegates to
`PackedUnlocks.getCarPackUnlockState`; the original `paint_PC.cfg` contains
per-car `*Pack1` through `*Pack9` definitions (not every car uses all nine).
The editor exposes these serialized slots numerically rather than guessing
localized display names or mapping absent slots to actual garage cars.

The standalone `RiffRacer-SaveEditor` prototype implements the observed version-6 format, locating a unique
checksum-valid chunk rather than fixing its file offset. It changes only
coins, ownership and pack flags, then writes the original checksum recurrence.
Synthetic tests check every untouched bit, malformed and ambiguous saves,
range limits, atomic replacement, two backup generations, and refusal after
external file modification. A private real save round-trips unchanged.
Native GTK and Win32 GUI tests exercise selection, pending edits and reload;
Windows execution was under Wine, not a native Windows retest.
In-game acceptance of edited car/paint flags remains untested. This is a
ReRiff utility implementation; no original assembly reconstruction or
matching-byte credit is claimed.
