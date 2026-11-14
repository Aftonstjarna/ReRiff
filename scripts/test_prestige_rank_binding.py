"""Compare Rank hook guards and the preserved checker with the pinned original."""
import argparse
import hashlib
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('original', type=Path)
a = parser.parse_args()
image = a.original.read_bytes()
assert hashlib.sha256(image).hexdigest() == '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
pe = struct.unpack_from('<I', image, 60)[0]
sections = pe + 24 + struct.unpack_from('<H', image, pe + 20)[0]
def read(rva, size):
    for i in range(struct.unpack_from('<H', image, pe + 6)[0]):
        _, start, raw_size, raw = struct.unpack_from('<IIII', image, sections + i*40 + 8)
        if start <= rva and rva + size <= start + raw_size:
            return image[raw+rva-start:raw+rva-start+size]
    raise AssertionError('Original RVA outside raw sections')
source = (ROOT/'ExperimentalMenu/progression_rank.h').read_text()
managed = (ROOT/'ClientConfig/ProgressionRank.cs').read_text()
rva = int(re.search(r'BYTE \*method=base\+(0x[0-9a-f]+)', source)[1], 16)
assert rva == 0x2cbdac
header = read(rva, 12)
flags, stack, size, locals_token = struct.unpack('<HHII', header)
assert flags == 0x3003 and stack == 3 and size == 160 and locals_token == 0
code = read(rva+12, size)
for name, expected in [('progression_rank_header',header),('progression_rank_body',code)]:
    values = re.search(r'\b'+name+r'\[\]=\{([^}]+)\}', source)[1]
    assert bytes(int(x.strip(),0) for x in values.split(',')) == expected
assert bytes.fromhex(re.search(r'OriginalHex="([0-9a-f]+)"', managed)[1]) == code
for start, ident, level, call in [(0,10,5,0x10),(0x27,11,10,0x37),(0x4f,12,15,0x5f),(0x77,13,20,0x87)]:
    assert code[start:start+7] == bytes.fromhex('7eb10d00041f')+bytes([ident])
    assert code[call:call+5] == bytes.fromhex('2836100006')
    assert code[call+5:call+7] == bytes.fromhex('6c69')
    assert code[call+7:call+8] == bytes([0x1b if level==5 else 0x1f])
    if level!=5: assert code[call+8] == level
xp_header = read(0x2cbea4,12)
xp = read(0x2cbea4+12,struct.unpack_from('<I',xp_header,4)[0])
assert xp[0x18:0x23] == bytes.fromhex('2862120006022839100006')
assert xp[6:11] == xp[0x25:0x2a] == bytes.fromhex('2836100006')
print('PASS: exact original Rank header/body, IDs10-13 at levels5/10/15/20, and XP award call site; global cycle level calls remain original')
