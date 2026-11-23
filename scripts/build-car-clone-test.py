#!/usr/bin/env python3
"""Build a local-only Pulse clone in the unused dod slot from owned stock assets.

Does not install, launch, or change saves. Do not distribute the generated assets.
Native layouts: NativeLinux/archive.hpp and https://github.com/Aftonstjarna/ReRiff/wiki/Research-CAR-CAPACITY-AND-SAVE-TRACE.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import sys
import zipfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'packaging'))
from native_archive import build_volume


def name_hash(name):
    value = 5387
    for c in name.encode('ascii'):
        value = (value * 33 + c) & 0xffffffff
    return value


def parse_config(raw):
    magic, version, pool_size, count, roots = struct.unpack_from('<5I', raw)
    assert magic == 0x46434647 and version == 3
    pool = raw[20 + count * 32:]
    assert len(pool) == pool_size
    def text(offset):
        return pool[offset:pool.index(0, offset)].decode('ascii')
    def node(index):
        n, r1, v, r2, h, kind, size, r3 = struct.unpack_from('<8I', raw, 20 + index * 32)
        assert not (r1 or r2 or r3) and kind in (0, 1, 2)
        name = text(n)
        assert name_hash(name) == h
        value = text(v) if kind == 0 else struct.unpack('<f', struct.pack('<I', v))[0] if kind == 1 else [node(i) for i in range(v, v + size)]
        return dict(name=name, type=kind, value=value)
    return [node(i) for i in range(roots)]


def encode_config(roots):
    records = [None] * len(roots)
    pool = bytearray()
    offsets = {}
    def text(s):
        if s not in offsets:
            offsets[s] = len(pool)
            pool.extend(s.encode('ascii') + b'\0')
        return offsets[s]
    def fill(i, item):
        name, kind, value = item['name'], item['type'], item['value']
        size = 0
        if kind == 2:
            start, size = len(records), len(value)
            records.extend([None] * size)
            for j, child in enumerate(value):
                fill(start + j, child)
            value = start
        elif kind == 0:
            value = text(value)
        else:
            value = struct.unpack('<I', struct.pack('<f', value))[0]
        records[i] = struct.pack('<8I', text(name), 0, value, 0, name_hash(name), kind, size, 0)
    for i, item in enumerate(roots):
        fill(i, item)
    raw = struct.pack('<5I', 0x46434647, 3, len(pool), len(records), len(roots)) + b''.join(records) + pool
    assert parse_config(raw) == roots
    return raw


def lua_chunk(roots, size_t=4):
    # Lua 5.1, little-endian, 32-bit instructions/floats, matching stock headers.
    # Emit only native table construction and global assignment instructions.
    constants, code = [], []
    max_reg = 0
    def constant(v):
        for i, old in enumerate(constants):
            if type(old) == type(v) and old == v:
                return i
        constants.append(v)
        return len(constants)-1
    def abc(op, a, b=0, c=0):
        code.append(op | a << 6 | c << 14 | b << 23)
    def abx(op, a, bx):
        assert bx < 262144
        code.append(op | a << 6 | bx << 14)
    def load(item, reg):
        nonlocal max_reg
        max_reg = max(max_reg, reg + 3)
        assert max_reg < 250
        if item['type'] != 2:
            if item['name'] == 'unlockedAtStart':
                abc(2, reg, int(bool(item['value'])))  # LOADBOOL; Lua number 0 is truthy.
                return
            abx(1, reg, constant(item['value']))
            return
        abc(10, reg)  # NEWTABLE
        children = item['value']
        array = item['name'] == 'default' or all(not c['name'] for c in children)
        # GFCF arrays retain index zero; Lua's sequence starts at index one.
        # Config car-name/unlock lists have an explicit empty sentinel;
        # paint packs and RGB vectors do not. Lua sequences are one-based.
        sentinel = bool(children) and children[0]['type'] == 0 and children[0]['value'] == ''
        first = 0 if sentinel else 1
        for i, child in enumerate(children):
            abx(1, reg+1, constant(float(i + first) if array else child['name']))
            load(child, reg+2)
            abc(9, reg, reg+1, reg+2)  # SETTABLE
    for item in roots:
        load(item, 0)
        abx(7, 0, constant(item['name']))  # SETGLOBAL
    abc(30, 0, 1)  # RETURN no results
    def string(value):
        raw = value.encode('ascii') + b'\0'
        return len(raw).to_bytes(size_t, 'little') + raw
    out = b'\x1bLua\x51\0\x01\x04' + bytes([size_t, 4, 4, 0])
    out += bytes(size_t) + struct.pack('<II4B', 0, 0, 0, 0, 2, max_reg)
    out += struct.pack('<I', len(code)) + struct.pack('<' + 'I'*len(code), *code)
    out += struct.pack('<I', len(constants))
    for value in constants:
        out += b'\x04' + string(value) if isinstance(value, str) else b'\x03' + struct.pack('<f', value)
    out += struct.pack('<4I', 0, 0, 0, 0)  # prototypes, lineinfo, locals, upvalue names
    return out


def named(items, name):
    return next(i for i in items if i['name'] == name)


def scalar(name, value):
    return dict(name=name, type=0 if isinstance(value, str) else 1, value=value)


def token_names(raw):
    at = 12
    banks = struct.unpack_from('<I', raw, 8)[0]
    count = 0
    for _ in range(banks):
        length, entries, _ = struct.unpack_from('<III', raw, at)
        count += entries
        at += 12 + length
    at += 1024
    names = []
    for _ in range(count):
        assert raw[at] == 0
        end = raw.index(0, at + 1)
        names.append(raw[at+1:end].decode('ascii'))
        at = end + 1
    assert at == len(raw)
    return names


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    assert hashlib.sha256((a.reference/'Game.exe').read_bytes()).hexdigest() == '2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2'
    a.output.mkdir(parents=True, exist_ok=False)
    resources = a.output/'resources'
    provenance = {}
    def write(path, raw):
        dest = resources/path.lower()
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(raw)
    with zipfile.ZipFile(a.reference/'vfs.zip') as z:
        def read(path):
            raw = z.read(path)
            provenance[path] = hashlib.sha256(raw).hexdigest()
            return raw
        for stem in ['vehicleSelect', 'unlocks', 'unlocks_PC', 'paint', 'paint_PC']:
            data = parse_config(read('scripts/config/'+stem+'.cfg'))
            if stem.startswith('paint'):
                paints = named(data, 'Paints')['value']
                clone = copy.deepcopy(named(paints, 'alf'))
                clone['name'] = 'dod'
                clone['value'] = clone['value'][:2]
                for index, pack in enumerate(clone['value']):
                    fields = pack['value']
                    named(fields, 'packName')['value'] = ['alfPack10', 'fabPack10'][index]
                    fields[:] = [f for f in fields if f['name'] != 'packPrice']
                    if index:
                        fields.append(scalar('packPrice', 1.0))
                    for key, rgb in [('col0',(230,40,210) if not index else (30,240,150)),('col1',(30,30,30)),('col2',(240,240,240))]:
                        for channel, value in zip(named(fields,key)['value'],rgb):
                            channel['value'] = float(value)
                paints[paints.index(named(paints, 'dod'))] = clone
            else:
                cars = named(data, 'carNames')['value']
                assert len(cars) == 16 and cars[-1]['value'] == 'pl1'
                assert all(c['value'] != 'dod' for c in cars)
                cars.append(scalar('', 'dod'))
                data.insert(next(i for i,n in enumerate(data) if n['name']=='default'), scalar('dod', 16.0))
                named(data, 'default')['value'].append(scalar('dod', 16.0))
                if stem.startswith('unlocks'):
                    named(data,'carUnlocks')['value'].append(dict(name='',type=2,value=[scalar('unlockedAtStart',0.0),scalar('purchaseUnlock',1.0)]))
            write('scripts/config/'+stem+'.cfg', encode_config(data))
            for suffix, size in [('luac',4),('luac64',8)]:
                # Verify the stock architecture before emitting its counterpart.
                original = read('scripts/config/'+stem+'.'+suffix)
                chunk = lua_chunk(data, size)
                assert original[:12] == chunk[:12], (stem,suffix,original[:12],chunk[:12])
                write('scripts/config/'+stem+'.'+suffix,chunk)
        for name in z.namelist():
            lower = name.lower()
            if any(s in lower for s in ['graphics/vehicles/alf/', 'graphics/vehicles/decals/alf_', 'graphics/be/hud/car/alf/']):
                write(lower.replace('/alf/','/dod/').replace('/alf_','/dod_'),read(name))
        names = token_names(read('strings/tokens.dat'))
        assert 'car_dod' in names and 'alfpack10' in names and 'fabpack10' in names
        changes = {'car_dod':'Pulse Test','alfpack10':'Test Magenta','fabpack10':'Test Mint'}
        for name in z.namelist():
            if not name.startswith('strings/common/') or not name.endswith('.str'):
                continue
            raw = read(name)
            at = 8
            texts = []
            while at < len(raw):
                count = struct.unpack_from('<I',raw,at)[0];at+=4
                texts.append(raw[at:at+count*2].decode('utf-16-le'));at+=count*2
            assert len(texts)==len(names)
            out = bytearray(raw[:8])
            for token, text in zip(names,texts):
                value=changes.get(token,text).encode('utf-16-le')
                out.extend(struct.pack('<I',len(value)//2));out.extend(value)
            write(name,bytes(out))
    pair = build_volume(resources,a.output/'mod','vfs1')
    for source in pair:
        source.rename(source.with_name('vfs'+source.suffix))
    (a.output/'origin.txt').write_text('[origins]\ndod=alf\n')
    (a.output/'manifest.json').write_text(json.dumps(dict(stock_sources=provenance, clone_id='dod', runtime_slot=15, label='Pulse Test', packs=2, limitation='Uses previously unused stock localization tokens; arbitrary new IDs not proven.'),indent=2)+'\n')
    print('Built local-only Pulse Test: slot 15, two paints, native purchase prices 1 coin.')

if __name__ == '__main__':
    main()
