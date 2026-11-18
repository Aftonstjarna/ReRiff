"""Compare the prestige entry ID with the hash-pinned original Settings switch."""
import argparse
import hashlib
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('original', type=Path)
a = p.parse_args()
image = a.original.read_bytes()
assert hashlib.sha256(image).hexdigest() == '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
pe = struct.unpack_from('<I', image, 60)[0]
sections = pe + 24 + struct.unpack_from('<H', image, pe + 20)[0]

def read(rva, size):
    for i in range(struct.unpack_from('<H', image, pe + 6)[0]):
        virtual_size, start, raw_size, raw = struct.unpack_from('<IIII', image, sections + i * 40 + 8)
        if start <= rva and rva + size <= start + raw_size:
            return image[raw + rva - start:raw + rva - start + size]
    raise AssertionError('RVA outside original raw sections')

def body(rva):
    flags, stack, size = struct.unpack('<HHI', read(rva, 8))
    assert flags & 3 == 3
    return read(rva + (flags >> 12) * 4, size)

settings = body(0x2f0794)
credits = body(0x31d660)
assert credits[:2] == b'\x02\x1f'  # self, ldc.i4.s state
state = credits[2]
at = 0xa1
assert settings[at] == 0x45
count = struct.unpack_from('<I', settings, at + 1)[0]
assert count == 5
end = at + 5 + 4 * count
matches = []
targets = {}
for index in range(count):
    target = end + struct.unpack_from('<i', settings, at + 5 + 4 * index)[0]
    if settings[target:target + 3] == bytes([0x02, 0x1f, state]):
        matches.append(index)
        targets[index] = target
assert len(matches) == 1
source = (ROOT / 'ExperimentalMenu/prestige_prompt.h').read_text()
actual = int(re.search(r'#define PRESTIGE_CREDITS_SELECTION (\d+)', source)[1])
assert actual == matches[0], (actual, matches)
code_rva = 0x2f0794 + (struct.unpack('<H', read(0x2f0794, 2))[0] >> 12) * 4
for label, expected, rva in [('credits_switch', settings[at:end], code_rva + at), ('credits_transition', bytes([0x02, 0x1f, state]), code_rva + targets[actual])]:
    values = re.search(r'\b' + label + r'\[\]=\{([^}]+)\}', source)[1]
    assert bytes(int(v.strip(), 0) for v in values.split(',')) == expected
    address = int(re.search(r'memcmp\(base\+(0x[0-9a-f]+),' + label, source)[1], 16)
    assert address == rva and read(address, len(expected)) == expected
print(f'PASS: original Credits constructor state {state}, original switch action {actual}, and exact runtime byte guards agree')
