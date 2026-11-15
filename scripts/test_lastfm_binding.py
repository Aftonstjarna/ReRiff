"""Verify scrobble observation addresses against the pinned original executable."""
import argparse
import hashlib
from pathlib import Path
import struct

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('original', type=Path)
a = p.parse_args()
b = a.original.read_bytes()
assert hashlib.sha256(b).hexdigest() == '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
pe = struct.unpack_from('<I', b, 60)[0]
optional = pe + 24
image_base = struct.unpack_from('<I', b, optional + 28)[0]
sections = optional + struct.unpack_from('<H', b, pe + 20)[0]
def read(rva, size):
    for i in range(struct.unpack_from('<H', b, pe + 6)[0]):
        _, start, length, raw = struct.unpack_from('<IIII', b, sections + i * 40 + 8)
        if start <= rva and rva + size <= start + length:
            return b[raw + rva - start:raw + rva - start + size]
    raise AssertionError('RVA not present in original')
for slot, thunk, fixup in [(2, 0x21ef04, 0x7a39f8), (3, 0x21f01f, 0x7a39fc)]:
    assert struct.unpack('<I', read(0x7a399c + slot * 4, 4))[0] == image_base + thunk
    assert read(thunk, 6) == b'\xff\x25' + struct.pack('<I', image_base + fixup)
for name, rva, size in [('gaRace.enter', 0x21ede0, 280), ('gaRace.tick', 0x21ef88, 139),
                        ('gaAudioManager.getMusicFileDuration', 0x2c8dc0, 28),
                        ('gaRaceManager.setPaused', 0x21f3d4, 23),
                        ('gaRaceManager.onNode0Hit', 0x21f358, 112),
                        ('gaSaveGame.writeSongCache', 0x2f6f18, 722)]:
    header = read(rva, 12)
    assert struct.unpack_from('<I', header, 4)[0] == size
    print(name, hex(rva), 'IL SHA256', hashlib.sha256(read(rva + 12, size)).hexdigest())
print('PASS: original whole-file hash, race slots/thunks and observed method sizes')
