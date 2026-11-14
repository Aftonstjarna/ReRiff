"""Compare ghost-result guards and achievement identity with the original depot."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
from zipfile import ZipFile

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('original', type=Path)
a = p.parse_args()
image = a.original.read_bytes()
assert hashlib.sha256(image).hexdigest() == '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
pe = struct.unpack_from('<I', image, 60)[0]
sections = pe+24+struct.unpack_from('<H', image, pe+20)[0]
def read(rva, size):
    for i in range(struct.unpack_from('<H', image, pe+6)[0]):
        _, start, raw_size, raw = struct.unpack_from('<IIII', image, sections+i*40+8)
        if start <= rva and rva+size <= start+raw_size:
            return image[raw+rva-start:raw+rva-start+size]
    raise AssertionError('Original RVA outside raw sections')
source = (ROOT/'ExperimentalMenu/progression_ghost.h').read_text()
assert read(0x7a4048+8,4) == struct.pack('<I',0x400000+0x2347e0)
assert read(0x2347e0,6) == b'\xff\x25'+struct.pack('<I',0x400000+0x7a40c8)
values = re.search(r'ghost_original_awards\[\]=\{([^}]+)\}', source)[1]
assert bytes(int(x.strip(),0) for x in values.split(',')) == read(0x2346c2,0x4c5-0x456)
assert 'base+0x7a4048' in source and 'base+0x2347e0' in source and 'base+0x7a40c8' in source
code = read(0x234260+12,0x4c5)
assert code[0x458:0x462] == bytes.fromhex('28d405000628ac070006')
assert code[0x492:0x49a] == bytes.fromhex('1f201728800e0006')  # native friend award32
assert code[0x4bc:0x4c4] == bytes.fromhex('1f131728800e0006')  # native recommended award19
assert code[0x49c:0x4a1] == bytes.fromhex('1109183323')  # ghost kind2
assert code[0x4af:0x4b7] == bytes.fromhex('110a6c11166c360d')  # strict win, unordered excluded
selected = read(0x332898+12,0x30d)
assert selected[0x291:0x299] == bytes.fromhex('2030010000581954')  # leader kind3
assert selected[0x2fa:0x302] == bytes.fromhex('2030010000581854')
assert selected[0x303:0x30c] == bytes.fromhex('203401000058110d56')
# Native profile formats ACH_Title%i with stored ID+1.
profile = read(0x325bbc+12,0x6ff)
assert profile[0x2a9:0x2ac] == bytes.fromhex('071758')
with ZipFile(a.original.parent/'vfs.zip') as archive:
    tokens = archive.read('strings/tokens.dat')
    at = 12
    banks = []
    for _ in range(struct.unpack_from('<I',tokens,8)[0]):
        length,count,_ = struct.unpack_from('<III',tokens,at)
        at += 12
        banks.append((tokens[at:at+length].decode('ascii'),count))
        at += length
    assert banks == [('Common',651),('FrontEnd',0),('BackEnd',0)]
    at += 256*4
    names = []
    for _ in range(651):
        assert tokens[at] == 0
        end = tokens.index(b'\0',at+1)
        names.append(tokens[at+1:end].decode('ascii'))
        at = end+1
    raw = archive.read('strings/common/us.str')
    at = 8
    texts = []
    while at < len(raw):
        count = struct.unpack_from('<I',raw,at)[0]
        at += 4
        texts.append(raw[at:at+count*2].decode('utf-16-le'))
        at += count*2
    assert len(names) == len(texts) == 651
    strings = dict(zip(names,texts))
    assert strings['ach_title33'] == 'Friend or Foe'
    assert strings['ach_desc33'] == "Beat a friend's ghost"
    assert strings['ach_title20'] == "Can't Touch This"
print('PASS: original Friend or Foe identity32, recommended kind2 / leader kind3, strict rounded-score win, original versus vtable/fixup and exact award guards')
